#pragma once

#include <gdstk/gdstk.hpp>
#include <phy/Layout.h>

namespace phy {

void import_layout(Layout &layout, const Tech &tech, string path, string cellName);

}
