#pragma once

#include <gdstk/gdstk.hpp>
#include <phy/Library.h>

#include <set>

using namespace phy;
using namespace std;

void emitGDS(gdstk::Cell &cell, const Rect &rect, const Layout &layout, int layer);
void emitGDS(gdstk::Cell &cell, const Layer &layer, const Layout &layout);
void emitGDS(gdstk::Library &lib, const Layout &layout);
void emitGDS(gdstk::Library &lib, const Library &library, set<string> cellNames = set<string>());
void emitGDS(string libname, string filename, const Library &library, set<string> cellNames = set<string>());
