/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/tabs/tab_manager.h"

#include "base/check.h"
#include "base/hash/hash.h"
#include "brave/components/brave_ads/core/internal/ads_client_helper.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"
#include "url/gurl.h"

namespace brave_ads {

TabManager::TabManager() {
  AdsClientHelper::AddObserver(this);
}

TabManager::~TabManager() {
  AdsClientHelper::RemoveObserver(this);
}

// static
TabManager& TabManager::GetInstance() {
  return GlobalState::GetInstance()->GetTabManager();
}

void TabManager::AddObserver(TabManagerObserver* observer) {
  CHECK(observer);
  observers_.AddObserver(observer);
}

void TabManager::RemoveObserver(TabManagerObserver* observer) {
  CHECK(observer);
  observers_.RemoveObserver(observer);
}

bool TabManager::IsVisible(const int32_t id) const {
  return id != 0 && visible_tab_id_ == id;
}

bool TabManager::IsPlayingMedia(const int32_t id) const {
  const absl::optional<TabInfo> tab = MaybeGetForId(id);
  return tab ? tab->is_playing_media : false;
}

absl::optional<TabInfo> TabManager::GetVisible() const {
  return MaybeGetForId(visible_tab_id_);
}

absl::optional<TabInfo> TabManager::GetLastVisible() const {
  return MaybeGetForId(last_visible_tab_id_);
}

absl::optional<TabInfo> TabManager::MaybeGetForId(const int32_t id) const {
  if (tabs_.find(id) == tabs_.cend()) {
    return absl::nullopt;
  }

  return tabs_.at(id);
}

///////////////////////////////////////////////////////////////////////////////

TabInfo& TabManager::GetOrCreateForId(const int32_t id) {
  if (tabs_.find(id) == tabs_.cend()) {
    TabInfo tab;
    tab.id = id;
    tabs_[id] = tab;
  }

  return tabs_[id];
}

void TabManager::Remove(const int32_t id) {
  tabs_.erase(id);
}

void TabManager::NotifyTabDidChangeFocus(const int32_t id) const {
  for (TabManagerObserver& observer : observers_) {
    observer.OnTabDidChangeFocus(id);
  }
}

void TabManager::NotifyTabDidChange(const TabInfo& tab) const {
  for (TabManagerObserver& observer : observers_) {
    observer.OnTabDidChange(tab);
  }
}

void TabManager::NotifyDidOpenNewTab(const TabInfo& tab) const {
  for (TabManagerObserver& observer : observers_) {
    observer.OnDidOpenNewTab(tab);
  }
}

void TabManager::NotifyTextContentDidChange(
    const int32_t id,
    const std::vector<GURL>& redirect_chain,
    const std::string& text) {
  for (TabManagerObserver& observer : observers_) {
    observer.OnTextContentDidChange(id, redirect_chain, text);
  }
}

void TabManager::NotifyHtmlContentDidChange(
    const int32_t id,
    const std::vector<GURL>& redirect_chain,
    const std::string& html) {
  for (TabManagerObserver& observer : observers_) {
    observer.OnHtmlContentDidChange(id, redirect_chain, html);
  }
}

void TabManager::NotifyDidCloseTab(const int32_t id) const {
  for (TabManagerObserver& observer : observers_) {
    observer.OnDidCloseTab(id);
  }
}

void TabManager::NotifyTabDidStartPlayingMedia(const int32_t id) const {
  for (TabManagerObserver& observer : observers_) {
    observer.OnTabDidStartPlayingMedia(id);
  }
}

void TabManager::NotifyTabDidStopPlayingMedia(const int32_t id) const {
  for (TabManagerObserver& observer : observers_) {
    observer.OnTabDidStopPlayingMedia(id);
  }
}

void TabManager::OnNotifyTabHtmlContentDidChange(
    const int32_t id,
    const std::vector<GURL>& redirect_chain,
    const std::string& html) {
  CHECK(!redirect_chain.empty());

  const uint32_t hash = base::FastHash(html);
  if (hash != last_html_content_hash_) {
    last_html_content_hash_ = hash;

    BLOG(2, "Tab id " << id << " HTML content changed");
    NotifyHtmlContentDidChange(id, redirect_chain, html);
  }
}

void TabManager::OnNotifyTabTextContentDidChange(
    const int32_t id,
    const std::vector<GURL>& redirect_chain,
    const std::string& text) {
  CHECK(!redirect_chain.empty());

  const uint32_t hash = base::FastHash(text);
  if (hash != last_text_content_hash_) {
    last_text_content_hash_ = hash;

    BLOG(2, "Tab id " << id << " text content changed");
    NotifyTextContentDidChange(id, redirect_chain, text);
  }
}

void TabManager::OnNotifyTabDidStartPlayingMedia(const int32_t id) {
  TabInfo& tab = GetOrCreateForId(id);
  if (!tab.is_playing_media) {
    tab.is_playing_media = true;

    BLOG(2, "Tab id " << id << " started playing media");
    NotifyTabDidStartPlayingMedia(id);
  }
}

void TabManager::OnNotifyTabDidStopPlayingMedia(const int32_t id) {
  TabInfo& tab = GetOrCreateForId(id);
  if (tab.is_playing_media) {
    tab.is_playing_media = false;

    BLOG(2, "Tab id " << id << " stopped playing media");
    NotifyTabDidStopPlayingMedia(id);
  }
}

void TabManager::OnNotifyTabDidChange(const int32_t id,
                                      const std::vector<GURL>& redirect_chain,
                                      const bool is_visible) {
  const bool is_existing_tab = static_cast<bool>(MaybeGetForId(id));

  TabInfo& tab = GetOrCreateForId(id);

  const bool redirect_chain_did_change = tab.redirect_chain != redirect_chain;
  if (redirect_chain_did_change) {
    tab.redirect_chain = redirect_chain;
  }

  if (!is_visible) {
    BLOG(7, "Tab id " << id << " is occluded");
    if (!redirect_chain_did_change) {
      return;
    }

    BLOG(2, "Tab id " << id << " did change");
    return NotifyTabDidChange(tab);
  }

  if (visible_tab_id_ == id) {
    if (!redirect_chain_did_change) {
      return;
    }

    BLOG(2, "Tab id " << id << " did change");
    return NotifyTabDidChange(tab);
  }

  BLOG(2, "Tab id " << id << " is visible");

  last_visible_tab_id_ = visible_tab_id_;
  visible_tab_id_ = id;

  if (is_existing_tab) {
    BLOG(2, "Focused on existing tab with id " << id);
    return NotifyTabDidChangeFocus(id);
  }

  BLOG(2, "Opened a new tab with id " << id);
  NotifyDidOpenNewTab(tab);
}

void TabManager::OnNotifyDidCloseTab(const int32_t id) {
  BLOG(2, "Tab id " << id << " was closed");

  Remove(id);

  NotifyDidCloseTab(id);
}

}  // namespace brave_ads
