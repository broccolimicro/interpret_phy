#pragma once

#include <gdstk/gdstk.hpp>
#include <phy/Layout.h>

using namespace phy;

void emitGDS(gdstk::Cell &cell, const Rect &rect, const Layout &layout, int layer);
void emitGDS(gdstk::Cell &cell, const Layer &layer, const Layout &layout);
void emitGDS(gdstk::Library &lib, const Layout &layout);
