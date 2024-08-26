#pragma once

#include <gdstk/gdstk.hpp>
#include <phy/Layout.h>

using namespace phy;

void loadGDS(Layout &layout, const Tech &tech, string path, string cellName);
