#pragma once
#include "order.h"
#include "trade.h"
#include "types.h"
#include <map>
#include <unordered_map>
#include <functional>
#include <algorithm>
#include <numeric>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <memory>
#include <chrono>



class OrderBook {
    public:

        Trades addOrder(OrderPointer order) {
            std::scoped_lock ordersLock{ordersMutex_};
            return addOrderInternal(order);
        }

        Trades addOrders(const OrderPointers& orders) {
            std::scoped_lock ordersLock{ordersMutex_};
            Trades trades{};
            for (const auto& order: orders) {
                auto orderTrades = addOrderInternal(order);
                trades.insert(trades.end(), orderTrades.begin(), orderTrades.end());
            }
            return trades;
        }

        Trades modifyOrder(const OrderModify& order) {
            std::scoped_lock ordersLock{ordersMutex_};
            return modifyOrderInternal(order);
        }

        Trades modifyOrders(const OrderModifies& orders) {
            std::scoped_lock ordersLock{ordersMutex_};
            Trades trades{};
            for (const auto& order: orders) {
                auto orderTrades = modifyOrderInternal(order);
                trades.insert(trades.end(), orderTrades.begin(), orderTrades.end());
            }
            return trades;
        }


        void cancelOrder(OrderID orderID) {
            std::scoped_lock ordersLock{ordersMutex_};
            cancelOrderInternal(orderID);
        }

        void cancelOrders(const OrderIDs& orderIDs) {
            std::scoped_lock ordersLock{ordersMutex_};
            for (const auto& orderID: orderIDs) {
                cancelOrderInternal(orderID);
            }
        }

        void PruneGoodForDayOrders() {
            using namespace std::chrono;
            const auto end = hours(16);

            while (true) {
                const auto now = system_clock::now();
                const auto now_c = system_clock::to_time_t(now);
                std::tm now_parts;
                localtime_r(&now_c, &now_parts);

                if (now_parts.tm_hour >= end.count())
                    now_parts.tm_mday += 1;

                now_parts.tm_hour = end.count();
                now_parts.tm_min = 0;
                now_parts.tm_sec = 0;

                auto next = system_clock::from_time_t(mktime(&now_parts));
                auto till = next - now + milliseconds(100);

                {
                    std::unique_lock ordersLock{ordersMutex_};

                    if (shutdownConditionVariable_.wait_for(ordersLock, till, [this] {return shutdown_.load(std::memory_order_acquire); }))
                        return;
                }
                OrderIDs orderIDs;
                {
                    std::scoped_lock ordersLock{ordersMutex_};
                    
                    for (const auto& [_, entry] : orders_) {
                        const auto& [order, _] = entry;

                        if (order->getOrderType() != OrderType::GoodForDay)
                        continue;

                        orderIDs.push_back(order->getOrderID());
                    }
                }

                cancelOrders(orderIDs);
            }
        }

        std::size_t Size() const {
            std::scoped_lock ordersLock{ordersMutex_};
            return orders_.size();
        }


        OrderBookLevelInfos getOrderInfos() const {
            std::scoped_lock ordersLock{ordersMutex_};
            LevelInfos bidInfos{};
            LevelInfos askInfos{};

            bidInfos.reserve(bids_.size());
            askInfos.reserve(asks_.size());

            auto createLevelInfos = [this](Price price, const OrderPointers& orders) {
                Quantity totalQuantity = data_.at(price).quantity_;
                return LevelInfo{price, totalQuantity};
            };
       

            for (const auto& [price, orders] : bids_) {
                bidInfos.push_back(createLevelInfos(price, orders));
    
            }

            for (const auto& [price, orders] : asks_) {
                askInfos.push_back(createLevelInfos(price, orders));
    
            }
            
            return OrderBookLevelInfos{std::move(bidInfos), std::move(askInfos)};

        }
    
        OrderBook() : ordersPruneThread_{ [this] { PruneGoodForDayOrders();}} {}

        ~OrderBook() {
            shutdown_.store(true, std::memory_order_release);
            shutdownConditionVariable_.notify_one();
            ordersPruneThread_.join();
        }


    private:

        struct OrderEntry {
            OrderPointer orderPointer{};
            OrderPointers::iterator orderIterator{};
        };

        struct LevelData {
            Quantity quantity_{};
            Quantity count_{};

            enum class Action {
                Add,
                Remove,
                Match
            };
        };

        std::unordered_map<Price, LevelData> data_; 
        std::map<Price, OrderPointers, std::greater<Price>> bids_;
        std::map<Price, OrderPointers, std::less<Price>> asks_;
        std::unordered_map<OrderID, OrderEntry> orders_;
        mutable std::mutex ordersMutex_;
        std::condition_variable shutdownConditionVariable_;
        std::atomic<bool> shutdown_{false};
        std::thread ordersPruneThread_;

        void updateLevelData(Price price, Quantity quantity, LevelData::Action action) {
            auto& data = data_[price];

            if (action == LevelData::Action::Add) {
                data.quantity_ += quantity;
                data.count_ += 1;
            }
            else if (action == LevelData::Action::Remove) {
                data.quantity_ -= quantity;
                data.count_ -= 1;
            }
            else if (action == LevelData::Action::Match) {
                data.quantity_ -= quantity;
            }
            if (data.count_ == 0) {
                data_.erase(price);
            }

        }
        Trades addOrderInternal(OrderPointer order) {
            if (!order) {
                return {};
            }
            if (orders_.contains(order->getOrderID())) {
                return {};
            }
            if (order->getOrderType() == OrderType::Market) {
                if (order->getSide() == Side::Buy && !asks_.empty()) {
                    const auto& [worstAsk, _] = *asks_.rbegin();
                    order->ToGoodTillCancel(worstAsk);
                }
                else if (order->getSide() == Side::Sell && !bids_.empty()) {
                    const auto& [worstBid, _] = *bids_.rbegin();
                    order->ToGoodTillCancel(worstBid);
                }
                else {
                    return {};
                }
            }
            if (order->getOrderType() == OrderType::FillandKill && !canMatch(order->getSide(), order->getPrice())) {
                return {};
            }

            if (order->getOrderType() == OrderType::FillorKill && !canFullyFill(order->getSide(), order->getPrice(), order->getInitialQuantity())) {
                return {};
            }

            OrderPointers::iterator iterator{};

            if (order->getSide() == Side::Buy) {
                auto& orders = bids_[order->getPrice()];
                orders.push_back(order);
                iterator = std::prev(orders.end());
            } 
            else {
                auto& orders = asks_[order->getPrice()];
                orders.push_back(order);
                iterator = std::prev(orders.end());

            }
                
            orders_.insert({order->getOrderID(), OrderEntry{order, iterator}});
            updateLevelData(order->getPrice(), order->getInitialQuantity(), LevelData::Action::Add);
            return matchOrders();
            }

        Trades modifyOrderInternal(const OrderModify& order) {
            if (!orders_.contains(order.getOrderID())) {
                return {};
            }
                const auto& [existingOrder, _] = orders_.at(order.getOrderID());
                OrderType orderType{existingOrder->getOrderType()};
                
                cancelOrderInternal(order.getOrderID());
                return addOrderInternal(order.ToOrderPointer(orderType));
            }

        void cancelOrderInternal(OrderID orderID) {  
            auto it = orders_.find(orderID);
            if (it == orders_.end()) {
                return;
            }

            const auto [order, iterator] = it->second;
            orders_.erase(it);

            if (order->getSide() == Side::Sell) {
                auto price = order->getPrice();
                auto& orders = asks_.at(price);

                orders.erase(iterator);

                if (orders.empty()) {
                    asks_.erase(price);
                    }
                }
                else {
                    auto price = order->getPrice();
                    auto& orders = bids_.at(price);

                    orders.erase(iterator);

                    if (orders.empty()) {
                        bids_.erase(price);
                    }
                }
            updateLevelData(order->getPrice(), order->getRemainingQuantity(), LevelData::Action::Remove);
        }


        bool canMatch(Side side, Price price) const {
            if (side == Side::Buy) {
                if (asks_.empty()) {
                    return false;
                }

                const auto& [bestAsk, _] = *asks_.begin();
                return price >= bestAsk;
                }
                else {
                    if (bids_.empty()) 
                        return false;
                    
                    const auto& [bestBid, _] = *bids_.begin();
                    return price <= bestBid;

                }
            }

        Trades matchOrders() {
            Trades trades{};
            trades.reserve(orders_.size());

            while (true) {

                if (bids_.empty() || asks_.empty())
                break;

                auto& [bidPrice, bids] = *bids_.begin();
                auto& [askPrice, asks] = *asks_.begin();

                if (bidPrice < askPrice)
                    break;

                while (bids.size() && asks.size()) {
                    auto& bid = bids.front();
                    auto& ask = asks.front();

                    Quantity quantity = std::min(bid->getRemainingQuantity(), ask->getRemainingQuantity());

                    bid->fill(quantity);
                    ask->fill(quantity);         
                    
                    updateLevelData(bidPrice, quantity, bid->isFilled() ? LevelData::Action::Remove : LevelData::Action::Match);
                    updateLevelData(askPrice, quantity, ask->isFilled() ? LevelData::Action::Remove : LevelData::Action::Match);
                    trades.push_back(Trade{TradeInfo{bid->getOrderID(), bid->getPrice(), quantity}, TradeInfo {ask->getOrderID(), ask->getPrice(), quantity}});

                    if (bid-> isFilled()) {
                        orders_.erase(bid->getOrderID());
                        bids.pop_front();
                    
                    }

                    if (ask->isFilled()) {
                        orders_.erase(ask->getOrderID());
                        asks.pop_front();
                }

            }

                if (bids.empty()) {
                    bids_.erase(bids_.begin());
                }

                if (asks.empty()) {
                    asks_.erase(asks_.begin());
                }

        }
            if (!bids_.empty()) {
                auto& [_, bids] = *bids_.begin();
                auto& bid = bids.front();
                if (bid ->getOrderType() == OrderType::FillandKill)

                cancelOrderInternal(bid->getOrderID());
            }
            
            if (!asks_.empty()) {
                auto& [_, asks] = *asks_.begin();
                auto& ask = asks.front();
                if (ask ->getOrderType() == OrderType::FillandKill)
                cancelOrderInternal(ask->getOrderID());
            }

            return trades;
        }


        bool canFullyFill(Side side, Price price, Quantity quantity) const {
            if (!canMatch(side, price)) {
                return false;
                }
            if (side == Side::Buy) {
                for (const auto& [levelPrice, orders] : asks_) {
                    if (levelPrice > price) {
                        return false;
                    }
                    const auto levelQuantity = data_.at(levelPrice).quantity_;
                    if (quantity <= levelQuantity) {
                        return true;
                    }
                    quantity -= levelQuantity;
                }
            }

                
            else if (side == Side::Sell) {
                for (const auto& [levelPrice, orders] : bids_) {
                    if (levelPrice < price) {
                        return false;
                    }
                    const auto levelQuantity = data_.at(levelPrice).quantity_;
                    if (quantity <= levelQuantity) {
                        return true;
                    }
                    quantity -= levelQuantity;
            }
        }
            return false;
        
    }
};