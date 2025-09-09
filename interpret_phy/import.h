#pragma once

#ifndef M_PI
// windows doesn't it's constants in math.h, this is required to compile gdstk import on windows.
#define M_PI 3.14159265358979323846
#endif

#include <gdstk/gdstk.hpp>
#include <phy/Layout.h>
#include <phy/Library.h>

namespace phy {

string import_name(string name);
bool import_layout(Layout &layout, const gdstk::Cell *gdsCell);
bool import_layout(Layout &layout, const gdstk::Library &lib, string cellName);
bool import_layout(Layout &layout, string path, string cellName);
bool import_library(phy::Library &lib, string path);

}
