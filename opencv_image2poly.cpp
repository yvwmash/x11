/* the app converts raster image polygon contours to a JSON file that contains list of the contours.
 * saves to out0.json
 * adapted from ChatGPT(https://chatgpt.com) conversation.
 *
*/

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

extern "C" {
#include <json-c/json.h>
}

#include <cmath>
#include <utility>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-enum-enum-conversion"

#include <opencv2/opencv.hpp>

#pragma GCC diagnostic pop

#include "vg_algebra/geometry.h"

// Function to compute the Euclidean distance between two points
static double distance(const cv::Point& p1, const cv::Point& p2) {
    return std::sqrt(std::pow(p2.x - p1.x, 2) + std::pow(p2.y - p1.y, 2));
}

/* remove close vertices from a polygon */
static std::vector<cv::Point> remove_close_vertices(const std::vector<cv::Point>& va, double min_distance) {
    std::vector<cv::Point> filtered_vertices;
    if (va.empty()) return filtered_vertices;

    filtered_vertices.push_back(va[0]); // Always keep the first point

    /* iterate through the vertices of the polygon and compare the distance between consecutive points */
    for (size_t i = 1; i < va.size(); ++i) {
        bool is_too_close = false;
        for (const auto& point : filtered_vertices) {
            if (distance(va[i], point) < min_distance) {
                is_too_close = true;
                break;
            }
        }
        if (!is_too_close) {
            filtered_vertices.push_back(va[i]);
        }
    }

    return filtered_vertices;
}

/* polygon winding order */
static int polygon_winding_order(cv::Mat &image, std::vector<cv::Point> &va)
{
	std::vector<pt2<double>> vscaled;
	int          w       = image.cols;
	int          h       = image.rows;

	vscaled.reserve(va.size());

	for(auto &v : va) {
		double x = 2.0 * (v.x - 0.0) / (w - 0.0) - 1.0; /* map to -1, 1 range */
		double y = 2.0 * (v.y - 0.0) / (h - 0.0) - 1.0; /* map to -1, 1 range */
		vscaled.push_back( {x, y} );
	}

	int r = polygon_winding(vscaled.data(), vscaled.size());
	const char *vw[] = {
	"POLY_WINDING_CCW",
	"POLY_WINDING_CW",
	};
	const char *ps = NULL;
	if(POLY_WINDING_CCW == r) {
		ps = vw[0];
	} else {
		ps = vw[1];
	}
	printf(" ! polygon winding is: %s\n", ps);
	printf(" ! \tpolygon winding is compatible to OpenGL: %d\n", POLY_WINDING_CCW == r);

	return r;
}

/* */
static int pnpoly(int nvert, pt2d *va, pt2d pt)
{
  int i, j, c = 0;

  for (i = 0, j = nvert - 1; i < nvert; j = i++) {
    if ( ((va[i].y > pt.y) != (va[j].y > pt.y))
         && (pt.x < (va[j].x - va[i].x) * (pt.y - va[i].y) / (va[j].y - va[i].y) + va[i].x)
       )
    {
       c = !c;
    }
  }
  return c;
}

/* */
static bool remove_inner_contours(int w, int h, std::vector<std::vector<cv::Point>> &vp) {
 bool                                 r = true;
 std::vector<std::vector<cv::Point>>  vout;

 /* vp => normilized points */
 std::vector<std::vector<pt2d>> vnp;
 for(auto &c : vp) {
  std::vector<pt2d> va;

  for(auto &p : c) {
   double x = 2.0 * (p.x - 0.0) / (w - 0.0) - 1.0; /* map to -1, 1 range */
   double y = 2.0 * (p.y - 0.0) / (h - 0.0) - 1.0; /* map to -1, 1 range */

   va.emplace_back( x, y );
  }

  vnp.push_back( va );
 }

 for(unsigned i = 0; i < vnp.size(); ++i) {
  int       save_i = -1;
  unsigned  f      = 0;
  size_t    n      = vnp[i].size();

  for(unsigned ii = 0; ii < vnp.size(); ++ii) {
   if(i == ii) {
    continue;
   }

   f = 0;
   for(unsigned vi = 0; vi < vnp[i].size(); vi += 1) {
    f += pnpoly(n, vnp[ii].data(), vnp[i][vi]);
   }
   if((0 < f) && (f < n)) {
    r = false;
   }else if(f == n) {
    save_i = i;
   }

  }

  if(save_i < 0) {
   vout.push_back(vp[i]);
  }

 }

 vp = std::move(vout);

 return r;
}

/*  */
static void print_contours(auto &va) {
	for (size_t i = 0; i < va.size(); ++i) {
		printf("c[%lu]:\n", i);
		for (auto &v : va[i]) {
			printf("\t{%d,%d}\n", v.x, v.y);
		}
	}
}

/* ================================================================================================================== */

#define USAGE { fprintf(stderr, "usage: imgc2poly [-nd] <image_path>\n"); return 0; }

#define RET_POLYGONS_INCONSISTENT 1

int main(int argc, char** argv) {
	const char *fn              = NULL;
	bool        bdisplay        = true;
	bool        b_inconsistency = false; /* polygon contours same level. some vertices as "in" and some as "out" */

	if(1 == argc) {
		USAGE;
    }else if(2 == argc) {
		fn = argv[1];
    }else if(3 == argc) {
		fn       = argv[2];
		bdisplay = false;

        if(0 != strcmp("-nd", argv[1])) {
			USAGE;
        }
    }else {
		USAGE;
	}

    // Read the input image
    cv::Mat image = cv::imread(fn);
    if (image.empty()) {
        fprintf(stderr, " * could not open the image %s\n", fn);
        return 1;
    }

    // Convert to grayscale
    cv::Mat gray;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);

	const unsigned dilation_size = 2;
	cv::Mat element = getStructuringElement( cv::MORPH_RECT, cv::Size( 2 * dilation_size + 1, 2 * dilation_size + 1 ), cv::Point( dilation_size, dilation_size ) );

	cv::Mat dilated;
	cv::dilate(gray, dilated, element);

    // Perform Canny edge detection
    cv::Mat edges;
    cv::Canny(gray, edges, 100, 300, 3, true);

    // Find contours in the edge-detected image
    std::vector<std::vector<cv::Point>>  contours;
    std::vector<std::vector<cv::Point>>  outer_contours;
    std::vector<cv::Vec4i>               hierarchy;

    cv::findContours(edges, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    /* separate outermost contours */
    for (size_t i = 0; i < contours.size(); ++i) {
        if (hierarchy[i][3] == -1) { // no parent = outermost contour
            outer_contours.push_back(contours[i]);
        }
    }

	/* store verices of contours */
    const double min_distance = 10.0;
	std::vector<std::vector<cv::Point>> vp;
	{
	 for (size_t i = 0; i < outer_contours.size(); ++i) {
        double                  epsilon = 0.005 * cv::arcLength(outer_contours[i], true); /* adjust epsilon for accuracy */
        std::vector<cv::Point>  approx;
		std::vector<cv::Point>  filtered_close_approx;

        /* approximate the contour to a polygon */
        cv::approxPolyDP(outer_contours[i], approx, epsilon, true);

        /* remove close vertices */
        filtered_close_approx = remove_close_vertices(approx, min_distance);

		if(filtered_close_approx.size() <= 2) {
			continue;
		}

		vp.emplace_back(filtered_close_approx);
	 }
	}

	/* filter out inner contours */
	{
		int  w = image.cols;
		int  h = image.rows;

		b_inconsistency = !remove_inner_contours(w, h, vp);
	}

    /* create a JSON object to store the contours */
    json_object* json_polygons = json_object_new_array();
    /* save to JSON */
	{
		int          w       = image.cols;
		int          h       = image.rows;
		json_object *polygon;
		auto         fn_store_v = [&polygon, &w, &h](const cv::Point& pt) -> void {
			double       x     = 2.0 * (pt.x - 0.0) / (w - 0.0) - 1.0; /* map to -1, 1 range */
			double       y     = 2.0 * (pt.y - 0.0) / (h - 0.0) - 1.0; /* map to -1, 1 range */
			json_object *point = json_object_new_object();

			json_object_object_add(point, "x", json_object_new_double(x));
			json_object_object_add(point, "y", json_object_new_double(y));
			json_object_array_add(polygon, point);
		};

		for (size_t i = 0; i < vp.size(); ++i) {
			(void)polygon_winding_order;

			/* contour to JSON format */
			polygon = json_object_new_array();
			for (const auto& pt : vp[i]) {
				fn_store_v(pt);
			}
			fn_store_v(vp[i].front()); /* close polygon */
			json_object_array_add(json_polygons, polygon);
		}
	}

    /* write the JSON data to a file */
    {
     int         fd = open("./out/out0.json", O_CLOEXEC|O_CREAT|O_WRONLY|O_TRUNC, S_IRUSR|S_IWUSR);
	 size_t      l;
     const char *s;

	 if(fd < 0) {
      fprintf(stderr, " * write JSON data: %s\n\t%s\n", "out0.json", strerror(errno));
      return 2;
     }

	 s = json_object_to_json_string_ext(json_polygons, JSON_C_TO_STRING_PRETTY);
     l = strlen(s);
     write(fd, s, l);
     close(fd);

     // Free JSON objects
     json_object_put(json_polygons);
    }

    /* display contours and vertices */
	if(bdisplay) {
		cv::Scalar red = { 0, 0, 150};

		for (size_t i = 0; i < vp.size(); ++i) {
			for (auto &v : vp[i]) {
				cv::circle(image, v, 5, red, -1);
			}
		}

		// Draw the original contour
		cv::drawContours(image, vp, -1, cv::Scalar(0, 0, 0), 2);
		// Draw circles on the vertices of the polygon
		cv::imshow("Contour Approximation", image);
		while (true) {
			int k = cv::waitKey(0);
			if (k) {
				break;
			}
		}
	}

#define RET_POLYGONS_INCONSISTENT 1

	int ret = 0;

	if ( b_inconsistency ) { /* polygon contours same level. some vertices as "in" and some as "out" */
		ret = RET_POLYGONS_INCONSISTENT;
	}

    return ret;
}
