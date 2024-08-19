#ifndef SIGNAL_SETTINGS_H
#define SIGNAL_SETTINGS_H

#include <mutex>

#include "trade_signal_model.h"
#include "datetime_range_model.h"

struct SignalSettings {
  TradeSignal signal;
  DatetimeRange news_range;
  DatetimeRange deactivate_range;

  std::mutex signal_mutex;
  std::mutex news_range_mutex;
  std::mutex deactivate_range_mutex;
};

#endif // SIGNAL_SETTINGS_H

