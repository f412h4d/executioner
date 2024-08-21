#ifndef ORDER_MODEL_HPP
#define ORDER_MODEL_HPP

#include <string>

struct Order {
  std::string order_id = "";
  std::string status = "";
  std::string side = "";
  double quantity = 0.0;
  double price = 0.0;
};

#endif // ORDER_MODEL_HPP
