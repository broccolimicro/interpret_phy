#pragma once

// windows doesn't it's constants in math.h, this is required to compile gdstk import on windows.
#define M_PI 3.14159265358979323846
#include <gdstk/gdstk.hpp>
#include <phy/Library.h>

#include <map>

using namespace std;

namespace phy {

string export_name(string name);
void export_rect(gdstk::Cell &cell, const Rect &rect, const Layout &layout, int layer);
void export_label(gdstk::Cell &cell, const Label &lbl, const Layout &layout, int layer);
void export_layer(gdstk::Cell &cell, const Layer &layer, const Layout &layout);
bool export_instance(gdstk::Cell &cell, const Instance &inst, const map<int, gdstk::Cell*> &cells);
gdstk::Cell *export_layout(const Layout &layout, const map<int, gdstk::Cell*> *cells=nullptr);
void export_layout(gdstk::GdsWriter &writer, const Library &library, int idx, map<int, gdstk::Cell*> &cells);
void export_layout(string filename, const Layout &layout);
void export_library(gdstk::Library &lib, const Library &library);
void export_library(string libname, string filename, const Library &library);

void export_lef(string filename, const Layout &layout, int type=0);

}
