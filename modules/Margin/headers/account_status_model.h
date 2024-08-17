#ifndef ACCOUNT_STATUS_MODEL_HPP
#define ACCOUNT_STATUS_MODEL_HPP

#include <string>
#include <vector>

struct AccountStatus {
    double totalInitialMargin = 0.0;
    double totalMaintMargin = 0.0;
    double totalWalletBalance = 0.0;
    double totalUnrealizedProfit = 0.0;
    double totalMarginBalance = 0.0;
    double totalPositionInitialMargin = 0.0;
    double totalOpenOrderInitialMargin = 0.0;
    double totalCrossWalletBalance = 0.0;
    double totalCrossUnPnl = 0.0;
    double availableBalance = 0.0;
    double maxWithdrawAmount = 0.0;
    std::vector<std::string> assets;
};

#endif // ACCOUNT_STATUS_MODEL_HPP

