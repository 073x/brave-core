/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/profiles/brave_renderer_updater.h"

#include <utility>

#include "base/bind.h"
#include "brave/common/brave_renderer_configuration.mojom.h"
#include "brave/components/brave_wallet/brave_wallet_constants.h"
#include "brave/components/brave_wallet/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/signin/identity_manager_factory.h"
#include "chrome/common/pref_names.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_process_host.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"

BraveRendererUpdater::BraveRendererUpdater(Profile* profile)
    : profile_(profile),
      is_tor_profile_(false),
      identity_manager_observer_(this) {
  identity_manager_ = IdentityManagerFactory::GetForProfile(profile);
  identity_manager_observer_.Add(identity_manager_);
  PrefService* pref_service = profile->GetPrefs();
  brave_wallet_web3_provider_.Init(kBraveWalletWeb3Provider, pref_service);

  pref_change_registrar_.Init(pref_service);
  pref_change_registrar_.Add(
      kBraveWalletWeb3Provider,
      base::BindRepeating(&BraveRendererUpdater::UpdateAllRenderers,
                          base::Unretained(this)));
}

BraveRendererUpdater::~BraveRendererUpdater() {}

void BraveRendererUpdater::Shutdown() {
  identity_manager_observer_.RemoveAll();
  identity_manager_ = nullptr;
}

void BraveRendererUpdater::InitializeRenderer(
    content::RenderProcessHost* render_process_host) {
  is_tor_profile_ = render_process_host->GetBrowserContext()->IsTor();
  auto renderer_configuration = GetRendererConfiguration(render_process_host);
  UpdateRenderer(&renderer_configuration);
}

std::vector<mojo::AssociatedRemote<brave::mojom::BraveRendererConfiguration>>
BraveRendererUpdater::GetRendererConfigurations() {
  std::vector<mojo::AssociatedRemote<brave::mojom::BraveRendererConfiguration>>
      rv;
  for (content::RenderProcessHost::iterator it(
           content::RenderProcessHost::AllHostsIterator());
       !it.IsAtEnd(); it.Advance()) {
    Profile* renderer_profile =
        static_cast<Profile*>(it.GetCurrentValue()->GetBrowserContext());
    if (renderer_profile == profile_ ||
        renderer_profile->GetOriginalProfile() == profile_) {
      auto renderer_configuration =
          GetRendererConfiguration(it.GetCurrentValue());
      if (renderer_configuration)
        rv.push_back(std::move(renderer_configuration));
    }
  }
  return rv;
}

mojo::AssociatedRemote<brave::mojom::BraveRendererConfiguration>
BraveRendererUpdater::GetRendererConfiguration(
    content::RenderProcessHost* render_process_host) {
  IPC::ChannelProxy* channel = render_process_host->GetChannel();
  if (!channel)
    return mojo::AssociatedRemote<brave::mojom::BraveRendererConfiguration>();

  mojo::AssociatedRemote<brave::mojom::BraveRendererConfiguration>
      renderer_configuration;
  channel->GetRemoteAssociatedInterface(&renderer_configuration);

  return renderer_configuration;
}

void BraveRendererUpdater::OnPrimaryAccountChanged(
    const signin::PrimaryAccountChangeEvent& event) {
  if (event.GetEventTypeFor(signin::ConsentLevel::kSync) ==
      signin::PrimaryAccountChangeEvent::Type::kNone) {
    return;
  }
  UpdateAllRenderers();
}

void BraveRendererUpdater::UpdateAllRenderers() {
  auto renderer_configurations = GetRendererConfigurations();
  for (auto& renderer_configuration : renderer_configurations)
    UpdateRenderer(&renderer_configuration);
}

void BraveRendererUpdater::UpdateRenderer(
    mojo::AssociatedRemote<brave::mojom::BraveRendererConfiguration>*
        renderer_configuration) {
  bool use_brave_web3_provider = (static_cast<BraveWalletWeb3ProviderTypes>(
                                      brave_wallet_web3_provider_.GetValue()) ==
                                  BraveWalletWeb3ProviderTypes::BRAVE_WALLET) &&
                                 !is_tor_profile_;

  (*renderer_configuration)
      ->SetConfiguration(
          brave::mojom::DynamicParams::New(use_brave_web3_provider));
}
