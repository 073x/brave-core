/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/geolocation/geolocation_accuracy_tab_helper.h"

#include "base/auto_reset.h"
#include "base/functional/callback.h"
#include "base/run_loop.h"
#include "brave/browser/ui/browser_dialogs.h"
#include "brave/browser/ui/geolocation/pref_names.h"
#include "build/build_config.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/permission_controller.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "third_party/blink/public/common/permissions/permission_utils.h"

#if BUILDFLAG(IS_WIN)
#include "brave/browser/ui/geolocation/geolocation_accuracy_utils_win.h"
#endif

// static
void GeolocationAccuracyTabHelper::MaybeCreateForWebContents(
    content::WebContents* contents) {
#if BUILDFLAG(IS_WIN)
  content::WebContentsUserData<
      GeolocationAccuracyTabHelper>::CreateForWebContents(contents);
#endif
}

GeolocationAccuracyTabHelper::GeolocationAccuracyTabHelper(
    content::WebContents* contents)
    : WebContentsUserData(*contents), WebContentsObserver(contents) {}

GeolocationAccuracyTabHelper::~GeolocationAccuracyTabHelper() = default;

bool GeolocationAccuracyTabHelper::LaunchAccuracyHelperDialogIfNeeded() {
  if (auto* prefs =
          user_prefs::UserPrefs::Get(web_contents()->GetBrowserContext());
      !prefs->GetBoolean(kShowGeolocationAccuracyHelperDialog)) {
    return false;
  }

  if (is_dialog_running_) {
    return true;
  }

  if (accuracy_dialog_asked_) {
    return false;
  }

#if BUILDFLAG(IS_WIN)
  if (IsSystemLocationSettingEnabled()) {
    DVLOG(2) << __func__ << " : system location service is enabled.";
    return false;
  }
#endif

  // Only show dialog when this is active tab.
  Browser* browser = chrome::FindBrowserWithWebContents(web_contents());
  if (!browser ||
      web_contents() != browser->tab_strip_model()->GetActiveWebContents()) {
    return false;
  }

  accuracy_dialog_asked_ = true;
  base::RunLoop run_loop(base::RunLoop::Type::kNestableTasksAllowed);
  // is_dialog_running_ = true;
  base::AutoReset reset(&is_dialog_running_, true);
  brave::ShowGeolocationAccuracyHelperDialog(web_contents(),
                                             run_loop.QuitClosure());
  run_loop.Run();
  // is_dialog_running_ = false;
  return false;
}

void GeolocationAccuracyTabHelper::DidStartNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInMainFrame() ||
      navigation_handle->IsSameDocument()) {
    return;
  }

  accuracy_dialog_asked_ = false;
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(GeolocationAccuracyTabHelper);
