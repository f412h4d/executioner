#include "../headers/utils.h"
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <climits>
#include <openssl/hmac.h>
#include <iomanip>

namespace Utils {
    std::string urlEncode(const std::string &value) {
        std::ostringstream escaped;
        escaped.fill('0');
        escaped << std::hex;

        for (char c: value) {
            if (isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == '.' || c == '~') {
                escaped << c;
            } else {
                escaped << '%' << std::setw(2) << std::uppercase << static_cast<int>(static_cast<unsigned char>(c));
            }
        }

        return escaped.str();
    }

    std::string trimQuotes(const std::string &str) {
        if (str.size() >= 2 && str.front() == '"' && str.back() == '"') {
            return str.substr(1, str.size() - 2);
        }
        return str;
    }

    std::map<std::string, std::string> loadEnvFile(const std::string &filePath) {
        std::map<std::string, std::string> env;
        std::ifstream file(filePath);
        std::string line;

        while (std::getline(file, line)) {
            std::istringstream is_line(line);
            std::string key;
            if (std::getline(is_line, key, '=')) {
                std::string value;
                if (std::getline(is_line, value)) {
                    key = trimQuotes(key);
                    value = trimQuotes(value);
                    env[key] = value;
                }
            }
        }

        return env;
    }

    std::string getExecutablePath() {
        char result[PATH_MAX];
        ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
        return std::string(result, (count > 0) ? count : 0);
    }


    std::string HMAC_SHA256(const std::string &key, const std::string &data) {
        unsigned char *digest;
        digest = HMAC(EVP_sha256(), key.c_str(), key.length(), (unsigned char *) data.c_str(), data.length(), NULL,
                      NULL);
        char mdString[65];
        for (int i = 0; i < 32; i++) {
            sprintf(&mdString[i * 2], "%02x", (unsigned int) digest[i]);
        }
        return std::string(mdString);
    }
}
