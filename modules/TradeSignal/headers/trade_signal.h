#ifndef SIGNAL_H
#define SIGNAL_H

#include "APIParams.h"
#include "SignalQueue.h"
#include <string>

namespace SignalService {

void placeTpAndSlOrders(const APIParams &apiParams, std::string side, double quantity, double tpPrice, double slPrice);
void cancelWithDelay(int signal, const APIParams &apiParams, SignalQueue &cancelQueue);
void process(int signal, const APIParams &apiParams, double quantity, double entryPrice);

} // namespace Signal

#endif // SIGNAL_H
