#pragma once

#include "ssd1306.hpp"

namespace Hardware
{
template <typename i2c_device>
using Ssd1306_128x64 = Ssd1306<i2c_device, 8>;
}
