#ifndef SIGNALING_H
#define SIGNALING_H

#include <string>
#include <vector>
#include "../../Order/models/APIParams/APIParams.h"

namespace Signaling {
    [[noreturn]] void init(const APIParams &apiParams);
}

#endif // SIGNALING_H
