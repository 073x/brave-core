/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_UNITTEST_UNITTEST_PREF_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_UNITTEST_UNITTEST_PREF_UTIL_H_

#include <cstdint>
#include <string>

#include "base/values.h"

namespace base {
class Time;
}  // namespace base

namespace brave_ads {

// Unlike |ads_client_mock_->Set*Pref| in |UnitTestBase|, |Set*Pref| will not
// notify observers.
void SetBooleanPref(const std::string& path, bool default_value);
void SetIntegerPref(const std::string& path, int default_value);
void SetDoublePref(const std::string& path, double default_value);
void SetStringPref(const std::string& path, const std::string& default_value);
void SetInt64Pref(const std::string& path, int64_t default_value);
void SetUint64Pref(const std::string& path, uint64_t default_value);
void SetDictPref(const std::string& path, base::Value::Dict default_value = {});
void SetListPref(const std::string& path, base::Value::List default_value = {});
void SetTimePref(const std::string& path, base::Time default_value);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_UNITTEST_UNITTEST_PREF_UTIL_H_
