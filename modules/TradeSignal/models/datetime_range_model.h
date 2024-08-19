#ifndef DATETIME_RANGE_H
#define DATETIME_RANGE_H

#include <nlohmann/json.hpp>
#include <string>

struct DatetimeRange {
  std::string start; // Start date and time in UTC format "2024-08-05 14:00:00"
  std::string end;   // End date and time in UTC format "2024-08-05 16:00:00"

  std::string toJsonString() const {
    nlohmann::json json_message = {
        {"start", start},
        {"end", end}};
    return json_message.dump();
  }

  static DatetimeRange fromJsonString(const std::string &json_string) {
    DatetimeRange datetime_range;
    nlohmann::json json_message = nlohmann::json::parse(json_string);

    datetime_range.start = json_message["start"].get<std::string>();
    datetime_range.end = json_message["end"].get<std::string>();

    return datetime_range;
  }
};

#endif // DATETIME_RANGE_H

