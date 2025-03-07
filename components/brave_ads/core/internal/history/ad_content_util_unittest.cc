/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/history/ad_content_util.h"

#include "brave/components/brave_ads/common/interfaces/brave_ads.mojom-shared.h"
#include "brave/components/brave_ads/core/confirmation_type.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/notification_ad_builder.h"
#include "brave/components/brave_ads/core/notification_ad_info.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

constexpr char kTitle[] = "title";
constexpr char kDescription[] = "description";

}  // namespace

class BraveAdsAdContentUtilTest : public UnitTestBase {};

TEST_F(BraveAdsAdContentUtilTest, Build) {
  // Arrange
  const CreativeNotificationAdInfo creative_ad =
      BuildCreativeNotificationAd(/*should_use_random_uuids*/ true);
  const NotificationAdInfo ad = BuildNotificationAd(creative_ad);

  // Act
  const AdContentInfo ad_content =
      BuildAdContent(ad, ConfirmationType::kViewed, kTitle, kDescription);

  // Assert
  AdContentInfo expected_ad_content;
  expected_ad_content.type = ad.type;
  expected_ad_content.placement_id = ad.placement_id;
  expected_ad_content.creative_instance_id = ad.creative_instance_id;
  expected_ad_content.creative_set_id = ad.creative_set_id;
  expected_ad_content.campaign_id = ad.campaign_id;
  expected_ad_content.advertiser_id = ad.advertiser_id;
  expected_ad_content.brand = kTitle;
  expected_ad_content.brand_info = kDescription;
  expected_ad_content.brand_display_url = ad.target_url.host();
  expected_ad_content.brand_url = ad.target_url;
  expected_ad_content.user_reaction_type = mojom::UserReactionType::kNeutral;
  expected_ad_content.confirmation_type = ConfirmationType::kViewed;

  EXPECT_EQ(expected_ad_content, ad_content);
}

}  // namespace brave_ads
