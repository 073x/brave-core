/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/permission_rules/search_result_ads/search_result_ad_permission_rules.h"

#include "brave/components/brave_ads/core/internal/account/account_util.h"
#include "brave/components/brave_ads/core/internal/ads/serving/permission_rules/permission_rule_util.h"
#include "brave/components/brave_ads/core/internal/ads/serving/permission_rules/search_result_ads/search_result_ads_per_day_permission_rule.h"
#include "brave/components/brave_ads/core/internal/ads/serving/permission_rules/search_result_ads/search_result_ads_per_hour_permission_rule.h"

namespace brave_ads {

// static
bool SearchResultAdPermissionRules::HasPermission() {
  if (!UserHasJoinedBraveRewards()) {
    return true;
  }

  if (!PermissionRulesBase::HasPermission()) {
    return false;
  }

  const SearchResultAdsPerDayPermissionRule ads_per_day_permission_rule;
  if (!ShouldAllow(ads_per_day_permission_rule)) {
    return false;
  }

  const SearchResultAdsPerHourPermissionRule ads_per_hour_permission_rule;
  return ShouldAllow(ads_per_hour_permission_rule);
}

}  // namespace brave_ads
