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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-enum-enum-conversion"

#include <opencv2/opencv.hpp>

#pragma GCC diagnostic pop

// Function to compute the Euclidean distance between two points
static double distance(const cv::Point& p1, const cv::Point& p2) {
    return std::sqrt(std::pow(p2.x - p1.x, 2) + std::pow(p2.y - p1.y, 2));
}

// Function to remove close vertices from a contour
static std::vector<cv::Point> remove_close_vertices(const std::vector<cv::Point>& contour, double min_distance) {
    std::vector<cv::Point> filtered_contour;
    if (contour.empty()) return filtered_contour;

    filtered_contour.push_back(contour[0]); // Always keep the first point

    // Iterate through the contour and compare the distance between consecutive points
    for (size_t i = 1; i < contour.size(); ++i) {
        bool is_too_close = false;
        for (const auto& point : filtered_contour) {
            if (distance(contour[i], point) < min_distance) {
                is_too_close = true;
                break;
            }
        }
        if (!is_too_close) {
            filtered_contour.push_back(contour[i]);
        }
    }

    return filtered_contour;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: ./imgc2poly <image_path>\n");
        return 0;
    }

    // Read the input image
    cv::Mat image = cv::imread(argv[1]);

    if (image.empty()) {
        fprintf(stderr, " * could not open the image %s\n", argv[1]);
        return 1;
    }

    // Convert to grayscale
    cv::Mat gray;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);

    // Apply a Gaussian blur to reduce noise
    cv::Mat blurred;
    cv::GaussianBlur(gray, blurred, cv::Size(5, 5), 1.5);

    // Perform Canny edge detection
    cv::Mat edges;
    cv::Canny(blurred, edges, 100, 200); // lower and upper thresholds

    // Find contours in the edge-detected image
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(edges, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    // Define the minimum distance between consecutive vertices (in pixels)
    double min_distance = 10.0; // Adjust this value based on your needs

    // Create a JSON object to store the contours
    json_object* json_polygons = json_object_new_array();

    // Iterate over all the contours
    for (size_t i = 0; i < contours.size(); ++i) {
        // Approximate the contour to a polygon
        double epsilon = 0.01 * cv::arcLength(contours[i], true); // Adjust epsilon for accuracy
        std::vector<cv::Point> approx;
        cv::approxPolyDP(contours[i], approx, epsilon, true);

        // Remove close vertices
        std::vector<cv::Point> filtered_approx = remove_close_vertices(approx, min_distance);

        // Convert filtered contour to JSON format
        json_object* polygon = json_object_new_array();
		int          w       = image.cols;
		int          h       = image.rows;
        for (const auto& pt : filtered_approx) {
			double x = 2.0 * (pt.x - 0.0) / (w - 0.0) - 1.0; /* map to -1, 1 range */
			double y = 2.0 * (pt.y - 0.0) / (h - 0.0) - 1.0; /* map to -1, 1 range */

            json_object* point = json_object_new_object();
            json_object_object_add(point, "x", json_object_new_double(x));
            json_object_object_add(point, "y", json_object_new_double(y));
            json_object_array_add(polygon, point);
        }

        // Add the polygon to the JSON array
        json_object_array_add(json_polygons, polygon);
    }

    // Write the JSON data to a file
    {
     int         fd = open("./out/out0.json", O_CLOEXEC|O_CREAT|O_WRONLY, S_IRUSR|S_IWUSR);
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

    return 0;
}
