#ifndef ORDER_INPUT_H
#define ORDER_INPUT_H

#include <string>

class OrderInput {
public:
    std::string symbol;
    std::string side;
    std::string type;
    std::string timeInForce;
    double quantity;
    double price;

    OrderInput(
            const std::string &symbol,
            const std::string &side,
            const std::string &type,
            const std::string &timeInForce,
            const double &quantity,
            const double &price
    );
};

#endif // ORDER_INPUT_H
