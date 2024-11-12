#include "import.h"
#include <limits>
#include <vector>
#include <algorithm>

using namespace std;

namespace phy {

string import_name(string name) {
	// Do name mangling
	static map<char, char> replace = {
		{'_', '_'},
		{'0', '.'},
		{'1', '['},
		{'2', ']'},
		{'3', '\''},
		{'4', '('},
		{'5', ')'},
		{'6', '<'},
		{'7', '>'},
	};

	string result;
	bool escaped = false;
	for (auto c = name.begin(); c != name.end(); c++) {
		if (escaped) {
			auto pos = replace.find(*c);
			if (pos != replace.end()) {
				result += pos->second;
			} else {
				result.push_back(*c);
			}
			escaped = false;
		} else if (*c == '_') {
			escaped = true;
		} else {
			result.push_back(*c);
		}
	}
	return result;
}

bool import_layout(Layout &layout, const gdstk::Cell *gdsCell) {
	bool success = true;
	layout.name = gdsCell->name;

	gdstk::Array<gdstk::Polygon*> polys{0,0,nullptr};
	gdsCell->get_polygons(true, true, 0, false, gdstk::Tag{}, polys);
	for (int i = 0; i < (int)polys.count; i++) {
		gdstk::Polygon* poly = polys[i];

		int major = gdstk::get_layer(poly->tag);
		int minor = gdstk::get_type(poly->tag);
		int draw = layout.tech->findPaint(major, minor);
		if (draw < 0) {
			printf("warning: unrecognized gds layer %d/%d\n", major, minor);
			continue;
		}

		Poly p;
		for (int j = 0; j < (int)poly->point_array.count; j++) {
			gdstk::Vec2 point = poly->point_array[j];
			p.v.push_back(vec2i((int)point.x, (int)point.y));
		}
		p.normalize();

		layout.push(draw, p);
	}

	gdstk::Array<gdstk::Label*> labels{0,0,nullptr};
	gdsCell->get_labels(true, 0, false, gdstk::Tag{}, labels);
	for (int i = 0; i < (int)labels.count; i++) {
		gdstk::Label* label = labels[i];

		gdstk::Vec2 point = label->origin;
		vec2i origin((int)point.x, (int)point.y);
		string txt = import_name(label->text);
		
		int major = gdstk::get_layer(label->tag);
		int minor = gdstk::get_type(label->tag);

		int layer = layout.tech->findPaint(major, minor);
		layout.label(layer, Label(-1, origin, txt)); 
	}

	layout.normalize();
	return success;
}


bool import_layout(Layout &layout, const gdstk::Library &lib, string cellName) {
	gdstk::Cell *gdsCell = lib.get_cell(cellName.c_str());
	if (gdsCell == nullptr) {
		return false;
	}

	return import_layout(layout, gdsCell);
}

bool import_layout(Layout &layout, string path, string cellName) {
	gdstk::Library lib = gdstk::read_gds(path.c_str(), ((double)layout.tech->dbunit)*1e-6, ((double)layout.tech->dbunit)*1e-6, nullptr, nullptr);
	bool result = import_layout(layout, lib, cellName);
	lib.free_all();
	return result;
}

bool import_library(phy::Library &lib, string path) {
	gdstk::Library gds = gdstk::read_gds(path.c_str(), ((double)lib.tech->dbunit)*1e-6, ((double)lib.tech->dbunit)*1e-6, nullptr, nullptr);
	bool result = true;
	lib.macros.resize(gds.cell_array.count, Layout(*lib.tech));
	for (int i = 0; i < (int)gds.cell_array.count; i++) {
		result = import_layout(lib.macros[i], gds.cell_array[i]) and result;
	}

	gds.free_all();
	return result;
}

}
