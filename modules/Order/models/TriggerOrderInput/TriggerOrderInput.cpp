#include "TriggerOrderInput.h"

TriggerOrderInput::TriggerOrderInput(
        const std::string &symbol,
        const std::string &side,
        const std::string &type,
        const std::string &timeInForce,
        const double &quantity,
        const double &price,
        const double &stopPrice,
        const bool &reduceOnly
) :
        OrderInput(symbol, side, type, timeInForce, quantity, price),
        stopPrice(stopPrice),
        reduceOnly(reduceOnly) {}
