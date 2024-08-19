#include "../headers/news.h"
#include "../../Utils/headers/utils.h"

std::string getCurrentDate() {
  auto now = std::chrono::system_clock::now();
  std::time_t now_time = std::chrono::system_clock::to_time_t(now);
  std::tm local_tm = *std::localtime(&now_time);
  std::ostringstream oss;
  oss << std::put_time(&local_tm, "%Y-%m-%d");
  return oss.str();
}

bool isToday(const std::string &dateTime) {
  return dateTime.substr(0, 10) == getCurrentDate();
}

std::chrono::system_clock::time_point
parseDateTime(const std::string &dateTime) {
  std::tm tm = {};
  std::istringstream ss(dateTime);
  ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
  return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

std::pair<std::chrono::system_clock::time_point,
          std::chrono::system_clock::time_point>
createRange(
    const std::vector<std::chrono::system_clock::time_point> &sortedDates) {
  if (sortedDates.empty()) {
    throw std::runtime_error("No dates provided");
  }
  auto minDate = sortedDates.front() - std::chrono::hours(13);
  auto maxDate = sortedDates.back() + std::chrono::hours(1);
  return {minDate, maxDate};
}

bool isCurrentTimeInRange(
    const std::pair<std::chrono::system_clock::time_point,
                    std::chrono::system_clock::time_point> &range) {
  auto now = std::chrono::system_clock::now();
  return now >= range.first && now <= range.second;
}

bool isTimeInRange(
    const std::pair<std::chrono::system_clock::time_point,
                    std::chrono::system_clock::time_point> &range,
    const std::chrono::system_clock::time_point &time) {
  return time >= range.first && time <= range.second;
}

std::string getCurrentDateTime() {
  auto now = std::chrono::system_clock::now();
  std::time_t now_time = std::chrono::system_clock::to_time_t(now);
  std::tm local_tm = *std::localtime(&now_time);
  std::ostringstream oss;
  oss << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S");
  return oss.str();
}

std::pair<std::chrono::system_clock::time_point,
          std::chrono::system_clock::time_point>
fetchNewsDateRange() {
  std::string output = Utils::exec("../run_gsutil_news.sh");
  std::istringstream iss(output);
  std::string line;
  std::vector<std::string> dateTimes;

  // Skip the first line (header)
  std::getline(iss, line);

  // Read the rest of the lines
  while (std::getline(iss, line)) {
    if (isToday(line)) { // Filter dates to only include today's dates
      dateTimes.push_back(line);
    }
  }

  if (dateTimes.empty()) {
    // std::cout << "No dates found for today." << std::endl;
    // Return a default range or handle as per your application's logic
    std::chrono::system_clock::time_point far_past =
        std::chrono::system_clock::now() -
        std::chrono::hours(24 * 365 * 100); // 100 years in the past
    return {far_past, far_past};
  }

  std::vector<std::chrono::system_clock::time_point> sortedDates;
  std::transform(dateTimes.begin(), dateTimes.end(),
                 std::back_inserter(sortedDates), parseDateTime);
  std::sort(sortedDates.begin(), sortedDates.end());

  return createRange(sortedDates);
}
