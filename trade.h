#pragma once
#include "types.h"
#include <fstream>
#include <string>
#include <chrono>
#include <stdexcept>


struct TradeInfo {
        OrderID OrderID_{};
        Price Price_{};
        Quantity Quantity_{};
};

class Trade {
    public:

    Trade(const TradeInfo& bidTrade, const TradeInfo& askTrade)
    : bidTrade_(bidTrade), askTrade_(askTrade)
    {
        auto now = std::chrono::system_clock::now();
        timestamp_ = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
    }

    const TradeInfo& getBidTrade() const {
        return bidTrade_;
    }

    const TradeInfo& getAskTrade() const {
        return askTrade_;
    }

    Timestamp getTimestamp() const {
        return timestamp_;
    }

    private:

    TradeInfo bidTrade_{};
    TradeInfo askTrade_{};
    Timestamp timestamp_{};
};

using Trades = std::vector<Trade>;

class TradeLogger {
    public:

        TradeLogger(const std::string& filename)
        :  file_{filename}
        {
            if (!file_.is_open()) {
                throw std::runtime_error("Failed to open trade log file");
            }
            file_ << "TimeStamp,BidOrderID,BidPrice,AskOrderID,AskPrice,Quantity\n";
            file_.flush();
        }

        void log(const Trades& trades) {
            for (const auto& trade : trades) {
                file_ << trade.getTimestamp() << ","
                << trade.getBidTrade().OrderID_ << ","
                << trade.getBidTrade().Price_ << ","
                << trade.getAskTrade().OrderID_ << ","
                << trade.getAskTrade().Price_ << ","
                << trade.getBidTrade().Quantity_ << "\n";
            }
            file_.flush();
        }

    private:
        std::ofstream file_ {};

};