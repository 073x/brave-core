/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/query_filter/query_filter_service.h"

#include <string>
#include "base/strings/sys_string_conversions.h"
#include "brave/components/query_filter/utils.h"
#import "net/base/mac/url_conversions.h"
#include "url/gurl.h"

@implementation QueryFilterService

+ (nullable NSURL*)stripQueryParamsFromRequestURL:(NSURL*)requestURL
                            initiatorURL:(NSURL*)initiatorURL
                       redirectSourceURL:(NSURL*)redirectSourceURL
                           requestMethod:(NSString*)requestMethod
                      isInternalRedirect:(BOOL)isInternalRedirect {
  GURL initiator_gurl = net::GURLWithNSURL(initiatorURL);
  GURL redirect_source_gurl = net::GURLWithNSURL(redirectSourceURL);
  GURL request_gurl = net::GURLWithNSURL(requestURL);
  const auto request_method = base::SysNSStringToUTF8(requestMethod);

  auto filtered_url = query_filter::MaybeApplyQueryStringFilter(
      initiator_gurl, redirect_source_gurl, request_gurl, request_method,
      isInternalRedirect);

  if (filtered_url.has_value()) {
    return net::NSURLWithGURL(filtered_url.value());
  } else {
    return nullptr;
  }
}

@end
