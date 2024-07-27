#ifndef NEWS_H
#define NEWS_H

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <stdexcept>

// Get the current date in YYYY-MM-DD format
std::string getCurrentDate();

// Check if the given datetime string is today
bool isToday(const std::string& dateTime);

// Parse a datetime string into a chrono time_point
std::chrono::system_clock::time_point parseDateTime(const std::string& dateTime);

// Create a range from a sorted list of dates
std::pair<std::chrono::system_clock::time_point, std::chrono::system_clock::time_point> createRange(
    const std::vector<std::chrono::system_clock::time_point>& sortedDates);

// Check if the current time is within a given range
bool isCurrentTimeInRange(const std::pair<std::chrono::system_clock::time_point, std::chrono::system_clock::time_point>& range);

// Get the current date and time in YYYY-MM-DD HH:MM:SS format
std::string getCurrentDateTime();

// Fetch news dates from the file and return the range
std::pair<std::chrono::system_clock::time_point, std::chrono::system_clock::time_point> fetchNewsDateRange();

#endif // NEWS_H
