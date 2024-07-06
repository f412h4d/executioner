#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <map>

namespace Utils {
    std::string urlEncode(const std::string &value);

    std::string trimQuotes(const std::string &str);

    std::map<std::string, std::string> loadEnvFile(const std::string &filePath);

    std::string getExecutablePath();

    std::string HMAC_SHA256(const std::string &key, const std::string &data);
}

#endif //UTILS_H
