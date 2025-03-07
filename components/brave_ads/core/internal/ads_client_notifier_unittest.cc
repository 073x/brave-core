/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/ads_client_notifier.h"

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/mock_ads_client_notifier_observer.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

using testing::ElementsAre;
using testing::Mock;
using testing::StrictMock;

constexpr char kLocale[] = "Locale";
constexpr char kPrefPath[] = "PrefPath";
constexpr char kManifestVersion[] = "ManifestVersion";
constexpr char kResourceId[] = "ResourceId";
constexpr char kPaymentId[] = "PaymentId";
constexpr char kRecoverySeed[] = "RecoverySeed";
constexpr char kRedirectChainUrl[] = "https://example.com";
constexpr char kText[] = "Text content";
constexpr char kHtml[] = "HTML content";

constexpr int32_t kTabId = 1;
constexpr int32_t kPageTransitionType = 2;

constexpr bool kIsVisible = true;
constexpr bool kScreenWasLocked = true;

constexpr base::TimeDelta kIdleTime = base::Minutes(1);

void Notify(const AdsClientNotifier& queued_notifier) {
  queued_notifier.NotifyDidInitializeAds();
  queued_notifier.NotifyLocaleDidChange(kLocale);
  queued_notifier.NotifyPrefDidChange(kPrefPath);
  queued_notifier.NotifyDidUpdateResourceComponent(kManifestVersion,
                                                   kResourceId);
  queued_notifier.NotifyRewardsWalletDidUpdate(kPaymentId, kRecoverySeed);
  queued_notifier.NotifyTabTextContentDidChange(
      kTabId, {GURL(kRedirectChainUrl)}, kText);
  queued_notifier.NotifyTabHtmlContentDidChange(
      kTabId, {GURL(kRedirectChainUrl)}, kHtml);
  queued_notifier.NotifyTabDidStartPlayingMedia(kTabId);
  queued_notifier.NotifyTabDidStopPlayingMedia(kTabId);
  queued_notifier.NotifyTabDidChange(kTabId, {GURL(kRedirectChainUrl)},
                                     kIsVisible);
  queued_notifier.NotifyDidCloseTab(kTabId);
  queued_notifier.NotifyUserGestureEventTriggered(kPageTransitionType);
  queued_notifier.NotifyUserDidBecomeIdle();
  queued_notifier.NotifyUserDidBecomeActive(kIdleTime, kScreenWasLocked);
  queued_notifier.NotifyBrowserDidEnterForeground();
  queued_notifier.NotifyBrowserDidEnterBackground();
  queued_notifier.NotifyBrowserDidBecomeActive();
  queued_notifier.NotifyBrowserDidResignActive();
  queued_notifier.NotifyDidSolveAdaptiveCaptcha();
}

void ExpectNotifierCalls(MockAdsClientNotifierObserver& observer,
                         int expected_calls_count) {
  EXPECT_CALL(observer, OnNotifyDidInitializeAds()).Times(expected_calls_count);
  EXPECT_CALL(observer, OnNotifyLocaleDidChange(kLocale))
      .Times(expected_calls_count);
  EXPECT_CALL(observer, OnNotifyPrefDidChange(kPrefPath))
      .Times(expected_calls_count);
  EXPECT_CALL(observer,
              OnNotifyDidUpdateResourceComponent(kManifestVersion, kResourceId))
      .Times(expected_calls_count);
  EXPECT_CALL(observer,
              OnNotifyRewardsWalletDidUpdate(kPaymentId, kRecoverySeed))
      .Times(expected_calls_count);
  EXPECT_CALL(observer,
              OnNotifyTabTextContentDidChange(
                  kTabId, ElementsAre(GURL(kRedirectChainUrl)), kText))
      .Times(expected_calls_count);
  EXPECT_CALL(observer,
              OnNotifyTabHtmlContentDidChange(
                  kTabId, ElementsAre(GURL(kRedirectChainUrl)), kHtml))
      .Times(expected_calls_count);
  EXPECT_CALL(observer, OnNotifyTabDidStartPlayingMedia(kTabId))
      .Times(expected_calls_count);
  EXPECT_CALL(observer, OnNotifyTabDidStopPlayingMedia(kTabId))
      .Times(expected_calls_count);
  EXPECT_CALL(observer,
              OnNotifyTabDidChange(kTabId, ElementsAre(GURL(kRedirectChainUrl)),
                                   kIsVisible))
      .Times(expected_calls_count);
  EXPECT_CALL(observer, OnNotifyDidCloseTab(kTabId))
      .Times(expected_calls_count);
  EXPECT_CALL(observer, OnNotifyUserGestureEventTriggered(kPageTransitionType))
      .Times(expected_calls_count);
  EXPECT_CALL(observer, OnNotifyUserDidBecomeIdle())
      .Times(expected_calls_count);
  EXPECT_CALL(observer,
              OnNotifyUserDidBecomeActive(kIdleTime, kScreenWasLocked))
      .Times(expected_calls_count);
  EXPECT_CALL(observer, OnNotifyBrowserDidEnterForeground())
      .Times(expected_calls_count);
  EXPECT_CALL(observer, OnNotifyBrowserDidEnterBackground())
      .Times(expected_calls_count);
  EXPECT_CALL(observer, OnNotifyBrowserDidBecomeActive())
      .Times(expected_calls_count);
  EXPECT_CALL(observer, OnNotifyBrowserDidResignActive())
      .Times(expected_calls_count);
  EXPECT_CALL(observer, OnNotifyDidSolveAdaptiveCaptcha())
      .Times(expected_calls_count);
}

}  // namespace

TEST(BraveAdsAdsClientNotifierTest, FireQueuedNotifications) {
  // Arrange
  AdsClientNotifier queued_notifier;
  queued_notifier.set_should_queue_notifications_for_testing(
      /*should_queue_notifications*/ true);

  StrictMock<MockAdsClientNotifierObserver> observer;
  queued_notifier.AddObserver(&observer);

  // Act
  ExpectNotifierCalls(observer, /*expected_calls_count*/ 0);
  Notify(queued_notifier);

  // Assert
  Mock::VerifyAndClearExpectations(&observer);

  // Act
  ExpectNotifierCalls(observer, /*expected_calls_count*/ 1);
  queued_notifier.NotifyPendingObservers();

  // Assert
  Mock::VerifyAndClearExpectations(&observer);

  // Act
  ExpectNotifierCalls(observer, /*expected_calls_count*/ 1);
  Notify(queued_notifier);

  // Assert
  Mock::VerifyAndClearExpectations(&observer);

  queued_notifier.RemoveObserver(&observer);
}

TEST(BraveAdsAdsClientNotifierTest, NotificationsNotFiredIfWereQueued) {
  // Arrange
  StrictMock<MockAdsClientNotifierObserver> observer;
  ExpectNotifierCalls(observer, /*expected_calls_count*/ 0);

  // Act
  {
    AdsClientNotifier queued_notifier;
    queued_notifier.set_should_queue_notifications_for_testing(
        /*should_queue_notifications*/ true);

    queued_notifier.AddObserver(&observer);
    Notify(queued_notifier);
    queued_notifier.RemoveObserver(&observer);
  }

  // Assert
}

TEST(BraveAdsAdsClientNotifierTest, ShouldNotQueueNotifications) {
  // Arrange
  StrictMock<MockAdsClientNotifierObserver> observer;
  ExpectNotifierCalls(observer, /*expected_calls_count*/ 1);

  AdsClientNotifier queued_notifier;
  queued_notifier.set_should_queue_notifications_for_testing(
      /*should_queue_notifications*/ false);
  queued_notifier.AddObserver(&observer);

  // Act
  Notify(queued_notifier);
  queued_notifier.RemoveObserver(&observer);

  // Assert
}

}  // namespace brave_ads
