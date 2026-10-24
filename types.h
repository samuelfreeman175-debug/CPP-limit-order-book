#pragma once
#include <cstdint>
#include <vector>
#include <utility>

enum class OrderType {

    GoodTillCancel,
    FillandKill
};

enum class Side {

    Buy,
    Sell
};

using Price = std::int32_t;
using Quantity = std::uint32_t;
using OrderID = std::uint64_t;

struct LevelInfo {
    Price price_;
    Quantity quantity_;
};

using LevelInfos = std::vector<LevelInfo>;

class OrderBookLevelInfos {
    public:

    OrderBookLevelInfos(LevelInfos bids, LevelInfos asks)
    :  Bids(std::move(bids)), Asks(std::move(asks)) {}


    const LevelInfos& getBids() const {
        return Bids;
    }

    const LevelInfos& getAsks() const {
        return Asks;
    }

    private:
        std::vector<LevelInfo> Bids{};
        std::vector<LevelInfo> Asks{};

};