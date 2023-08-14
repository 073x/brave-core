/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_service/status_tray/notification_utils.h"

#include <objbase.h>
#include <wrl/event.h>
#include <wrl/client.h>

#include "base/win/scoped_hstring.h"
#include "base/functional/bind.h"
#include "base/win/win_util.h"
#include "base/win/com_init_util.h"
#include "base/win/core_winrt_util.h"
#include "base/logging.h"
#include "chrome/installer/util/install_util.h"
#include "chrome/installer/util/shell_util.h"

namespace mswr = Microsoft::WRL;
namespace winui = ABI::Windows::UI;
namespace winxml = ABI::Windows::Data::Xml;

namespace brave_vpn {


// Templated wrapper for winfoundtn::GetActivationFactory().
template <unsigned int size>
HRESULT CreateActivationFactory(wchar_t const (&class_name)[size],
                                const IID& iid,
                                void** factory) {
  base::win::ScopedHString ref_class_name =
      base::win::ScopedHString::Create(base::WStringPiece(class_name, size - 1));
  return base::win::RoGetActivationFactory(ref_class_name.get(), iid, factory);
}

mswr::ComPtr<winui::Notifications::IToastNotifier> CreateToastNotifier() {
  mswr::ComPtr<winui::Notifications::IToastNotificationManagerStatics>
      toast_manager;
  HRESULT hr = CreateActivationFactory(
      RuntimeClass_Windows_UI_Notifications_ToastNotificationManager,
      IID_PPV_ARGS(&toast_manager));
  if (FAILED(hr)) {
    return nullptr;
  }
  mswr::ComPtr<winui::Notifications::IToastNotifier> notifier;
  std::wstring appid = ShellUtil::GetBrowserModelId(InstallUtil::IsPerUserInstall());
  base::win::ScopedHString application_id = base::win::ScopedHString::Create(appid);
  LOG(ERROR) << "application_id:" << appid;
  hr = toast_manager->CreateToastNotifierWithId(application_id.get(),
                                                &notifier);
  if (FAILED(hr)) {
    return nullptr;
  }
  
  return notifier;
}

mswr::ComPtr<winui::Notifications::IToastNotification> GetToastNotification(const std::wstring& xml_template) {
    base::win::ScopedHString ref_class_name =
        base::win::ScopedHString::Create(RuntimeClass_Windows_Data_Xml_Dom_XmlDocument);
    mswr::ComPtr<IInspectable> inspectable;
    HRESULT hr =
        base::win::RoActivateInstance(ref_class_name.get(), &inspectable);
    if (FAILED(hr)) {
      return nullptr;
    }

    mswr::ComPtr<winxml::Dom::IXmlDocumentIO> document_io;
    hr = inspectable.As(&document_io);
    if (FAILED(hr)) {
      return nullptr;
    }

    base::win::ScopedHString ref_template = base::win::ScopedHString::Create(xml_template);
    hr = document_io->LoadXml(ref_template.get());
    if (FAILED(hr)) {
      return nullptr;
    }

    mswr::ComPtr<winxml::Dom::IXmlDocument> document;
    hr = document_io.As<winxml::Dom::IXmlDocument>(&document);
    if (FAILED(hr)) {
      return nullptr;
    }

    mswr::ComPtr<winui::Notifications::IToastNotificationFactory>
        toast_notification_factory;
    hr = CreateActivationFactory(
        RuntimeClass_Windows_UI_Notifications_ToastNotification,
        IID_PPV_ARGS(&toast_notification_factory));
    if (FAILED(hr)) {
      return nullptr;
    }

    mswr::ComPtr<winui::Notifications::IToastNotification> toast_notification;
    hr = toast_notification_factory->CreateToastNotification(
        document.Get(), &toast_notification);
    if (FAILED(hr)) {
      return nullptr;
    }
    return toast_notification;
}

mswr::ComPtr<winui::Notifications::IToastNotification> ShowConnectedNotification() {
  base::win::AssertComInitialized();
  mswr::ComPtr<winui::Notifications::IToastNotifier> notifier = CreateToastNotifier();
  mswr::ComPtr<winui::Notifications::IToastNotification> toast = GetToastNotification(
    L"<toast><visual><binding template='ToastGeneric'><text>Hello world</text></binding></visual></toast>"
  );
  if (!toast)
    return nullptr;
  winui::Notifications::NotificationSetting setting;
  HRESULT hr = notifier->get_Setting(&setting);
  LOG(ERROR) << "successs:" << SUCCEEDED(hr) << ", setting:" << int(setting);
  
  if (FAILED(notifier->Show(toast.Get()))) {
      LOG(ERROR) << "Failed to show notification";
      return nullptr;
  }
  LOG(ERROR) << "Notification shown";
  return toast;
}

}  // namespace brave_vpn
