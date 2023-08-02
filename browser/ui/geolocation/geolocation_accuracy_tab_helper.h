/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_GEOLOCATION_GEOLOCATION_ACCURACY_TAB_HELPER_H_
#define BRAVE_BROWSER_UI_GEOLOCATION_GEOLOCATION_ACCURACY_TAB_HELPER_H_

#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

class GeolocationAccuracyTabHelper
    : public content::WebContentsUserData<GeolocationAccuracyTabHelper>,
      public content::WebContentsObserver {
 public:
  static void MaybeCreateForWebContents(content::WebContents* contents);

  ~GeolocationAccuracyTabHelper() override;

  // True when want to prevent permission bubble showing.
  bool LaunchAccuracyHelperDialogIfNeeded();

  // content::WebContentsObserver overrides:
  void DidStartNavigation(
      content::NavigationHandle* navigation_handle) override;

 private:
  friend WebContentsUserData;

  explicit GeolocationAccuracyTabHelper(content::WebContents* contents);

  bool accuracy_dialog_asked_ = false;
  bool is_dialog_running_ = false;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

#endif  // BRAVE_BROWSER_UI_GEOLOCATION_GEOLOCATION_ACCURACY_TAB_HELPER_H_
