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

	int polyCount = 0;

	gdstk::Array<gdstk::Polygon*> polys{0,0,nullptr};
	gdsCell->get_polygons(true, true, -1, false, gdstk::Tag{}, polys);
	for (int i = 0; i < (int)polys.count; i++) {
		gdstk::Polygon* poly = polys[i];
		if (poly->point_array.count != 4) {
			polyCount++;
			continue;
		}

		vec2i ll(std::numeric_limits<int>::max(), std::numeric_limits<int>::max());
		vec2i ur(std::numeric_limits<int>::min(), std::numeric_limits<int>::min());
	
		int major = gdstk::get_layer(poly->tag);
		int minor = gdstk::get_type(poly->tag);

		int draw = layout.tech->findPaint(major, minor);
		for (int j = 0; j < (int)poly->point_array.count; j++) {
			gdstk::Vec2 point = poly->point_array[j];
			if ((int)point.x < ll[0]) {
				ll[0] = (int)point.x;
			}
			if ((int)point.x > ur[0]) {
				ur[0] = (int)point.x;
			}
			if ((int)point.y < ll[1]) {
				ll[1] = (int)point.y;
			}
			if ((int)point.y > ur[1]) {
				ur[1] = (int)point.y;
			}
		}

		layout.push(draw, Rect(-1, ll, ur));
	}
	if (polyCount > 0) {
		printf("found %d polygons, only rectangles are supported\n", polyCount);
		success = false;
	}

	gdstk::Array<gdstk::Label*> labels{0,0,nullptr};
	gdsCell->get_labels(true, -1, false, gdstk::Tag{}, labels);
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
