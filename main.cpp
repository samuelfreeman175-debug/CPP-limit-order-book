#include <iostream> 
#include <cassert>
#include "order.h"
#include "orderbook.h"

int main() {

    Order myOrder{1, OrderType::GoodTillCancel, Side::Buy, 10050, 100};

    assert(myOrder.getInitialQuantity() == 100);
    assert(myOrder.getRemainingQuantity() == 100);
    assert(myOrder.getFilledQuantity() == 0);
    assert(myOrder.isFilled() == false);
   
    myOrder.fill(40);
    assert(myOrder.getRemainingQuantity() == 60);
    assert(myOrder.getFilledQuantity() == 40);

    myOrder.fill(60);
    assert(myOrder.isFilled() == true);

    try {
        myOrder.fill(10);
        assert(false);
    } catch (const std::logic_error& e) {
        std::cout << "Caught expected error: " << e.what() << "\n";
    }

    std::cout << "All order tests #1 passes" << std::endl;

    OrderModify myOrder2{1, Side::Buy, 10000, 250};
    
    OrderPointer modifiedOrder = myOrder2.ToOrderPointer(OrderType::GoodTillCancel);

    modifiedOrder->fill(50);

    assert(modifiedOrder -> getRemainingQuantity() == 200);

    std::cout << "All order tests #2 passes" << std::endl;

    OrderBook myOrderBook{};
    TradeLogger Logger{"trades.csv"};
    Trades trade1 = myOrderBook.addOrder(std::make_shared<Order>(1, OrderType::GoodTillCancel, Side::Buy, 100, 50));
    Trades trade2 = myOrderBook.addOrder(std::make_shared<Order>(2, OrderType::GoodTillCancel, Side::Sell, 110, 50));



    assert(trade1.empty());
    assert(myOrderBook.Size() == 2);

    myOrderBook.getOrderInfos();

    assert(myOrderBook.getOrderInfos().getBids().size() == 1);
    assert(myOrderBook.getOrderInfos().getAsks().size() == 1);
    assert(myOrderBook.getOrderInfos().getBids()[0].price_ == 100);
    assert(myOrderBook.getOrderInfos().getBids()[0].quantity_ == 50);

    myOrderBook.cancelOrder(1);

    assert(myOrderBook.Size() == 1);
    assert(myOrderBook.getOrderInfos().getBids().empty());

    std::cout << "All orderbook tests #1 passes" << std::endl;

    OrderBook myOrderBook2{};
    Trades trade3 = myOrderBook2.addOrder(std::make_shared<Order>(2, OrderType::GoodTillCancel, Side::Sell, 110, 50));
    Trades trade4 = myOrderBook2.addOrder(std::make_shared<Order>(3, OrderType::GoodTillCancel, Side::Buy, 110, 30));
    Logger.log(trade4);
    assert(trade4[0].getTimestamp() > 0);

    assert(myOrderBook2.Size() == 1);
    assert(trade4[0].getBidTrade().OrderID_ == 3);
    assert(trade4[0].getBidTrade().Quantity_ ==30);
    assert(myOrderBook2.getOrderInfos().getAsks()[0].quantity_ == 20);
    
    std::cout << "All orderbook tests #2 passes" << std::endl;

    Trades trade5 = myOrderBook2.addOrder(std::make_shared<Order>(4, OrderType::FillandKill, Side::Buy, 105, 10));
    Logger.log(trade5);

    assert(trade5.empty());
    assert(myOrderBook2.Size() == 1);

    Trades trade6 = myOrderBook2.addOrder(std::make_shared<Order>(5, OrderType::FillandKill, Side::Buy, 110, 50));
    Logger.log(trade6);
    assert(trade6[0].getTimestamp() > 0);

    assert(trade6.size() == 1);
    assert(trade6[0].getBidTrade().Quantity_ == 20);
    assert(myOrderBook2.Size() == 0);

    std::cout << "All orderbook tests #3 passes" << std::endl;
    
    Trades trade7 = myOrderBook2.addOrder(std::make_shared<Order>(6, OrderType::GoodTillCancel, Side::Buy, 100, 25));
    Trades trade8 = myOrderBook2.addOrder(std::make_shared<Order>(7, OrderType::GoodTillCancel, Side::Sell, 105, 25));

    Trades trade9 = myOrderBook2.modifyOrder(OrderModify{6, Side::Buy, 105, 25});
    Logger.log(trade9);
    assert(trade9[0].getTimestamp() > 0);

    assert(trade9.size() == 1);
    assert(myOrderBook2.Size() == 0);

    std::cout << "All orderbook tests #4 passes" << std::endl;
   
    Trades trade10 = myOrderBook.addOrder(std::make_shared<Order>(10, OrderType::GoodTillCancel, Side::Sell, 110, 200));
    Trades trade11 = myOrderBook.addOrder(std::make_shared<Order>(11, OrderType::Market, Side::Buy, 140, 80));
    Trades trade12 = myOrderBook.addOrder(std::make_shared<Order>(12, OrderType::FillorKill, Side::Buy, 110, 50));
    
    Logger.log(trade10);
    Logger.log(trade11);
    Logger.log(trade12);

    assert(trade10.empty());

    assert(trade11.size() == 2);
    assert(trade11[0].getBidTrade().OrderID_ == 11);
    assert(trade11[0].getBidTrade().Quantity_ == 50);
    assert(trade11[1].getBidTrade().OrderID_ == 11);
    assert(trade11[1].getBidTrade().Quantity_ == 30);

    assert(trade12.size() == 1);
    assert(trade12[0].getBidTrade().OrderID_ == 12);
    assert(trade12[0].getBidTrade().Quantity_ == 50);

    assert(myOrderBook.Size() == 1);
    assert(myOrderBook.getOrderInfos().getAsks()[0].quantity_ == 120);

    std::cout << "All orderbook tests #5 passes" << std::endl;




















}