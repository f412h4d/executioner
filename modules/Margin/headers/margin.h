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

    nlohmann::json getBalance(
            const APIParams &apiParams,
            const std::string &asset
    );
}

#endif // MARGIN_H
