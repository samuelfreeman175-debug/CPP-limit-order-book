#pragma once
#include "order.h"
#include "trade.h"
#include "types.h"
#include <map>
#include <unordered_map>
#include <functional>
#include <algorithm>
#include <numeric>

class OrderBook {
    public:

        void cancelOrder(OrderID orderID) {
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
        }

        Trades addOrder(OrderPointer order) {
            if (!order) {
                return {};
            }
            if (orders_.contains(order->getOrderID())) {
                return {};
            }
            if (order->getOrderType() == OrderType::FillandKill && !canMatch(order->getSide(), order->getPrice())) {
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
            return matchOrders();
            }
        

        Trades modifyOrder(const OrderModify& order) {
            if (!orders_.contains(order.getOrderID())) {
                return {};
            }
                const auto& [existingOrder, _] = orders_.at(order.getOrderID());
                OrderType orderType{existingOrder->getOrderType()};
                
                cancelOrder(order.getOrderID());
                return addOrder(order.ToOrderPointer(orderType));
            }


        std::size_t Size() const {
            return orders_.size();
        }


        OrderBookLevelInfos getOrderInfos() const {
            LevelInfos bidInfos{};
            LevelInfos askInfos{};

            bidInfos.reserve(bids_.size());
            askInfos.reserve(asks_.size());

            auto createLevelInfos = [](Price price, const OrderPointers& orders) {
                Quantity totalQuantity = std::accumulate(orders.begin(), orders.end(), Quantity{0}, [](Quantity runningSum, const OrderPointer& order) {
                    return runningSum + order->getRemainingQuantity();
                });

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
    

    private:

        struct OrderEntry {
            OrderPointer orderPointer{};
            OrderPointers::iterator orderIterator{};
        };

        std::map<Price, OrderPointers, std::greater<Price>> bids_;

        std::map<Price, OrderPointers, std::less<Price>> asks_;

        std::unordered_map<OrderID, OrderEntry> orders_;

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

                cancelOrder(bid->getOrderID());
            }
            
            if (!asks_.empty()) {
                auto& [_, asks] = *asks_.begin();
                auto& ask = asks.front();
                if (ask ->getOrderType() == OrderType::FillandKill)
                cancelOrder(ask->getOrderID());
            }

            return trades;
        }
};
