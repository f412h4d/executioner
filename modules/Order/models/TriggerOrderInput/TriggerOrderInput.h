#ifndef TRIGGER_ORDER_INPUT_H
#define TRIGGER_ORDER_INPUT_H

#include <string>
#include "OrderInput.h"

class TriggerOrderInput : public OrderInput {
public:
    double stopPrice;

    TriggerOrderInput(
            const std::string &symbol,
            const std::string &side,
            const std::string &type,
            const std::string &timeInForce,
            const double &quantity,
            const double &price,
            const double &stopPrice
    );
};

#endif // TRIGGER_ORDER_INPUT_H
