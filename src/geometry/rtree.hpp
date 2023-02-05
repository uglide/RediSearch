#pragma once

#define BOOST_ALLOW_DEPRECATED_HEADERS
#include <boost/geometry.hpp>
#undef BOOST_ALLOW_DEPRECATED_HEADERS
#include <algorithm>
#include "allocator.hpp"
#include "rtdoc.hpp"
#include "query_iterator.hpp"
#include "rtree.h"

namespace bg = boost::geometry;
namespace bgi = bg::index;

struct RTree {
	using parameter_type = bgi::quadratic<16>;
	using rtree_internal = bgi::rtree<RTDoc, parameter_type, RTDoc_Indexable, RTDoc_EqualTo, rm_allocator<RTDoc>>;

	rtree_internal rtree_;

  explicit RTree() = default;
  explicit RTree(rtree_internal const& rt) noexcept : rtree_{rt} {}
	
	void insert(const RTDoc& doc) {
		rtree_.insert(doc);
	}

	bool remove(const RTDoc& doc) {
		return rtree_.remove(doc);
	}

	[[nodiscard]] size_t size() const noexcept {
		return rtree_.size();
	}

	[[nodiscard]] bool is_empty() const noexcept {
		return rtree_.empty();
	}

	void clear() noexcept {
		rtree_.clear();
	}

	[[nodiscard]] size_t report() const noexcept {
		return rtree_.get_allocator().report();
	}

	template <typename Predicate>
	[[nodiscard]] QueryIterator::container query(Predicate p) const {
		QueryIterator::container result{};
		rtree_.query(p, std::back_inserter(result));
		return result;
	}

	[[nodiscard]] QueryIterator::container contains(RTDoc const *queryDoc) const {
		auto results = query(bgi::contains(queryDoc->rect_));
		std::erase_if(results, [&](auto const& doc) {
			return !bg::within(queryDoc->poly_, doc.poly_);
		});
		return results;
	}
	[[nodiscard]] QueryIterator::container within(RTDoc const *queryDoc) const {
		auto results = query(bgi::within(queryDoc->rect_));
		std::erase_if(results, [&](auto const& doc) {
			return !bg::within(doc.poly_, queryDoc->poly_);
		});
		return results;
	}

  [[nodiscard]] void* operator new(std::size_t sz) { return rm_malloc(sz); }
  void operator delete(void *p) { rm_free(p); }
};