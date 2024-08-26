#include "import.h"
#include <limits>

using namespace phy;
using namespace std;

void loadGDS(Layout &layout, const Tech &tech, string path, string cellName) {
	gdstk::Library lib = gdstk::read_gds(path.c_str(), tech.dbunit*1e-6, tech.dbunit*1e-6, nullptr, nullptr);
	gdstk::Cell *gdsCell = lib.get_cell(cellName.c_str());
	if (gdsCell == nullptr) {
		return;
	}

	int polyCount = 0;

	gdstk::Array<gdstk::Polygon*> polys;
	gdsCell->get_polygons(true, true, -1, false, gdstk::Tag{}, polys);

	for (int i = 0; i < (int)polys.count; i++) {
		gdstk::Polygon* poly = polys[i];
		if (poly->point_array.count != 4) {
			polyCount++;
			continue;
		}

		vec2i ll(std::numeric_limits<double>::max(), std::numeric_limits<double>::max());
		vec2i ur(std::numeric_limits<double>::min(), std::numeric_limits<double>::min());
	
		int major = gdstk::get_layer(poly->tag);
		int minor = gdstk::get_type(poly->tag);

		int draw = tech.findPaint(major, minor);
		for (int j = 0; j < (int)poly->point_array.count; j++) {
			gdstk::Vec2 point = poly->point_array[j];
			if (point.x < ll[0]) {
				ll[0] = point.x;
			}
			if (point.x > ur[0]) {
				ur[0] = point.x;
			}
			if (point.y < ll[1]) {
				ll[1] = point.y;
			}
			if (point.y > ur[1]) {
				ur[1] = point.y;
			}			
		}

		layout.push(draw, Rect(-1, ll, ur));
	}
	if (polyCount > 0) {
		printf("found %d polygons, only rectangles are supported\n", polyCount);
	}

	lib.free_all();
}

