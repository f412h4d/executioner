#ifndef PRICE_SETTINGS_HPP
#define PRICE_SETTINGS_HPP

struct PriceSettings {
    double entry_gap = 0.001;
    double tp_price_percentage = 0.014;
    double sl_price_percentage = -0.01;
    double current_price = 0.0;
    double calculated_tp = 0.0;
    double calculated_sl = 0.0;
    double calculated_price = 0.0;
    double quantity = 0.0;
};

#endif // PRICE_SETTINGS_HPP

