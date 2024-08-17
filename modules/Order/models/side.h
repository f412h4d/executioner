#ifndef SIDE_H
#define SIDE_H

#include <string>

class Side {
public:
  static std::string toTPSL(int signal) {
    switch (signal) {
    case 1:
      return "SELL";
    case -1:
      return "BUY";
    default:
      return "";
    }
  }

  static std::string fromSignal(int signal) {
    switch (signal) {
    case -1:
      return "SELL";
    case 1:
      return "BUY";
    default:
      return "";
    }
  }
};

#endif // SIDE_H
