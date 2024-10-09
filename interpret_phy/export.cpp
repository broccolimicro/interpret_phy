#include "export.h"

namespace phy {

void export_rect(gdstk::Cell &cell, const Rect &rect, const Layout &layout, int layer) {
	cell.polygon_array.append(new gdstk::Polygon(
		gdstk::rectangle(
			gdstk::Vec2{(double)rect.ll[0], (double)rect.ll[1]},
			gdstk::Vec2{(double)rect.ur[0], (double)rect.ur[1]},
			gdstk::make_tag(layout.tech.paint[layer].major, layout.tech.paint[layer].minor))));

	if (rect.hasLabel()) {
		cell.label_array.append(new gdstk::Label{
			.tag = gdstk::make_tag(layout.tech.paint[layer].major, layout.tech.paint[layer].minor),
			.text = strdup(layout.nets[rect.net].c_str()),
			.origin = gdstk::Vec2{(double)((rect.ll[0] + rect.ur[0])/2), (double)((rect.ll[1]+rect.ur[1])/2)},
			.magnification = 1,
		});
	}
}

void export_layer(gdstk::Cell &cell, const Layer &layer, const Layout &layout) {
	for (auto r = layer.geo.begin(); r != layer.geo.end(); r++) {
		export_rect(cell, *r, layout, layer.draw);
	}
}

void export_layout(gdstk::Library &lib, const Layout &layout) {
	gdstk::Cell *cell = new gdstk::Cell();
	cell->init(layout.name.c_str());
	for (auto layer = layout.layers.begin(); layer != layout.layers.end(); layer++) {
		export_layer(*cell, *layer, layout);
	}
	lib.cell_array.append(cell);
}

void export_library(gdstk::Library &lib, const Library &library, set<string> cellNames) {
	for (auto cell = library.macros.begin(); cell != library.macros.end(); cell++) {
		if (cellNames.empty() or cellNames.find(cell->name) != cellNames.end()) {
			export_layout(lib, *cell);
		}
	}
}

void export_library(string libname, string filename, const Library &library, set<string> cellNames) {
	gdstk::Library lib = {};
	lib.init(libname.c_str(), library.tech.dbunit*1e-6, library.tech.dbunit*1e-6);
	export_library(lib, library, cellNames);
	lib.write_gds(filename.c_str(), 0, NULL);
	lib.free_all();
}

}
