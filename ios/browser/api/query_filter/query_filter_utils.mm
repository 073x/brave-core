// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/api/query_filter/query_filter_utils.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/components/query_filter/utils.h"
#import "net/base/mac/url_conversions.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// MARK: - Implementation

@implementation NSURL (QueryFilterUtilities)

- (nullable NSURL*)
    brave_stripTrackerParamsUsingInitiatorURL:(/*nullable*/ NSURL*)initiatorURL
                            redirectSourceURL:
                                (/*nullable*/ NSURL*)redirectSourceURL
                                requestMethod:(NSString*)requestMethod
                           isInternalRedirect:(BOOL)isInternalRedirect {
  // Create new fake gurls because thats what
  // `query_filter::MaybeApplyQueryStringFilter` expects
  GURL initiator_url;
  GURL redirect_source_url;

  // Set the actual initiator and redirect source urls if we have them
  if (initiatorURL != nil) {
    initiator_url = net::GURLWithNSURL(initiatorURL);
  }
  if (redirectSourceURL != nil) {
    redirect_source_url = net::GURLWithNSURL(redirectSourceURL);
  }

  GURL request_url = net::GURLWithNSURL(self);
  const auto request_method = base::SysNSStringToUTF8(requestMethod);

  auto filtered_url = query_filter::MaybeApplyQueryStringFilter(
      initiator_url, redirect_source_url, request_url, request_method,
      isInternalRedirect);

  if (filtered_url.has_value()) {
    return net::NSURLWithGURL(filtered_url.value());
  } else {
    return nullptr;
  }
}

@end
