#pragma once

#include <gdstk/gdstk.hpp>
#include <phy/Library.h>

#include <set>

using namespace std;

namespace phy {

void export_rect(gdstk::Cell &cell, const Rect &rect, const Layout &layout, int layer);
void export_layer(gdstk::Cell &cell, const Layer &layer, const Layout &layout);
void export_layout(gdstk::Library &lib, const Layout &layout);
void export_library(gdstk::Library &lib, const Library &library, set<string> cellNames = set<string>());
void export_library(string libname, string filename, const Library &library, set<string> cellNames = set<string>());

}
