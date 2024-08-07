#ifndef MARGIN_H
#define MARGIN_H

#include <string>
#include <nlohmann/json.hpp>
#include "../../Order/models/APIParams/APIParams.h"

namespace Margin {
    double getPrice(
            const APIParams &apiParams,
            const std::string &symbol
    );

    nlohmann::json getPositions(
            const APIParams &apiParams,
            const std::string &symbol
    );

    nlohmann::json getOpenOrders(
            const APIParams &apiParams,
            const std::string &symbol
    );

    double getBalance(
            const APIParams &apiParams,
            const std::string &asset
    );

    nlohmann::json setLeverage(
            const APIParams &apiParams,
            const std::string &symbol,
            int leverage
    );
}

#endif // MARGIN_H
