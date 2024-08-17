#include "../headers/utils.h"
#include <chrono>
#include <climits>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <sstream>
#include <unistd.h>
#include <uuid/uuid.h>
#include <cstdio>
#include <string>

namespace Utils {

void printMapElements(const std::map<std::string, std::string> &env) {
  for (const auto &pair : env) {
    std::cout << pair.first << ": " << pair.second << std::endl;
  }
}
// ENV Utils
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
  return {result, (count > 0) ? static_cast<std::size_t>(count) : 0};
}

std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
    if (!pipe) throw std::runtime_error("popen() failed!");
    while (!feof(pipe.get())) {
        if (fgets(buffer.data(), 128, pipe.get()) != nullptr)
            result += buffer.data();
    }
    return result;
}

// HTTP Request Utils
std::string urlEncode(const std::string &value) {
  std::ostringstream escaped;
  escaped.fill('0');
  escaped << std::hex;

  for (char c : value) {
    if (isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == '.' || c == '~') {
      escaped << c;
    } else {
      escaped << '%' << std::setw(2) << std::uppercase << static_cast<int>(static_cast<unsigned char>(c));
    }
  }

  return escaped.str();
}

std::string HMAC_SHA256(const std::string &key, const std::string &data) {
  unsigned char *digest;
  unsigned int len = SHA256_DIGEST_LENGTH;

  digest =
      HMAC(EVP_sha256(), key.c_str(), static_cast<int>(key.length()),
           reinterpret_cast<const unsigned char *>(data.c_str()), static_cast<int>(data.length()), nullptr, nullptr);

  char mdString[SHA256_DIGEST_LENGTH * 2 + 1];
  for (unsigned int i = 0; i < len; i++) {
    sprintf(&mdString[i * 2], "%02x", digest[i]);
  }

  return {mdString};
}

double roundToTickSize(double price, double tick_size) { return std::round(price / tick_size) * tick_size; }

// Generate a UUID
std::string generate_uuid() {
  uuid_t uuid;
  uuid_generate(uuid);
  char uuid_str[37]; // UUID string is 36 characters plus null terminator
  uuid_unparse(uuid, uuid_str);
  return std::string(uuid_str);
}

// Get current timestamp in milliseconds
long getCurrentTimestamp() {
  using namespace std::chrono;
  return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}
} // namespace Utils
