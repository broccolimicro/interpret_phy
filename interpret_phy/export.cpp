#include "export.h"

#include <filesystem>
#include <vector>
#include <algorithm>

using namespace std;

namespace phy {

string idToString(size_t id) {
	// spice names are not sensitive to capitalization, and only support alphanum
	// characters. Not enough character types to support base64.
	static const string digits = "abcdefghijklmnopqrstuvwxyz012345";

	std::string result;
	for (int i = 0; i < (int)sizeof(size_t); i++) {
		int idx = id & 0x1F;
		id >>= 5;
		result.push_back(digits[idx]);
	}
	return result;
}

string export_name(string name) {
	// Do name mangling
	static map<char, char> replace = {
		{'_', '_'},
		{'.', '0'},
		{'[', '1'},
		{']', '2'},
		{'\'', '3'},
		{'(', '4'},
		{')', '5'},
		{'<', '6'},
		{'>', '7'},
	};

	string result;
	for (auto c = name.begin(); c != name.end(); c++) {
		auto pos = replace.find(*c);
		if (pos != replace.end()) {
			result.push_back('_');
			result.push_back(pos->second);
		} else {
			result.push_back(*c);
		}
	}
	return result;
}

void export_rect(gdstk::Cell &cell, const Rect &rect, const Layout &layout, int layer) {
	cell.polygon_array.append(new gdstk::Polygon(
		gdstk::rectangle(
			gdstk::Vec2{(double)rect.ll[0], (double)rect.ll[1]},
			gdstk::Vec2{(double)rect.ur[0], (double)rect.ur[1]},
			gdstk::make_tag(layout.tech->paint[layer].major, layout.tech->paint[layer].minor))));
}

void export_label(gdstk::Cell &cell, const Label &lbl, const Layout &layout, int layer) {
	cell.label_array.append(new gdstk::Label{
			.tag = gdstk::make_tag(layout.tech->paint[layer].major, layout.tech->paint[layer].minor),
			.text = strdup(export_name(lbl.txt).c_str()),
			.origin = gdstk::Vec2{(double)lbl.pos[0], (double)lbl.pos[1]},
			.magnification = 1,
		});
}

void export_layer(gdstk::Cell &cell, const Layer &layer, const Layout &layout) {
	if (layer.draw >= 0) {
		if (not layout.tech->isLabel(layer.draw)) {
			for (auto r = layer.geo.begin(); r != layer.geo.end(); r++) {
				export_rect(cell, *r, layout, layer.draw);
			}
		}
		for (auto l = layer.lbl.begin(); l != layer.lbl.end(); l++) {
			export_label(cell, *l, layout, layer.draw);
		}
	}
}

bool export_instance(gdstk::Cell &cell, const Instance &inst, const map<int, gdstk::Cell*> &cells) {
	auto pos = cells.find(inst.macro);
	if (pos != cells.end()) {
		double rotation = 0.0;
		bool x_reflection = false;
		if (inst.dir == vec2i(-1, 1)) {
			rotation = M_PI;
			x_reflection = true;
		} else if (inst.dir == vec2i(1, -1)) {
			x_reflection = true;
		} else if (inst.dir == vec2i(-1, -1)) {
			rotation = M_PI;
		}

		cell.reference_array.append(new gdstk::Reference{
				.type = gdstk::ReferenceType::Cell,
				.cell = pos->second,
				.origin = gdstk::Vec2{(double)inst.pos[0], (double)inst.pos[1]},
				.rotation = rotation,
				.magnification = 1.0,
				.x_reflection = x_reflection,
			});
		return true;
	}
	return false;
}

gdstk::Cell *export_layout(const Layout &layout, const map<int, gdstk::Cell*> *cells) {
	gdstk::Cell *cell = new gdstk::Cell();
	string name = layout.name;
	if (name.empty()) {
		string name = "anon_" + idToString(rand());
	}
	cell->init(name.c_str());
	for (auto layer = layout.layers.begin(); layer != layout.layers.end(); layer++) {
		export_layer(*cell, layer->second, layout);
	}
	if (cells != nullptr) {
		for (auto inst = layout.inst.begin(); inst != layout.inst.end(); inst++) {
			export_instance(*cell, *inst, *cells);
		}
	}
	return cell;
}

void export_layout(gdstk::GdsWriter &writer, const Library &library, int idx, map<int, gdstk::Cell*> &cells) {
	if (cells.find(idx) != cells.end()) {
		return;
	}

	vector<int> stack(1, idx);
	while (not stack.empty()) {
		int curr = stack.back();
		auto currMacro = library.macros.begin()+curr;
		
		bool done = true;
		for (auto i = currMacro->inst.begin(); i != currMacro->inst.end(); i++) {
			if (cells.find(i->macro) == cells.end()) {
				done = false;
				stack.erase(std::remove(stack.begin(), stack.end(), i->macro), stack.end());
				stack.push_back(i->macro);
			}
		}

		if (done) {
			gdstk::Cell *gds = export_layout(library.macros[curr], &cells);
			writer.write_cell(*gds);
			cells.insert({curr, gds});
			stack.pop_back();
		}
	}
}

void export_layout(string filename, const Layout &layout) {
	gdstk::Library lib = {};
	lib.init(layout.name.c_str(), ((double)layout.tech->dbunit)*1e-6, ((double)layout.tech->dbunit)*1e-6);
	lib.cell_array.append(export_layout(layout));
	lib.write_gds(filename.c_str(), 0, NULL);
	lib.free_all();
}

void export_library(gdstk::Library &lib, const Library &library) {
	map<int, gdstk::Cell*> cells;
	vector<bool> hasCell(library.macros.size(), false);

	for (int root = 0; root < (int)library.macros.size(); root++) {
		if (hasCell[root]) {
			continue;
		}

		vector<int> stack(1, root);
		while (not stack.empty()) {
			int curr = stack.back();
			auto currMacro = library.macros.begin()+curr;
			
			bool done = true;
			for (auto i = currMacro->inst.begin(); i != currMacro->inst.end(); i++) {
				if (not hasCell[i->macro]) {
					done = false;
					stack.erase(std::remove(stack.begin(), stack.end(), i->macro), stack.end());
					stack.push_back(i->macro);
				}
			}

			if (done) {
				gdstk::Cell *gds = export_layout(library.macros[curr], &cells);
				lib.cell_array.append(gds);
				cells.insert({curr, gds});
				hasCell[curr] = true;
				stack.pop_back();
			}
		}
	}
}

void export_library(string libname, string filename, const Library &library) {
	gdstk::Library lib = {};
	lib.init(libname.c_str(), ((double)library.tech->dbunit)*1e-6, ((double)library.tech->dbunit)*1e-6);
	export_library(lib, library);
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

	Rect box = layout.box;
	vec2i size = box.ur-box.ll;
	fprintf(fptr, "\tORIGIN %d %d ;\n", -box.ll[0], -box.ll[1]);
	fprintf(fptr, "\tFOREIGN %s %d %d ;\n", layout.name.c_str(), box.ll[0], box.ll[1]);
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

			if (not n->names.empty()) {
				fprintf(fptr, "\tPIN %s\n", n->names[0].c_str());
			} else {
				fprintf(fptr, "\tPIN __%d\n", i);
			}
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
				for (auto rect = layer->second.geo.begin(); rect != layer->second.geo.end(); rect++) {
					if (rect->net == i) {
						if (not found and layer->first >= 0) {
							fprintf(fptr, "\t\t\tLAYER %s ;\n", layout.tech->paint[layer->first].name.c_str());
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
		for (auto rect = layer->second.geo.begin(); rect != layer->second.geo.end(); rect++) {
			if (rect->net < 0 or (
				not layout.nets[rect->net].isInput and
				not layout.nets[rect->net].isOutput and
				not layout.nets[rect->net].isVdd and
				not layout.nets[rect->net].isGND and
				not layout.nets[rect->net].isSub)) {
				if (not found and layer->first >= 0) {
					fprintf(fptr, "\t\tLAYER %s ;\n", layout.tech->paint[layer->first].name.c_str());
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
