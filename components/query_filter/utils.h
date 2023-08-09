/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_QUERY_FILTER_UTILS_H_
#define BRAVE_COMPONENTS_QUERY_FILTER_UTILS_H_

#include <string>
#include "third_party/abseil-cpp/absl/types/optional.h"

class GURL;

absl::optional<GURL> ApplyQueryFilter(const GURL& original_url);
absl::optional<GURL> ApplyPotentialQueryStringFilter(const GURL& source,
                                                     const GURL& destination,
                                                     const std::string& method);

#endif  // BRAVE_COMPONENTS_QUERY_FILTER_UTILS_H_
