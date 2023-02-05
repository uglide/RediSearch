#pragma once

#define BOOST_ALLOW_DEPRECATED_HEADERS
#include <boost/geometry.hpp>
#undef BOOST_ALLOW_DEPRECATED_HEADERS
#include "allocator.hpp"

namespace bg = boost::geometry;
namespace bgm = bg::model;

struct Point {
	using coord_system = bg::cs::cartesian;
	using point_internal = bgm::point<double, 2, coord_system>;

	point_internal point_;

	explicit Point(double x, double y) noexcept : point_{x, y} {}
  explicit Point(point_internal const& other) noexcept : point_{other} {}
	
  [[nodiscard]] void* operator new(std::size_t sz) { return rm_malloc(sz); }
  void operator delete(void *p) noexcept { rm_free(p); }
};