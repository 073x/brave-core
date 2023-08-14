// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/api/query_filter/utils.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "url/gurl.h"

TEST(BraveQueryFilter, FilterQueryTrackers) {
  EXPECT_EQ(MaybeApplyQueryStringFilter(
                GURL("https://brave.com"), GURL("https://brave.com"),
                GURL("https://test.com/?gclid=123"), "GET", false),
            GURL("https://test.com/"));
  EXPECT_EQ(MaybeApplyQueryStringFilter(
                GURL("https://brave.com"), GURL("https://brave.com"),
                GURL("https://test.com/?fbclid=123"), "GET", false),
            GURL("https://test.com/"));
  EXPECT_EQ(MaybeApplyQueryStringFilter(
                GURL("https://brave.com"), GURL("https://brave.com"),
                GURL("https://test.com/?mkt_tok=123"), "GET", false),
            GURL("https://test.com/"));
  EXPECT_EQ(
      MaybeApplyQueryStringFilter(
          GURL("https://brave.com"), GURL("https://brave.com"),
          GURL("https://test.com/?gclid=123&unsubscribe=123"), "GET", false),
      GURL("https://test.com/?unsubscribe=123"));
  EXPECT_EQ(
      MaybeApplyQueryStringFilter(
          GURL("https://brave.com"), GURL("https://brave.com"),
          GURL("https://test.com/?gclid=123&Unsubscribe=123"), "GET", false),
      GURL("https://test.com/?Unsubscribe=123"));
  EXPECT_FALSE(MaybeApplyQueryStringFilter(
      GURL("https://brave.com"), GURL("https://brave.com"),
      GURL("https://test.com/"), "GET", false));
  EXPECT_FALSE(MaybeApplyQueryStringFilter(GURL("https://brave.com"),
                                           GURL("https://brave.com"), GURL(),
                                           "GET", false));
}
