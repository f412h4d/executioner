#include "TriggerOrderInput.h"

#include <utility>

TriggerOrderInput::TriggerOrderInput(
        const std::string &symbol,
        const std::string &side,
        const std::string &type,
        const std::string &timeInForce,
        const double &quantity,
        const double &price,
        const double &stopPrice,
        const bool &reduceOnly,
        std::string priceType

) :
        OrderInput(symbol, side, type, timeInForce, quantity, price),
        stopPrice(stopPrice),
        reduceOnly(reduceOnly),
        priceType(std::move(priceType)) {}
