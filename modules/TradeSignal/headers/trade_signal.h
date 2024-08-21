#ifndef SIGNAL_H
#define SIGNAL_H

#include "APIParams.h"
#include "SignalQueue.h"
#include <string>

namespace SignalService {
void process(int signal, const APIParams &apiParams, double quantity, double entryPrice);
void cancelWithDelay(int signal, const APIParams &apiParams, SignalQueue &cancelQueue);
void placeTpAndSlOrders(const APIParams &apiParams, std::string side, double quantity, double tpPrice, double slPrice);
} // namespace Signal

#endif // SIGNAL_H
