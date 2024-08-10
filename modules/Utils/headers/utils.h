#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <map>

namespace Utils {
    void printMapElements(const std::map<std::string, std::string> &env);

    std::string exec(const char* cmd);

    std::string trimQuotes(const std::string &str);

    std::map<std::string, std::string> loadEnvFile(const std::string &filePath);

    std::string getExecutablePath();

    std::string urlEncode(const std::string &value);

    std::string HMAC_SHA256(const std::string &key, const std::string &data);

    double roundToTickSize(double price, double tick_size);
}

#endif //UTILS_H
