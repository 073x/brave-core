/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/diagnostics/entries/opted_in_to_brave_news_ads_diagnostic_entry.h"

#include "brave/components/brave_ads/core/internal/ads_client_helper.h"
#include "brave/components/brave_ads/core/internal/common/strings/string_conversions_util.h"
#include "brave/components/brave_news/common/pref_names.h"

namespace brave_ads {

namespace {
constexpr char kName[] = "Opted-in to Brave News ads";
}  // namespace

DiagnosticEntryType OptedInToBraveNewsAdsDiagnosticEntry::GetType() const {
  return DiagnosticEntryType::kOptedInToBraveNewsAds;
}

std::string OptedInToBraveNewsAdsDiagnosticEntry::GetName() const {
  return kName;
}

std::string OptedInToBraveNewsAdsDiagnosticEntry::GetValue() const {
  const bool is_opted_in = AdsClientHelper::GetInstance()->GetBooleanPref(
                               brave_news::prefs::kBraveNewsOptedIn) &&
                           AdsClientHelper::GetInstance()->GetBooleanPref(
                               brave_news::prefs::kNewTabPageShowToday);
  return BoolToString(is_opted_in);
}

}  // namespace brave_ads
