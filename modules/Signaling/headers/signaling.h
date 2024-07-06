#ifndef SIGNALING_H
#define SIGNALING_H

#include <string>
#include <vector>
#include "../../Order/models/APIParams/APIParams.h"

namespace Signaling {
    void mockSignal(const std::string &filePath, const APIParams &apiParams);
    std::vector<int> readSignalsFromFile(const std::string &filePath);
}

#endif // SIGNALING_H
