//
// Copyright (c) 2012-2015 Krzysztof Jusiak (krzysztof at jusiak dot net)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "boost/di/core/array.hpp"
#include <vector>
#include <memory>

namespace core {

test array_empty_ctor = [] { array<std::vector<int>()>{}; };

test array_ctor = [] {
  array<std::vector<int>(), int, int> a{42, 87};
  expect(a.size() == 2);
  expect(a[0] == 42);
  expect(a[1] == 87);
};

test array_ctor_smart_ptr = [] {
  array<std::vector<std::unique_ptr<int>>(), int, int> a{std::make_unique<int>(42), std::make_unique<int>(87)};
  expect(a.size() == 2);
  expect(*a[0] == 42);
  expect(*a[1] == 87);
};

test array_ctor_named = [] {
  array<std::vector<std::unique_ptr<int>>(), named<int>, int> a{std::make_unique<int>(42), std::make_unique<int>(87)};
  expect(a.size() == 2);
  expect(*a[0] == 42);
  expect(*a[1] == 87);
};

}  // core
