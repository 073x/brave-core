/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <windows.ui.notifications.h>
#include <wrl/client.h>

namespace mswr = Microsoft::WRL;
namespace winui = ABI::Windows::UI;

namespace brave_vpn {

  mswr::ComPtr<winui::Notifications::IToastNotification> ShowConnectedNotification();

}  // namespace brave_vpn
