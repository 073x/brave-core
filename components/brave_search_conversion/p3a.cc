// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/brave_search_conversion/p3a.h"

#include "base/logging.h"
#include "base/metrics/histogram_functions.h"
#include "base/notreached.h"
#include "brave/components/brave_search_conversion/pref_names.h"
#include "brave/components/p3a_utils/bucket.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace brave_search_conversion {
namespace p3a {

namespace {

const int kMaxStoredQueryCount = 41;
const int kQueriesBeforeChurnBuckets[] = {0, 1, 2, 5, 10, 20, 40};

const char* GetPromoShownPrefName(ConversionType type) {
  switch (type) {
    case ConversionType::kBanner:
    // Added below four new types just for preventing NOTREACHED().
    // Need to handle later with p3a team.
    case ConversionType::kBannerTypeA:
    case ConversionType::kBannerTypeB:
    case ConversionType::kBannerTypeC:
    case ConversionType::kBannerTypeD:
      return prefs::kP3ABannerShown;
    case ConversionType::kButton:
      return prefs::kP3AButtonShown;
    case ConversionType::kNTP:
      return prefs::kP3ANTPShown;
    default:
      NOTREACHED();
      return nullptr;
  }
}

const char* GetPromoTriggeredPrefName(ConversionType type) {
  switch (type) {
    case ConversionType::kBanner:
    // Added below four new types just for preventing NOTREACHED().
    // Need to handle later with p3a team.
    case ConversionType::kBannerTypeA:
    case ConversionType::kBannerTypeB:
    case ConversionType::kBannerTypeC:
    case ConversionType::kBannerTypeD:
      return prefs::kP3ABannerTriggered;
    case ConversionType::kButton:
      return prefs::kP3AButtonTriggered;
    case ConversionType::kNTP:
      return prefs::kP3ANTPTriggered;
    default:
      NOTREACHED();
      return nullptr;
  }
}

const char* GetPromoTypeHistogramName(ConversionType type) {
  switch (type) {
    case ConversionType::kBanner:
      return kSearchPromoBannerHistogramName;
    case ConversionType::kButton:
      return kSearchPromoButtonHistogramName;
    case ConversionType::kNTP:
      return kSearchPromoNTPHistogramName;
    default:
      NOTREACHED();
      return nullptr;
  }
}

void UpdateHistograms(PrefService* prefs) {
  const ConversionType types[] = {
      ConversionType::kBanner, ConversionType::kButton, ConversionType::kNTP};

  VLOG(1) << "SearchConversionP3A: updating histograms";

  const bool default_engine_triggered =
      prefs->GetBoolean(prefs::kP3ADefaultEngineConverted);
  for (const auto type : types) {
    const char* shown_pref_name = GetPromoShownPrefName(type);
    const char* triggered_pref_name = GetPromoTriggeredPrefName(type);
    const char* histogram_name = GetPromoTypeHistogramName(type);
    DCHECK(shown_pref_name);
    DCHECK(triggered_pref_name);
    DCHECK(histogram_name);
    if (!prefs->GetBoolean(shown_pref_name)) {
      // Do not report to P3A if promo was never shown.
      continue;
    }
    const bool promo_triggered = prefs->GetBoolean(triggered_pref_name);

    // 0 = have not triggered promo, have not made Brave default via SERP
    // 1 = have triggered promo, have not made Brave default via SERP
    int answer = promo_triggered;

    if (default_engine_triggered) {
      // 2 = have not triggered promo, have made Brave default via SERP
      // 3 = have triggered promo, have made Brave default via SERP
      answer += 2;
    }

    base::UmaHistogramExactLinear(histogram_name, answer, 4);
  }
}

}  // namespace

void RegisterLocalStatePrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(prefs::kP3AButtonShown, false);
  registry->RegisterBooleanPref(prefs::kP3ABannerShown, false);
  registry->RegisterBooleanPref(prefs::kP3ANTPShown, false);

  registry->RegisterBooleanPref(prefs::kP3ABannerTriggered, false);
  registry->RegisterBooleanPref(prefs::kP3AButtonTriggered, false);
  registry->RegisterBooleanPref(prefs::kP3ANTPTriggered, false);

  registry->RegisterBooleanPref(prefs::kP3ADefaultEngineConverted, false);

  registry->RegisterIntegerPref(prefs::kP3AQueryCountBeforeChurn, 0);
  registry->RegisterBooleanPref(prefs::kP3AAlreadyChurned, false);
}

void RecordPromoShown(PrefService* prefs, ConversionType type) {
  const char* pref_name = GetPromoShownPrefName(type);
  DCHECK(pref_name);

  VLOG(1) << "SearchConversionP3A: promo shown, pref = " << pref_name;

  const bool prev_setting = prefs->GetBoolean(pref_name);
  if (prev_setting) {
    return;
  }
  prefs->SetBoolean(pref_name, true);
  UpdateHistograms(prefs);
}

void RecordPromoTrigger(PrefService* prefs, ConversionType type) {
  const char* pref_name = GetPromoTriggeredPrefName(type);
  DCHECK(pref_name);

  VLOG(1) << "SearchConversionP3A: promo triggered, pref = " << pref_name;

  const bool prev_setting = prefs->GetBoolean(pref_name);
  if (prev_setting) {
    return;
  }
  prefs->SetBoolean(pref_name, true);
  UpdateHistograms(prefs);
}

void RecordLocationBarQuery(PrefService* prefs) {
  const int total = prefs->GetInteger(prefs::kP3AQueryCountBeforeChurn);
  if (total >= kMaxStoredQueryCount) {
    return;
  }
  prefs->SetInteger(prefs::kP3AQueryCountBeforeChurn, total + 1);
}

void RecordDefaultEngineConversion(PrefService* prefs) {
  VLOG(1) << "SearchConversionP3A: default engine converted";
  prefs->SetBoolean(prefs::kP3ADefaultEngineConverted, true);
  prefs->ClearPref(prefs::kP3AQueryCountBeforeChurn);
  UpdateHistograms(prefs);
}

void RecordDefaultEngineChurn(PrefService* prefs) {
  VLOG(1) << "SearchConversionP3A: default engine churned";
  const bool already_churned = prefs->GetBoolean(prefs::kP3AAlreadyChurned);
  const int total = prefs->GetInteger(prefs::kP3AQueryCountBeforeChurn);
  if (already_churned && total == 0) {
    // If the user already churned before, only report if they have made at
    // least one query. This will handle the case of the user switching to
    // another engine on multiple profiles.
    return;
  }
  p3a_utils::RecordToHistogramBucket(kSearchQueriesBeforeChurnHistogramName,
                                     kQueriesBeforeChurnBuckets, total);
  prefs->SetBoolean(prefs::kP3AAlreadyChurned, true);
  prefs->ClearPref(prefs::kP3AQueryCountBeforeChurn);
}

}  // namespace p3a
}  // namespace brave_search_conversion
