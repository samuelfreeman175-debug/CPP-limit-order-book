#pragma once
#include "types.h"
#include "constants.h"
#include <stdexcept>
#include <list>
#include <memory>


class Order {

    public:

        Order(OrderID orderID, OrderType orderType, Side side, Price price, Quantity quantity) 
        :  OrderID_(orderID), OrderType_(orderType), Side_(side), Price_(price), initialQuantity_(quantity), remainingQuantity_(quantity){
            
            if (quantity == 0) {
                throw std::invalid_argument("Quantity cannot be 0");   
            }
        }

        Order(OrderID orderID, Side side, Quantity quantity)
        :  Order(orderID, OrderType::Market, side, Constants::InvalidPrice, quantity) {}

        OrderID getOrderID() const {
            return OrderID_;
        }

        OrderType getOrderType() const {
            return OrderType_;
        }

        Side getSide() const {
            return Side_;
        }

        Price getPrice() const {
            return Price_;
        }

        Quantity getInitialQuantity() const {
            return initialQuantity_;
        }

        Quantity getRemainingQuantity() const {
            return remainingQuantity_;
        }

        Quantity getFilledQuantity() const {
            return initialQuantity_ - remainingQuantity_;
        }

        bool isFilled() const {
            return remainingQuantity_ == 0;
        }

        void ToGoodTillCancel(Price price) {
            if (OrderType_ != OrderType::Market) {
                throw std::logic_error("This is not a Market Order.");
            }
            Price_ = price;
            OrderType_ = OrderType::GoodTillCancel;
        }
    
        void fill(Quantity quantity) {

            if (quantity > remainingQuantity_) {
            throw std::logic_error("Cannot fill order for more than its remaining quantity.");
            }
            remainingQuantity_ -= quantity;
        }

    private:

        OrderID OrderID_{};
        OrderType OrderType_{};
        Side Side_{};
        Price Price_{};
        Quantity initialQuantity_{};
        Quantity remainingQuantity_{};

};

using OrderPointer = std::shared_ptr<Order>;

using OrderPointers = std::list<OrderPointer>;


class OrderModify {
    public:

        OrderModify(OrderID orderID, Side side, Price price, Quantity quantity)
        :   OrderID_{orderID}, Side_{side}, Price_{price}, Quantity_{quantity} {}

        OrderID getOrderID() const {
            return OrderID_;
        }
        Side getSide() const {
            return Side_;
        }

        Price getPrice() const {
            return Price_;
        }

        Quantity getQuantity() const {
            return Quantity_;
        }

        OrderPointer ToOrderPointer(OrderType type) const {
            return std::make_shared<Order>(getOrderID(), type, getSide(), getPrice(), getQuantity());
        }

    private:

        OrderID OrderID_{};
        Side Side_{};
        Price Price_{};
        Quantity Quantity_{};
};
using OrderModifies = std::vector<OrderModify>;

