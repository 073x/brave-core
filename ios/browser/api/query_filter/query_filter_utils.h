// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_API_QUERY_FILTER_QUERY_FILTER_UTILS_H_
#define BRAVE_IOS_BROWSER_API_QUERY_FILTER_QUERY_FILTER_UTILS_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
@interface NSURL (QueryFilterUtilities)

/**
 This function will return a new url stripping any tracking query params.
 If nothing is to be stripped, a null value is returned.

 @param requestURL Indicates where we are navigating to.
 @param initiatorURL Indicates the origin initiating the resource request. It's
 the first url in a redirect chain.
 @param redirectSourceURL Indicates the we are currently navigating from. It's
 the last url in a redirect chain
 @param method The HTTP method of the request.
 @param isInternalRedirect Indicating if this is an internal redirect or not.
 @return The url we should redirect to.
 */
- (nullable NSURL*)
    brave_stripTrackerParamsUsingInitiatorURL:(nullable NSURL*)initiatorURL
                            redirectSourceURL:(nullable NSURL*)redirectSourceURL
                                requestMethod:(NSString*)requestMethod
                           isInternalRedirect:(BOOL)isInternalRedirect
    NS_SWIFT_NAME(stripTrackerParams(initiatorURL:redirectSourceURL:requestMethod:isInternalRedirect:));  // NOLINT

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_QUERY_FILTER_QUERY_FILTER_UTILS_H_
