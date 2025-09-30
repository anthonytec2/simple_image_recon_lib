#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "simple_image_recon_lib/simple_image_reconstructor.hpp"

namespace py = pybind11;
using namespace simple_image_recon_lib;

PYBIND11_MODULE(simple_image_recon, m)
{
  m.doc() = "Python bindings for SimpleImageReconstructor - event camera image reconstruction";

  py::class_<SimpleImageReconstructor>(
    m, "SimpleImageReconstructor",
    "Reconstructs images from event camera data using temporal and spatial filtering")
    .def(py::init<>(), "Create a new SimpleImageReconstructor instance")

    .def(
      "event", &SimpleImageReconstructor::event, py::arg("t"), py::arg("ex"), py::arg("ey"),
      py::arg("polarity"),
      R"doc(
             Process a single event from the event camera.
             
             Args:
                 t: Event timestamp (microseconds)
                 ex: Event x-coordinate
                 ey: Event y-coordinate  
                 polarity: Event polarity (0 for negative, 1 for positive)
             )doc")

    .def(
      "process_events",
      [](
        SimpleImageReconstructor & self, py::array_t<uint32_t> timestamps,
        py::array_t<uint16_t> x_coords, py::array_t<uint16_t> y_coords,
        py::array_t<uint8_t> polarities) {
        auto t_buf = timestamps.request();
        auto x_buf = x_coords.request();
        auto y_buf = y_coords.request();
        auto p_buf = polarities.request();

        if (t_buf.ndim != 1 || x_buf.ndim != 1 || y_buf.ndim != 1 || p_buf.ndim != 1) {
          throw std::runtime_error("All arrays must be 1-dimensional");
        }

        size_t n = t_buf.shape[0];
        if (x_buf.shape[0] != n || y_buf.shape[0] != n || p_buf.shape[0] != n) {
          throw std::runtime_error("All arrays must have the same length");
        }

        auto * t_ptr = static_cast<uint32_t *>(t_buf.ptr);
        auto * x_ptr = static_cast<uint16_t *>(x_buf.ptr);
        auto * y_ptr = static_cast<uint16_t *>(y_buf.ptr);
        auto * p_ptr = static_cast<uint8_t *>(p_buf.ptr);

        // Release GIL for performance
        py::gil_scoped_release release;

        for (size_t i = 0; i < n; ++i) {
          self.event(t_ptr[i], x_ptr[i], y_ptr[i], p_ptr[i]);
        }
      },
      py::arg("timestamps"), py::arg("x"), py::arg("y"), py::arg("polarity"),
      R"doc(
        Process multiple events in batch (much faster than calling event() in a loop).
        
        Args:
            timestamps: numpy array of timestamps (uint32)
            x: numpy array of x-coordinates (uint16)
            y: numpy array of y-coordinates (uint16)
            polarity: numpy array of polarities (uint8, 0 or 1)
            
        All arrays must have the same length.
        )doc")

    .def(
      "initialize", &SimpleImageReconstructor::initialize, py::arg("width"), py::arg("height"),
      py::arg("cutoff_time"), py::arg("tile_size"), py::arg("fill_ratio"),
      R"doc(
             Initialize the reconstructor with image dimensions and parameters.
             
             Args:
                 width: Image width in pixels
                 height: Image height in pixels
                 cutoff_time: Temporal filter cutoff time (microseconds)
                 tile_size: Size of tiles for activity detection
                 fill_ratio: Target fill ratio for event window adaptation
             )doc")

    .def(
      "get_image",
      [](const SimpleImageReconstructor & self) {
        size_t width = self.getWidth();
        size_t height = self.getHeight();
        py::array_t<uint8_t> img({height, width});
        auto buf = img.request();
        self.getImage(static_cast<uint8_t *>(buf.ptr), width);
        return img;
      },
      R"doc(
           Get the reconstructed image as a numpy array.
           
           Returns:
               numpy.ndarray: Grayscale image of shape (height, width) with dtype uint8
           )doc")

    .def("get_width", &SimpleImageReconstructor::getWidth, "Get the image width")

    .def("get_height", &SimpleImageReconstructor::getHeight, "Get the image height")

    .def(
      "get_event_window_size", &SimpleImageReconstructor::getEventWindowSize,
      "Get the current event window size (number of events buffered)")

    // Python properties for more Pythonic access
    .def_property_readonly("width", &SimpleImageReconstructor::getWidth, "Image width in pixels")
    .def_property_readonly("height", &SimpleImageReconstructor::getHeight, "Image height in pixels")
    .def_property_readonly(
      "event_window_size", &SimpleImageReconstructor::getEventWindowSize,
      "Current event window size");

  // Note: GAUSSIAN_3x3 and GAUSSIAN_5x5 are static constexpr and used internally
  // They don't need to be exposed to Python

  m.attr("__version__") = "1.0.0";
}