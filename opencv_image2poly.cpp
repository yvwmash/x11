/* the app converts raster image polygon contours to a JSON file that contains list of the contours.
 * saves to out0.json
 * adapted from ChatGPT(https://chatgpt.com) conversation.
 *
*/

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/stat.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation-deprecated-sync"
#pragma clang diagnostic ignored "-Wreserved-macro-identifier"
#pragma clang diagnostic ignored "-Wc++98-compat-pedantic"
#pragma clang diagnostic ignored "-Wold-style-cast"

extern "C" {
#include <json-c/json.h>
}

#pragma clang diagnostic pop

#include <cmath>
#include <utility>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"
#pragma clang diagnostic ignored "-Wdeprecated-enum-enum-conversion"
#pragma clang diagnostic ignored "-Wreserved-macro-identifier"
#pragma clang diagnostic ignored "-Wreserved-identifier"
#pragma clang diagnostic ignored "-Wcast-align"
#pragma clang diagnostic ignored "-Wc++98-compat-pedantic"
#pragma clang diagnostic ignored "-Wweak-vtables"
#pragma clang diagnostic ignored "-Wold-style-cast"
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"
#pragma clang diagnostic ignored "-Wextra-semi-stmt"
#pragma clang diagnostic ignored "-Wdeprecated-copy-with-user-provided-dtor"
#pragma clang diagnostic ignored "-Wdeprecated-dynamic-exception-spec"
#pragma clang diagnostic ignored "-Wsuggest-destructor-override"
#pragma clang diagnostic ignored "-Wdouble-promotion"
#pragma clang diagnostic ignored "-Wswitch-default"
#pragma clang diagnostic ignored "-Wswitch-enum"
#pragma clang diagnostic ignored "-Wextra-semi"
#pragma clang diagnostic ignored "-Wdeprecated-anon-enum-enum-conversion"
#pragma clang diagnostic ignored "-Wnewline-eof"
#pragma clang diagnostic ignored "-Wdocumentation-unknown-command" /*  */
#pragma clang diagnostic ignored "-Wfloat-conversion" /* C style casts, all legal */
#pragma clang diagnostic ignored "-Wimplicit-float-conversion"
#pragma clang diagnostic ignored "-Wimplicit-int-conversion"
#pragma clang diagnostic ignored "-Wformat-nonliteral"
#pragma clang diagnostic ignored "-Wexit-time-destructors" /* warning: declaration requires an exit-time destructor => static cv::Mutex mutex; */
#pragma clang diagnostic ignored "-Wundefined-reinterpret-cast" /* yes they do know their trade */
#pragma clang diagnostic ignored "-Wimplicit-int-float-conversion" /* hope they know their trade */
#pragma clang diagnostic ignored "-Winconsistent-missing-destructor-override"
#pragma clang diagnostic ignored "-Wshadow-field" /* thats unfortunate :p */
#pragma clang diagnostic ignored "-Wc11-extensions" /* warning: '_Atomic' is a C11 extension [-Wc11-extensions] */
#pragma clang diagnostic ignored "-Wcast-qual" /* drop constness, perfectly OK for C style */
#pragma clang diagnostic ignored "-Wpadded" /* struct padding, OK */
#pragma clang diagnostic ignored "-Wfloat-equal" /* all case are safe to a degree. still better use epsilons */
#pragma clang diagnostic ignored "-Wsign-conversion" /* no explicit conversion */
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage" /* hundredth of it. isn't the "p + off" is a normal way to access memory in C/C++? */

#include <opencv2/opencv.hpp>

#pragma clang diagnostic pop

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

	int r = polygon_winding<double>(vscaled.data(), vscaled.size());
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
static unsigned pnpoly(size_t nvert, pt2d *va, pt2d pt)
{
  size_t    i, j;
  unsigned  c = 0;

  for (i = 0, j = nvert - 1; i < nvert; j = i++) {
    if ( ((va[i].y > pt.y) != (va[j].y > pt.y))
         && (pt.x < (va[j].x - va[i].x) * (pt.y - va[i].y) / (va[j].y - va[i].y) + va[i].x)
       )
    {
       c = !c;
    }
  }
  return (unsigned)c;
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
  unsigned  save_i = UINT_MAX;
  unsigned  f      = 0;

  for(unsigned ii = 0; ii < vnp.size(); ++ii) {
   size_t  n = vnp[ii].size();

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

  if(save_i == UINT_MAX) {
   vout.push_back(vp[i]);
  }

 }

 vp = std::move(vout);

 return r;
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-template"
/*  */
static void print_contours(auto &va) {
	for (size_t i = 0; i < va.size(); ++i) {
		printf("c[%lu]:\n", i);
		for (auto &v : va[i]) {
			printf("\t{%d,%d}\n", v.x, v.y);
		}
	}
}
#pragma clang diagnostic pop

/* ================================================================================================================== */

#define USAGE { fprintf(stderr, "usage: imgc2poly [-nd] <image_path>\n"); return 0; }

int main(int argc, char** argv) {
	const char *fn              = NULL;
	bool        bdisplay        = true;
	bool        b_inconsistency = false; /* polygon contours same level. some vertices as "in" and some as "out" */

	if(1 == argc) {
		USAGE
    }else if(2 == argc) {
		fn = argv[1];
    }else if(3 == argc) {
		fn       = argv[2];
		bdisplay = false;

        if(0 != strcmp("-nd", argv[1])) {
			USAGE
        }
    }else {
		USAGE
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

	int ret = 0;

    /* write the JSON data to a file */
    {
     int         fd = open("./out/out0.json", O_CLOEXEC|O_CREAT|O_WRONLY|O_TRUNC, S_IRUSR|S_IWUSR);
	 size_t      l, ntotal = 0;
	 ssize_t     nwritten = 0;
     const char *s;

	 if(fd < 0) {
      fprintf(stderr, " * write JSON data: %s\n\t%s\n", "out0.json", strerror(errno));
      return 2;
     }

	 s = json_object_to_json_string_ext(json_polygons, JSON_C_TO_STRING_PRETTY);
     l = strlen(s);
     do {
      nwritten = write(fd, s + ntotal, l);
      if( nwritten < 0 ) {
       fprintf(stderr, " * img2poly: %s:%s:%d\n", __FILE__, __func__, __LINE__);
       perror(" * write: ");
       ret = 1;
       break;
      }
      ntotal += (size_t)nwritten;
      l      -= (size_t)nwritten;
     } while(l > 0);
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

	if ( b_inconsistency ) { /* polygon contours same level. some vertices as "in" and some as "out" */
		ret = RET_POLYGONS_INCONSISTENT;
	}

    return ret;
}
