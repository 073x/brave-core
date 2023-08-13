/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_pref.h"

#include "base/check.h"
#include "base/no_destructor.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_current_test_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_pref_registry_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads {

PrefMap& Prefs() {
  static base::NoDestructor<PrefMap> prefs;
  return *prefs;
}

std::string GetPref(const std::string& path) {
  CHECK(HasRegisteredPrefPath(path))
      << "Check failed. Trying to read an unregistered pref: " << path;

  const std::string uuid = GetUuidForCurrentTestAndValue(path);
  const PrefInfo& pref = Prefs()[uuid];
  if (!pref.value) {
    return pref.default_value;
  }

  return *pref.value;
}

void SetPref(const std::string& path, const std::string& value) {
  CHECK(HasRegisteredPrefPath(path))
      << "Check failed. Trying to write an unregistered pref: " << path;

  const std::string uuid = GetUuidForCurrentTestAndValue(path);
  PrefInfo& pref = Prefs()[uuid];
  pref.value = value;
}

std::string GetDefaultPref(const std::string& path) {
  CHECK(HasRegisteredPrefPath(path))
      << "Check failed. Trying to read an unregistered pref: " << path;

  const std::string uuid = GetUuidForCurrentTestAndValue(path);
  const PrefInfo& pref = Prefs()[uuid];
  return pref.default_value;
}

void SetDefaultPref(const std::string& path, const std::string& default_value) {
  CHECK(HasRegisteredPrefPath(path))
      << "Check failed. Trying to write an unregistered pref: " << path;

  const std::string uuid = GetUuidForCurrentTestAndValue(path);
  PrefInfo& pref = Prefs()[uuid];
  pref.default_value = default_value;
}

void ClearPref(const std::string& path) {
  const std::string uuid = GetUuidForCurrentTestAndValue(path);
  CHECK(HasRegisteredPrefPath(path))
      << "Check failed. Trying to clear an unregistered pref: " << path;

  PrefInfo& pref = Prefs()[uuid];
  pref.value.reset();
}

bool HasPrefPath(const std::string& path) {
  CHECK(HasRegisteredPrefPath(path))
      << "Check failed. Trying to read an unregistered pref: " << path;

  const std::string uuid = GetUuidForCurrentTestAndValue(path);
  PrefInfo& pref = Prefs()[uuid];
  return static_cast<bool>(pref.value);
}

}  // namespace brave_ads
