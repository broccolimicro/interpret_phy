#include "export.h"

using namespace phy;

void emitGDS(gdstk::Cell &cell, const Rect &rect, const Layout &layout, int layer) {
	cell.polygon_array.append(new gdstk::Polygon(
		gdstk::rectangle(
			gdstk::Vec2{(double)rect.ll[0], (double)rect.ll[1]},
			gdstk::Vec2{(double)rect.ur[0], (double)rect.ur[1]},
			gdstk::make_tag(layout.tech->paint[layer].major, layout.tech->paint[layer].minor))));

	if (rect.hasLabel()) {
		cell.label_array.append(new gdstk::Label{
			.tag = gdstk::make_tag(layout.tech->paint[layer].major, layout.tech->paint[layer].minor),
			.text = strdup(layout.nets[rect.net].c_str()),
			.origin = gdstk::Vec2{(double)((rect.ll[0] + rect.ur[0])/2), (double)((rect.ll[1]+rect.ur[1])/2)},
			.magnification = 1,
		});
	}
}

void emitGDS(gdstk::Cell &cell, const Layer &layer, const Layout &layout) {
	for (auto r = layer.geo.begin(); r != layer.geo.end(); r++) {
		emitGDS(cell, *r, layout, layer.draw);
	}
}

void emitGDS(gdstk::Library &lib, const Layout &layout) {
	gdstk::Cell *cell = new gdstk::Cell();
	cell->init(layout.name.c_str());
	for (auto layer = layout.layers.begin(); layer != layout.layers.end(); layer++) {
		emitGDS(*cell, *layer, layout);
	}
	lib.cell_array.append(cell);
}

