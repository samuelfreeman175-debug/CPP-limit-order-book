#pragma once
#include "types.h"


#include <limits>

struct Constants {
    static const Price InvalidPrice = std::numeric_limits<Price>::min();
};