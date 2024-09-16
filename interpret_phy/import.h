#pragma once

// windows doesn't it's constants in math.h, this is required to compile gdstk import on windows.
#define M_PI 3.14159265358979323846
#include <gdstk/gdstk.hpp>
#include <phy/Layout.h>

namespace phy {

void import_layout(Layout &layout, const Tech &tech, string path, string cellName);

}
