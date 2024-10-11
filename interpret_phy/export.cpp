#include "export.h"

#include <filesystem>

using namespace std;

namespace phy {

void export_rect(gdstk::Cell &cell, const Rect &rect, const Layout &layout, int layer) {
	cell.polygon_array.append(new gdstk::Polygon(
		gdstk::rectangle(
			gdstk::Vec2{(double)rect.ll[0], (double)rect.ll[1]},
			gdstk::Vec2{(double)rect.ur[0], (double)rect.ur[1]},
			gdstk::make_tag(layout.tech->paint[layer].major, layout.tech->paint[layer].minor))));

	if (rect.hasLabel()) {
		cell.label_array.append(new gdstk::Label{
			.tag = gdstk::make_tag(layout.tech->paint[layer].major, layout.tech->paint[layer].minor),
			.text = strdup(layout.nets[rect.net].name.c_str()),
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

void export_layout(string filename, const Layout &layout) {
	gdstk::Library lib = {};
	lib.init(layout.name.c_str(), ((double)layout.tech->dbunit)*1e-6, ((double)layout.tech->dbunit)*1e-6);
	export_layout(lib, layout);
	lib.write_gds(filename.c_str(), 0, NULL);
	lib.free_all();
}

void export_library(gdstk::Library &lib, const Library &library, set<string> cellNames) {
	for (auto cell = library.macros.begin(); cell != library.macros.end(); cell++) {
		if (not cell->name.empty() and (cellNames.empty() or cellNames.find(cell->name) != cellNames.end())) {
			export_layout(lib, *cell);
		}
	}
}

void export_library(string libname, string filename, const Library &library, set<string> cellNames) {
	gdstk::Library lib = {};
	lib.init(libname.c_str(), ((double)library.tech->dbunit)*1e-6, ((double)library.tech->dbunit)*1e-6);
	export_library(lib, library, cellNames);
	lib.write_gds(filename.c_str(), 0, NULL);
	lib.free_all();
}

void export_lef(string filename, const Layout &layout, int type) {
	// See https://github.com/KLayout/klayout/blob/766dd675c11d98b2461c448035197f6e934cb497/src/plugins/streamers/lefdef/db_plugin/dbLEFDEFImporter.cc#L1085
	// Purpose Name = Placement Code
  // LEFPIN       = LEFPIN
	// PIN          = PIN
	// LEFPINNAME   = LEFLABEL
	// PINNAME      = LABEL
	// FILL         = FILL
	// FILLOPC      = FILLOPC
	// LEFOBS       = OBS
	// SPNET        = SPNET
	// NET          = NET
	// VIA          = VIA
	// BLOCKAGE     = BLK
	// ALL = [LEFPIN, PIN, FILL, FILLOPC, OBS, SPNET, NET, VIA]

	FILE *fptr = fopen(filename.c_str(), "w");
	if (fptr == nullptr) {
		return;
	}

	fprintf(fptr, "MACRO %s\n", layout.name.c_str());

	enum {
		CELL = 0,
		BLOCK = 1,
		TAP = 2,
		FILL = 3
	};
	switch (type) {	
	case CELL: fprintf(fptr, "CLASS CORE ;\n"); break;
	case BLOCK: fprintf(fptr, "CLASS BLOCK ;\n"); break;
	case TAP: fprintf(fptr, "CLASS CORE WELLTAP ;\n"); break;
	case FILL: fprintf(fptr, "CLASS CORE SPACER ;\n"); break;
	default: fprintf(fptr, "CLASS CORE ;\n");
	}

	Rect bbox = layout.bbox();
	vec2i size = bbox.ur-bbox.ll;
	fprintf(fptr, "\tORIGIN %d %d ;\n", -bbox.ll[0], -bbox.ll[1]);
	fprintf(fptr, "\tFOREIGN %s %d %d ;\n", layout.name.c_str(), bbox.ll[0], bbox.ll[1]);
	fprintf(fptr, "\tSIZE %d BY %d ;\n", size[0], size[1]);
	fprintf(fptr, "\tSYMMETRY X Y ;\n");
	if (type != BLOCK) {
		fprintf(fptr, "\tSITE CoreSite ;\n");
	}

	for (int i = 0; i < (int)layout.nets.size(); i++) {
		auto n = layout.nets.begin() + i;
		if (n->isInput or n->isOutput) {
			string direction = "INOUT";
			if (not n->isInput) {
				direction = "OUTPUT";
			} else if (not n->isOutput) {
				direction = "INPUT";
			}

			fprintf(fptr, "\tPIN %s\n", n->name.c_str());
			if (n->isSub) {
				fprintf(fptr, "\t\tDIRECTION INOUT ;\n");
				fprintf(fptr, "\t\tUSE POWER ;\n");
			} else if (n->isVdd) {
				fprintf(fptr, "\t\tDIRECTION INOUT ;\n");
				fprintf(fptr, "\t\tUSE POWER ;\n");
				if (type != BLOCK) {
					fprintf(fptr, "\t\tSHAPE ABUTMENT ;\n");
				}
			} else if (n->isGND) {
				fprintf(fptr, "\t\tDIRECTION INOUT ;\n");
				fprintf(fptr, "\t\tUSE GROUND ;\n");
				if (type != BLOCK) {
					fprintf(fptr, "\t\tSHAPE ABUTMENT ;\n");
				}
			} else {
				fprintf(fptr, "\t\tDIRECTION %s ;\n", direction.c_str());
				fprintf(fptr, "\t\tUSE SIGNAL ;\n");
			}

			fprintf(fptr, "\t\tPORT\n");
			for (auto layer = layout.layers.begin(); layer != layout.layers.end(); layer++) {
				bool found = false;
				for (auto rect = layer->geo.begin(); rect != layer->geo.end(); rect++) {
					if (rect->net == i) {
						if (not found) {
							fprintf(fptr, "\t\t\tLAYER %s ;\n", layout.tech->paint[layer->draw].name.c_str());
							found = true;
						}
						fprintf(fptr, "\t\t\t\tRECT %d %d %d %d ;\n", rect->ll[0], rect->ll[1], rect->ur[0], rect->ur[1]);
					}
				}
			}
			fprintf(fptr, "\t\tEND\n");
		}
	}

	fprintf(fptr, "\tOBS\n");
	for (auto layer = layout.layers.begin(); layer != layout.layers.end(); layer++) {
		bool found = false;
		for (auto rect = layer->geo.begin(); rect != layer->geo.end(); rect++) {
			if (rect->net < 0 or (
				not layout.nets[rect->net].isInput and
				not layout.nets[rect->net].isOutput and
				not layout.nets[rect->net].isVdd and
				not layout.nets[rect->net].isGND and
				not layout.nets[rect->net].isSub)) {
				if (not found) {
					fprintf(fptr, "\t\tLAYER %s ;\n", layout.tech->paint[layer->draw].name.c_str());
					found = true;
				}
				fprintf(fptr, "\t\t\tRECT %d %d %d %d ;\n", rect->ll[0], rect->ll[1], rect->ur[0], rect->ur[1]);
			}
		}
	}
	fprintf(fptr, "\tEND\n");
	fprintf(fptr, "END %s\n\n", layout.name.c_str());
	
	fclose(fptr);
}

}
