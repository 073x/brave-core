// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_QUERY_FILTER_UTILS_H_
#define BRAVE_COMPONENTS_QUERY_FILTER_UTILS_H_

#include <string>

#include "third_party/abseil-cpp/absl/types/optional.h"
#include "url/gurl.h"

namespace query_filter {
absl::optional<GURL> ApplyQueryFilter(const GURL& original_url);

/**
 * This function will return a new url stripping any tracking query params.
 * If nothing is to be stripped, a null value is returned.
 *
 * @param initiator_url Indicates the origin initiating the resource request.
 * It's the first url in a redirect chain
 * @param redirect_source_url Indicates the we are currently navigating from.
 * @param request_url Indicates where we are navigating to.
 * @param request_method The HTTP method of the request.
 * @param internal_redirect Indicating if this is an internal redirect or not.
 * @return The url we should redirect to.
 */
absl::optional<GURL> MaybeApplyQueryStringFilter(
    const GURL& initiator_url,
    const GURL& redirect_source_url,
    const GURL& request_url,
    const std::string& request_method,
    const bool internal_redirect);
}  // namespace query_filter
#endif  // BRAVE_COMPONENTS_QUERY_FILTER_UTILS_H_
