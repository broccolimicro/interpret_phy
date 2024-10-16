#pragma once

// windows doesn't it's constants in math.h, this is required to compile gdstk import on windows.
#define M_PI 3.14159265358979323846
#include <gdstk/gdstk.hpp>
#include <phy/Library.h>

#include <set>

using namespace std;

namespace phy {

void export_rect(gdstk::Cell &cell, const Rect &rect, const Layout &layout, int layer);
void export_layer(gdstk::Cell &cell, const Layer &layer, const Layout &layout);
gdstk::Cell *export_layout(const Layout &layout);
void export_layout(string filename, const Layout &layout);
void export_library(gdstk::Library &lib, const Library &library, set<string> cellNames = set<string>());
void export_library(string libname, string filename, const Library &library, set<string> cellNames = set<string>());

void export_lef(string filename, const Layout &layout, int type=0);

}
