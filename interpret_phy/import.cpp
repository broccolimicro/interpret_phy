#include "import.h"
#include <limits>

using namespace std;

namespace phy {

bool import_layout(Layout &layout, const gdstk::Library &lib, string cellName) {
	bool success = true;

	gdstk::Cell *gdsCell = lib.get_cell(cellName.c_str());
	if (gdsCell == nullptr) {
		return false;
	}

	layout.name = cellName;

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
		string txt = label->text;
		
		int net = (int)layout.nets.size();
		for (int i = 0; i < (int)layout.nets.size(); i++) {
			if (layout.nets[i].name == txt) {
				net = i;
				break;
			}
		}
		if (net >= (int)layout.nets.size()) {
			layout.nets.push_back(txt);
		}

		int major = gdstk::get_layer(label->tag);
		int minor = gdstk::get_type(label->tag);

		int draw = layout.tech->findPaint(major, minor);

		bool found = false;
		for (auto layer = layout.layers.begin(); layer != layout.layers.end() and not found; layer++) {
			if (layer->draw == draw or layer->pin == draw or layer->label == draw) {
				for (auto rect = layer->geo.begin(); rect != layer->geo.end(); rect++) {
					vec2i cmp = rect->ll+rect->ur/2;
					if (origin[0] >= rect->ll[0] and origin[1] >= rect->ll[1]
						and origin[0] <= rect->ur[0] and origin[1] <= rect->ur[1]) {
						rect->net = net;
						found = true;
						break;
					}
				}
			}
		}
	}

	return success;
}

bool import_layout(Layout &layout, string path, string cellName) {
	gdstk::Library lib = gdstk::read_gds(path.c_str(), ((double)layout.tech->dbunit)*1e-6, ((double)layout.tech->dbunit)*1e-6, nullptr, nullptr);
	bool result = import_layout(layout, lib, cellName);
	lib.free_all();
	return result;
}

}
