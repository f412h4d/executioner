#include "OrderInput.h"

OrderInput::OrderInput(
        const std::string &symbol,
        const std::string &side,
        const std::string &type,
        const std::string &timeInForce,
        const double &quantity,
        const double &price
) :
        symbol(symbol),
        side(side),
        type(type),
        timeInForce(timeInForce),
        quantity(quantity),
        price(price) {}
