/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/rewards_engine_impl.h"

#include <memory>
#include <vector>

#include "base/task/thread_pool/thread_pool_instance.h"
#include "brave/components/brave_rewards/core/common/legacy_callback_helpers.h"
#include "brave/components/brave_rewards/core/common/security_util.h"
#include "brave/components/brave_rewards/core/common/time_util.h"
#include "brave/components/brave_rewards/core/global_constants.h"
#include "brave/components/brave_rewards/core/legacy/static_values.h"
#include "brave/components/brave_rewards/core/publisher/publisher_status_helper.h"

using std::placeholders::_1;

namespace brave_rewards::internal {

RewardsEngineImpl::RewardsEngineImpl(
    mojo::PendingAssociatedRemote<mojom::RewardsEngineClient> client_remote)
    : client_(std::move(client_remote)),
      promotion_(*this),
      publisher_(*this),
      media_(*this),
      contribution_(*this),
      wallet_(*this),
      database_(*this),
      report_(*this),
      state_(*this),
      api_(*this),
      recovery_(*this),
      bitflyer_(*this),
      gemini_(*this),
      uphold_(*this),
      zebpay_(*this) {
  DCHECK(base::ThreadPoolInstance::Get());
  set_client_for_logging(client_.get());
}

RewardsEngineImpl::~RewardsEngineImpl() {
  set_client_for_logging(nullptr);
}

// mojom::RewardsEngine implementation begin (in the order of appearance in
// Mojom)
void RewardsEngineImpl::Initialize(InitializeCallback callback) {
  if (ready_state_ != ReadyState::kUninitialized) {
    BLOG(0, "Already initializing");
    return std::move(callback).Run(mojom::Result::FAILED);
  }

  ready_state_ = ReadyState::kInitializing;
  InitializeDatabase(ToLegacyCallback(std::move(callback)));
}

void RewardsEngineImpl::GetEnvironment(GetEnvironmentCallback callback) {
  std::move(callback).Run(_environment);
}

void RewardsEngineImpl::CreateRewardsWallet(
    const std::string& country,
    CreateRewardsWalletCallback callback) {
  WhenReady(
      [](RewardsEngineImpl* self, const std::string& country,
         CreateRewardsWalletCallback callback) {
        self->wallet()->CreateWalletIfNecessary(
            country.empty() ? absl::nullopt
                            : absl::optional<std::string>(std::move(country)),
            std::move(callback));
      },
      country, std::move(callback));
}

void RewardsEngineImpl::GetRewardsParameters(
    GetRewardsParametersCallback callback) {
  WhenReady(
      [](RewardsEngineImpl* self, GetRewardsParametersCallback callback) {
        auto params = self->state()->GetRewardsParameters();
        if (params->rate == 0.0) {
          // A rate of zero indicates that the rewards parameters have
          // not yet been successfully initialized from the server.
          BLOG(1, "Rewards parameters not set - fetching from server");
          self->api()->FetchParameters(std::move(callback));
          return;
        }
        std::move(callback).Run(std::move(params));
      },
      std::move(callback));
}

void RewardsEngineImpl::GetAutoContributeProperties(
    GetAutoContributePropertiesCallback callback) {
  if (!IsReady()) {
    return std::move(callback).Run(mojom::AutoContributeProperties::New());
  }

  auto props = mojom::AutoContributeProperties::New();
  props->enabled_contribute = state()->GetAutoContributeEnabled();
  props->amount = state()->GetAutoContributionAmount();
  props->contribution_min_time = state()->GetPublisherMinVisitTime();
  props->contribution_min_visits = state()->GetPublisherMinVisits();
  props->reconcile_stamp = state()->GetReconcileStamp();
  std::move(callback).Run(std::move(props));
}

void RewardsEngineImpl::GetPublisherMinVisitTime(
    GetPublisherMinVisitTimeCallback callback) {
  if (!IsReady()) {
    return std::move(callback).Run(0);
  }

  std::move(callback).Run(state()->GetPublisherMinVisitTime());
}

void RewardsEngineImpl::GetPublisherMinVisits(
    GetPublisherMinVisitsCallback callback) {
  if (!IsReady()) {
    return std::move(callback).Run(0);
  }

  std::move(callback).Run(state()->GetPublisherMinVisits());
}

void RewardsEngineImpl::GetAutoContributeEnabled(
    GetAutoContributeEnabledCallback callback) {
  if (!IsReady()) {
    return std::move(callback).Run(false);
  }

  std::move(callback).Run(state()->GetAutoContributeEnabled());
}

void RewardsEngineImpl::GetReconcileStamp(GetReconcileStampCallback callback) {
  if (!IsReady()) {
    return std::move(callback).Run(0);
  }

  std::move(callback).Run(state()->GetReconcileStamp());
}

void RewardsEngineImpl::OnLoad(mojom::VisitDataPtr visit_data,
                               uint64_t current_time) {
  if (!IsReady() || !visit_data || visit_data->domain.empty()) {
    return;
  }

  auto iter = current_pages_.find(visit_data->tab_id);
  if (iter != current_pages_.end() &&
      iter->second.domain == visit_data->domain) {
    return;
  }

  if (last_shown_tab_id_ == visit_data->tab_id) {
    last_tab_active_time_ = current_time;
  }

  current_pages_[visit_data->tab_id] = *visit_data;
}

void RewardsEngineImpl::OnUnload(uint32_t tab_id, uint64_t current_time) {
  if (!IsReady()) {
    return;
  }

  OnHide(tab_id, current_time);
  auto iter = current_pages_.find(tab_id);
  if (iter != current_pages_.end()) {
    current_pages_.erase(iter);
  }
}

void RewardsEngineImpl::OnShow(uint32_t tab_id, uint64_t current_time) {
  if (!IsReady()) {
    return;
  }

  last_tab_active_time_ = current_time;
  last_shown_tab_id_ = tab_id;
}

void RewardsEngineImpl::OnHide(uint32_t tab_id, uint64_t current_time) {
  if (!IsReady()) {
    return;
  }

  if (tab_id != last_shown_tab_id_ || last_tab_active_time_ == 0) {
    return;
  }

  auto iter = current_pages_.find(tab_id);
  if (iter == current_pages_.end()) {
    return;
  }

  const std::string type = media()->GetLinkType(iter->second.domain, "", "");
  uint64_t duration = current_time - last_tab_active_time_;
  last_tab_active_time_ = 0;

  if (type == GITHUB_MEDIA_TYPE) {
    base::flat_map<std::string, std::string> parts;
    parts["duration"] = std::to_string(duration);
    media()->ProcessMedia(parts, type, iter->second.Clone());
    return;
  }

  publisher()->SaveVisit(iter->second.domain, iter->second, duration, true, 0,
                         [](mojom::Result, mojom::PublisherInfoPtr) {});
}

void RewardsEngineImpl::OnForeground(uint32_t tab_id, uint64_t current_time) {
  if (!IsReady()) {
    return;
  }

  // When performing automated testing, ignore changes in browser window
  // activation. When running tests in parallel, activation changes can
  // interfere with AC calculations on some platforms.
  if (is_testing) {
    return;
  }

  if (last_shown_tab_id_ != tab_id) {
    return;
  }

  OnShow(tab_id, current_time);
}

void RewardsEngineImpl::OnBackground(uint32_t tab_id, uint64_t current_time) {
  if (!IsReady()) {
    return;
  }

  // When performing automated testing, ignore changes in browser window
  // activation. When running tests in parallel, activation changes can
  // interfere with AC calculations on some platforms.
  if (is_testing) {
    return;
  }

  OnHide(tab_id, current_time);
}

void RewardsEngineImpl::OnXHRLoad(
    uint32_t tab_id,
    const std::string& url,
    const base::flat_map<std::string, std::string>& parts,
    const std::string& first_party_url,
    const std::string& referrer,
    mojom::VisitDataPtr visit_data) {
  if (!IsReady()) {
    return;
  }

  std::string type = media()->GetLinkType(url, first_party_url, referrer);
  if (type.empty()) {
    return;
  }
  media()->ProcessMedia(parts, type, std::move(visit_data));
}

void RewardsEngineImpl::SetPublisherExclude(
    const std::string& publisher_key,
    mojom::PublisherExclude exclude,
    SetPublisherExcludeCallback callback) {
  WhenReady(
      [](RewardsEngineImpl* self, const std::string& publisher_key,
         mojom::PublisherExclude exclude,
         SetPublisherExcludeCallback callback) {
        self->publisher()->SetPublisherExclude(publisher_key, exclude,
                                               std::move(callback));
      },
      publisher_key, exclude, std::move(callback));
}

void RewardsEngineImpl::RestorePublishers(RestorePublishersCallback callback) {
  WhenReady(
      [](RewardsEngineImpl* self, RestorePublishersCallback callback) {
        self->database()->RestorePublishers(std::move(callback));
      },
      std::move(callback));
}

void RewardsEngineImpl::FetchPromotions(FetchPromotionsCallback callback) {
  WhenReady(
      [](RewardsEngineImpl* self, FetchPromotionsCallback callback) {
        self->promotion()->Fetch(std::move(callback));
      },
      std::move(callback));
}

void RewardsEngineImpl::ClaimPromotion(const std::string& promotion_id,
                                       const std::string& payload,
                                       ClaimPromotionCallback callback) {
  WhenReady(
      [](RewardsEngineImpl* self, const std::string& promotion_id,
         const std::string& payload, ClaimPromotionCallback callback) {
        self->promotion()->Claim(promotion_id, payload, std::move(callback));
      },
      promotion_id, payload, std::move(callback));
}

void RewardsEngineImpl::AttestPromotion(const std::string& promotion_id,
                                        const std::string& solution,
                                        AttestPromotionCallback callback) {
  WhenReady(
      [](RewardsEngineImpl* self, const std::string& promotion_id,
         const std::string& solution, AttestPromotionCallback callback) {
        self->promotion()->Attest(promotion_id, solution, std::move(callback));
      },
      promotion_id, solution, std::move(callback));
}

void RewardsEngineImpl::SetPublisherMinVisitTime(int duration_in_seconds) {
  WhenReady(
      [](RewardsEngineImpl* self, int duration_in_seconds) {
        self->state()->SetPublisherMinVisitTime(duration_in_seconds);
      },
      duration_in_seconds);
}

void RewardsEngineImpl::SetPublisherMinVisits(int visits) {
  WhenReady([](RewardsEngineImpl* self,
               int visits) { self->state()->SetPublisherMinVisits(visits); },
            visits);
}

void RewardsEngineImpl::SetAutoContributionAmount(double amount) {
  WhenReady(
      [](RewardsEngineImpl* self, double amount) {
        self->state()->SetAutoContributionAmount(amount);
      },
      amount);
}

void RewardsEngineImpl::SetAutoContributeEnabled(bool enabled) {
  WhenReady(
      [](RewardsEngineImpl* self, bool enabled) {
        self->state()->SetAutoContributeEnabled(enabled);
      },
      enabled);
}

void RewardsEngineImpl::GetBalanceReport(mojom::ActivityMonth month,
                                         int32_t year,
                                         GetBalanceReportCallback callback) {
  WhenReady(
      [](RewardsEngineImpl* self, mojom::ActivityMonth month, int32_t year,
         GetBalanceReportCallback callback) {
        self->database()->GetBalanceReportInfo(
            month, year, ToLegacyCallback(std::move(callback)));
      },
      month, year, std::move(callback));
}

void RewardsEngineImpl::GetPublisherActivityFromUrl(
    uint64_t window_id,
    mojom::VisitDataPtr visit_data,
    const std::string& publisher_blob) {
  WhenReady(
      [](RewardsEngineImpl* self, uint64_t window_id,
         mojom::VisitDataPtr visit_data, const std::string& publisher_blob) {
        self->publisher()->GetPublisherActivityFromUrl(
            window_id, std::move(visit_data), publisher_blob);
      },
      window_id, std::move(visit_data), publisher_blob);
}

void RewardsEngineImpl::GetAutoContributionAmount(
    GetAutoContributionAmountCallback callback) {
  if (!IsReady()) {
    return std::move(callback).Run(0);
  }

  std::move(callback).Run(state()->GetAutoContributionAmount());
}

void RewardsEngineImpl::GetPublisherBanner(
    const std::string& publisher_id,
    GetPublisherBannerCallback callback) {
  WhenReady(
      [](RewardsEngineImpl* self, const std::string& publisher_id,
         GetPublisherBannerCallback callback) {
        self->publisher()->GetPublisherBanner(
            publisher_id, ToLegacyCallback(std::move(callback)));
      },
      publisher_id, std::move(callback));
}

void RewardsEngineImpl::OneTimeTip(const std::string& publisher_key,
                                   double amount,
                                   OneTimeTipCallback callback) {
  WhenReady(
      [](RewardsEngineImpl* self, const std::string& publisher_key,
         double amount, OneTimeTipCallback callback) {
        self->contribution()->OneTimeTip(publisher_key, amount,
                                         ToLegacyCallback(std::move(callback)));
      },
      publisher_key, amount, std::move(callback));
}

void RewardsEngineImpl::RemoveRecurringTip(
    const std::string& publisher_key,
    RemoveRecurringTipCallback callback) {
  WhenReady(
      [](RewardsEngineImpl* self, const std::string& publisher_key,
         RemoveRecurringTipCallback callback) {
        self->database()->RemoveRecurringTip(
            publisher_key, ToLegacyCallback(std::move(callback)));
      },
      publisher_key, std::move(callback));
}

void RewardsEngineImpl::GetCreationStamp(GetCreationStampCallback callback) {
  if (!IsReady()) {
    return std::move(callback).Run(0);
  }

  std::move(callback).Run(state()->GetCreationStamp());
}

void RewardsEngineImpl::GetRewardsInternalsInfo(
    GetRewardsInternalsInfoCallback callback) {
  WhenReady(
      [](RewardsEngineImpl* self, GetRewardsInternalsInfoCallback callback) {
        auto info = mojom::RewardsInternalsInfo::New();

        mojom::RewardsWalletPtr wallet = self->wallet_.GetWallet();
        if (!wallet) {
          BLOG(0, "Wallet is null");
          std::move(callback).Run(std::move(info));
          return;
        }

        // Retrieve the payment id.
        info->payment_id = wallet->payment_id;

        // Retrieve the boot stamp.
        info->boot_stamp = self->state()->GetCreationStamp();

        // Retrieve the key info seed and validate it.
        if (!util::Security::IsSeedValid(wallet->recovery_seed)) {
          info->is_key_info_seed_valid = false;
        } else {
          std::vector<uint8_t> secret_key =
              util::Security::GetHKDF(wallet->recovery_seed);
          std::vector<uint8_t> public_key;
          std::vector<uint8_t> new_secret_key;
          info->is_key_info_seed_valid = util::Security::GetPublicKeyFromSeed(
              secret_key, &public_key, &new_secret_key);
        }

        std::move(callback).Run(std::move(info));
      },
      std::move(callback));
}

void RewardsEngineImpl::SaveRecurringTip(mojom::RecurringTipPtr info,
                                         SaveRecurringTipCallback callback) {
  WhenReady(
      [](RewardsEngineImpl* self, mojom::RecurringTipPtr info,
         SaveRecurringTipCallback callback) {
        self->database()->SaveRecurringTip(
            std::move(info), [self, callback = ToLegacyCallback(std::move(
                                        callback))](mojom::Result result) {
              self->contribution()->SetMonthlyContributionTimer();
              callback(result);
            });
      },
      std::move(info), std::move(callback));
}

void RewardsEngineImpl::SendContribution(const std::string& publisher_id,
                                         double amount,
                                         bool set_monthly,
                                         SendContributionCallback callback) {
  WhenReady(
      [](RewardsEngineImpl* self, const std::string& publisher_id,
         double amount, bool set_monthly, SendContributionCallback callback) {
        self->contribution()->SendContribution(
            publisher_id, amount, set_monthly, std::move(callback));
      },
      publisher_id, amount, set_monthly, std::move(callback));
}

void RewardsEngineImpl::GetRecurringTips(GetRecurringTipsCallback callback) {
  WhenReady(
      [](RewardsEngineImpl* self, GetRecurringTipsCallback callback) {
        self->contribution()->GetRecurringTips(
            ToLegacyCallback(std::move(callback)));
      },
      std::move(callback));
}

void RewardsEngineImpl::GetOneTimeTips(GetOneTimeTipsCallback callback) {
  WhenReady(
      [](RewardsEngineImpl* self, GetOneTimeTipsCallback callback) {
        self->database()->GetOneTimeTips(util::GetCurrentMonth(),
                                         util::GetCurrentYear(),
                                         ToLegacyCallback(std::move(callback)));
      },
      std::move(callback));
}

void RewardsEngineImpl::GetActivityInfoList(
    uint32_t start,
    uint32_t limit,
    mojom::ActivityInfoFilterPtr filter,
    GetActivityInfoListCallback callback) {
  WhenReady(
      [](RewardsEngineImpl* self, uint32_t start, uint32_t limit,
         mojom::ActivityInfoFilterPtr filter,
         GetActivityInfoListCallback callback) {
        self->database()->GetActivityInfoList(
            start, limit, std::move(filter),
            ToLegacyCallback(std::move(callback)));
      },
      start, limit, std::move(filter), std::move(callback));
}

void RewardsEngineImpl::GetPublishersVisitedCount(
    GetPublishersVisitedCountCallback callback) {
  WhenReady(
      [](RewardsEngineImpl* self, GetPublishersVisitedCountCallback callback) {
        self->database()->GetPublishersVisitedCount(std::move(callback));
      },
      std::move(callback));
}

void RewardsEngineImpl::GetExcludedList(GetExcludedListCallback callback) {
  WhenReady(
      [](RewardsEngineImpl* self, GetExcludedListCallback callback) {
        self->database()->GetExcludedList(
            ToLegacyCallback(std::move(callback)));
      },
      std::move(callback));
}

void RewardsEngineImpl::RefreshPublisher(const std::string& publisher_key,
                                         RefreshPublisherCallback callback) {
  WhenReady(
      [](RewardsEngineImpl* self, const std::string& publisher_key,
         RefreshPublisherCallback callback) {
        self->publisher()->RefreshPublisher(
            publisher_key, ToLegacyCallback(std::move(callback)));
      },
      publisher_key, std::move(callback));
}

void RewardsEngineImpl::StartContributionsForTesting() {
  WhenReady([](RewardsEngineImpl* self) {
    self->contribution()->StartContributionsForTesting();  // IN-TEST
  });
}

void RewardsEngineImpl::UpdateMediaDuration(uint64_t window_id,
                                            const std::string& publisher_key,
                                            uint64_t duration,
                                            bool first_visit) {
  WhenReady(
      [](RewardsEngineImpl* self, uint64_t window_id,
         const std::string& publisher_key, uint64_t duration,
         bool first_visit) {
        self->publisher()->UpdateMediaDuration(window_id, publisher_key,
                                               duration, first_visit);
      },
      window_id, publisher_key, duration, first_visit);
}

void RewardsEngineImpl::IsPublisherRegistered(
    const std::string& publisher_id,
    IsPublisherRegisteredCallback callback) {
  WhenReady(
      [](RewardsEngineImpl* self, const std::string& publisher_id,
         IsPublisherRegisteredCallback callback) {
        self->publisher()->GetServerPublisherInfo(
            publisher_id, true /* use_prefix_list */,
            [callback = ToLegacyCallback(std::move(callback))](
                mojom::ServerPublisherInfoPtr info) {
              callback(info &&
                       info->status != mojom::PublisherStatus::NOT_VERIFIED);
            });
      },
      publisher_id, std::move(callback));
}

void RewardsEngineImpl::GetPublisherInfo(const std::string& publisher_key,
                                         GetPublisherInfoCallback callback) {
  WhenReady(
      [](RewardsEngineImpl* self, const std::string& publisher_key,
         GetPublisherInfoCallback callback) {
        self->database()->GetPublisherInfo(
            publisher_key, ToLegacyCallback(std::move(callback)));
      },
      publisher_key, std::move(callback));
}

void RewardsEngineImpl::GetPublisherPanelInfo(
    const std::string& publisher_key,
    GetPublisherPanelInfoCallback callback) {
  WhenReady(
      [](RewardsEngineImpl* self, const std::string& publisher_key,
         GetPublisherPanelInfoCallback callback) {
        self->publisher()->GetPublisherPanelInfo(
            publisher_key, ToLegacyCallback(std::move(callback)));
      },
      publisher_key, std::move(callback));
}

void RewardsEngineImpl::SavePublisherInfo(
    uint64_t window_id,
    mojom::PublisherInfoPtr publisher_info,
    SavePublisherInfoCallback callback) {
  WhenReady(
      [](RewardsEngineImpl* self, uint64_t window_id,
         mojom::PublisherInfoPtr publisher_info,
         SavePublisherInfoCallback callback) {
        self->publisher()->SavePublisherInfo(
            window_id, std::move(publisher_info),
            ToLegacyCallback(std::move(callback)));
      },
      window_id, std::move(publisher_info), std::move(callback));
}

void RewardsEngineImpl::SetInlineTippingPlatformEnabled(
    mojom::InlineTipsPlatforms platform,
    bool enabled) {
  WhenReady(
      [](RewardsEngineImpl* self, mojom::InlineTipsPlatforms platform,
         bool enabled) {
        self->state()->SetInlineTippingPlatformEnabled(platform, enabled);
      },
      platform, enabled);
}

void RewardsEngineImpl::GetInlineTippingPlatformEnabled(
    mojom::InlineTipsPlatforms platform,
    GetInlineTippingPlatformEnabledCallback callback) {
  if (!IsReady()) {
    return std::move(callback).Run(false);
  }

  std::move(callback).Run(state()->GetInlineTippingPlatformEnabled(platform));
}

void RewardsEngineImpl::GetShareURL(
    const base::flat_map<std::string, std::string>& args,
    GetShareURLCallback callback) {
  if (!IsReady()) {
    return std::move(callback).Run("");
  }

  std::move(callback).Run(publisher()->GetShareURL(args));
}

void RewardsEngineImpl::FetchBalance(FetchBalanceCallback callback) {
  WhenReady(
      [](RewardsEngineImpl* self, FetchBalanceCallback callback) {
        self->wallet()->FetchBalance(std::move(callback));
      },
      std::move(callback));
}

void RewardsEngineImpl::GetExternalWallet(const std::string& wallet_type,
                                          GetExternalWalletCallback callback) {
  WhenReady(
      [](RewardsEngineImpl* self, const std::string& wallet_type,
         GetExternalWalletCallback callback) {
        if (wallet_type == constant::kWalletBitflyer) {
          return self->bitflyer()->GetWallet(std::move(callback));
        }

        if (wallet_type == constant::kWalletGemini) {
          return self->gemini()->GetWallet(std::move(callback));
        }

        if (wallet_type == constant::kWalletUphold) {
          return self->uphold()->GetWallet(std::move(callback));
        }

        if (wallet_type == constant::kWalletZebPay) {
          return self->zebpay()->GetWallet(std::move(callback));
        }

        NOTREACHED() << "Unknown external wallet type!";
        std::move(callback).Run(
            base::unexpected(mojom::GetExternalWalletError::kUnexpected));
      },
      wallet_type, std::move(callback));
}

void RewardsEngineImpl::ConnectExternalWallet(
    const std::string& wallet_type,
    const base::flat_map<std::string, std::string>& args,
    ConnectExternalWalletCallback callback) {
  WhenReady(
      [](RewardsEngineImpl* self, const std::string& wallet_type,
         const base::flat_map<std::string, std::string>& args,
         ConnectExternalWalletCallback callback) {
        if (wallet_type == constant::kWalletBitflyer) {
          return self->bitflyer()->ConnectWallet(args, std::move(callback));
        }

        if (wallet_type == constant::kWalletGemini) {
          return self->gemini()->ConnectWallet(args, std::move(callback));
        }

        if (wallet_type == constant::kWalletUphold) {
          return self->uphold()->ConnectWallet(args, std::move(callback));
        }

        if (wallet_type == constant::kWalletZebPay) {
          return self->zebpay()->ConnectWallet(args, std::move(callback));
        }

        NOTREACHED() << "Unknown external wallet type!";
        std::move(callback).Run(
            base::unexpected(mojom::ConnectExternalWalletError::kUnexpected));
      },
      wallet_type, args, std::move(callback));
}

void RewardsEngineImpl::GetTransactionReport(
    mojom::ActivityMonth month,
    int year,
    GetTransactionReportCallback callback) {
  WhenReady(
      [](RewardsEngineImpl* self, mojom::ActivityMonth month, int year,
         GetTransactionReportCallback callback) {
        self->database()->GetTransactionReport(
            month, year, ToLegacyCallback(std::move(callback)));
      },
      month, year, std::move(callback));
}

void RewardsEngineImpl::GetContributionReport(
    mojom::ActivityMonth month,
    int year,
    GetContributionReportCallback callback) {
  WhenReady(
      [](RewardsEngineImpl* self, mojom::ActivityMonth month, int year,
         GetContributionReportCallback callback) {
        self->database()->GetContributionReport(
            month, year, ToLegacyCallback(std::move(callback)));
      },
      month, year, std::move(callback));
}

void RewardsEngineImpl::GetAllContributions(
    GetAllContributionsCallback callback) {
  WhenReady(
      [](RewardsEngineImpl* self, GetAllContributionsCallback callback) {
        self->database()->GetAllContributions(
            ToLegacyCallback(std::move(callback)));
      },
      std::move(callback));
}

void RewardsEngineImpl::GetMonthlyReport(mojom::ActivityMonth month,
                                         int year,
                                         GetMonthlyReportCallback callback) {
  WhenReady(
      [](RewardsEngineImpl* self, mojom::ActivityMonth month, int year,
         GetMonthlyReportCallback callback) {
        self->report()->GetMonthly(month, year,
                                   ToLegacyCallback(std::move(callback)));
      },
      month, year, std::move(callback));
}

void RewardsEngineImpl::GetAllMonthlyReportIds(
    GetAllMonthlyReportIdsCallback callback) {
  WhenReady(
      [](RewardsEngineImpl* self, GetAllMonthlyReportIdsCallback callback) {
        self->report()->GetAllMonthlyIds(ToLegacyCallback(std::move(callback)));
      },
      std::move(callback));
}

void RewardsEngineImpl::GetAllPromotions(GetAllPromotionsCallback callback) {
  WhenReady(
      [](RewardsEngineImpl* self, GetAllPromotionsCallback callback) {
        self->database()->GetAllPromotions(
            ToLegacyCallback(std::move(callback)));
      },
      std::move(callback));
}

void RewardsEngineImpl::Shutdown(ShutdownCallback callback) {
  if (!IsReady()) {
    return std::move(callback).Run(mojom::Result::FAILED);
  }

  ready_state_ = ReadyState::kShuttingDown;
  client_->ClearAllNotifications();

  database()->FinishAllInProgressContributions(
      std::bind(&RewardsEngineImpl::OnAllDone, this, _1,
                ToLegacyCallback(std::move(callback))));
}

void RewardsEngineImpl::GetEventLogs(GetEventLogsCallback callback) {
  WhenReady(
      [](RewardsEngineImpl* self, GetEventLogsCallback callback) {
        self->database()->GetLastEventLogs(
            ToLegacyCallback(std::move(callback)));
      },
      std::move(callback));
}

void RewardsEngineImpl::GetRewardsWallet(GetRewardsWalletCallback callback) {
  WhenReady(
      [](RewardsEngineImpl* self, GetRewardsWalletCallback callback) {
        auto rewards_wallet = self->wallet()->GetWallet();
        if (rewards_wallet) {
          // While the wallet creation flow is running, the Rewards wallet data
          // may have a recovery seed without a payment ID. Only return a struct
          // to the caller if it contains a payment ID.
          if (rewards_wallet->payment_id.empty()) {
            rewards_wallet = nullptr;
          }
        }
        std::move(callback).Run(std::move(rewards_wallet));
      },
      std::move(callback));
}
// mojom::RewardsEngine implementation end

// mojom::RewardsEngineClient helpers begin (in the order of appearance in
// Mojom)
std::string RewardsEngineImpl::GetClientCountryCode() {
  std::string country_code;
  client_->GetClientCountryCode(&country_code);
  return country_code;
}

bool RewardsEngineImpl::IsAutoContributeSupportedForClient() {
  bool value = false;
  client_->IsAutoContributeSupportedForClient(&value);
  return value;
}

std::string RewardsEngineImpl::GetLegacyWallet() {
  std::string wallet;
  client_->GetLegacyWallet(&wallet);
  return wallet;
}

mojom::ClientInfoPtr RewardsEngineImpl::GetClientInfo() {
  auto info = mojom::ClientInfo::New();
  client_->GetClientInfo(&info);
  return info;
}

absl::optional<std::string> RewardsEngineImpl::EncryptString(
    const std::string& value) {
  absl::optional<std::string> result;
  client_->EncryptString(value, &result);
  return result;
}

absl::optional<std::string> RewardsEngineImpl::DecryptString(
    const std::string& value) {
  absl::optional<std::string> result;
  client_->DecryptString(value, &result);
  return result;
}
// mojom::RewardsEngineClient helpers end

mojom::RewardsEngineClient* RewardsEngineImpl::client() {
  return client_.get();
}

database::Database* RewardsEngineImpl::database() {
  return &database_;
}

bool RewardsEngineImpl::IsShuttingDown() const {
  return ready_state_ == ReadyState::kShuttingDown;
}

bool RewardsEngineImpl::IsUninitialized() const {
  return ready_state_ == ReadyState::kUninitialized;
}

bool RewardsEngineImpl::IsReady() const {
  return ready_state_ == ReadyState::kReady;
}

void RewardsEngineImpl::InitializeDatabase(LegacyResultCallback callback) {
  DCHECK(ready_state_ == ReadyState::kInitializing);

  LegacyResultCallback finish_callback = std::bind(
      &RewardsEngineImpl::OnInitialized, this, _1, std::move(callback));

  auto database_callback = std::bind(&RewardsEngineImpl::OnDatabaseInitialized,
                                     this, _1, finish_callback);
  database()->Initialize(database_callback);
}

void RewardsEngineImpl::OnDatabaseInitialized(mojom::Result result,
                                              LegacyResultCallback callback) {
  DCHECK(ready_state_ == ReadyState::kInitializing);

  if (result != mojom::Result::OK) {
    BLOG(0, "Database could not be initialized. Error: " << result);
    callback(result);
    return;
  }

  state()->Initialize(base::BindOnce(&RewardsEngineImpl::OnStateInitialized,
                                     base::Unretained(this),
                                     std::move(callback)));
}

void RewardsEngineImpl::OnStateInitialized(LegacyResultCallback callback,
                                           mojom::Result result) {
  DCHECK(ready_state_ == ReadyState::kInitializing);

  if (result != mojom::Result::OK) {
    BLOG(0, "Failed to initialize state");
    callback(result);
    return;
  }

  callback(mojom::Result::OK);
}

void RewardsEngineImpl::OnInitialized(mojom::Result result,
                                      LegacyResultCallback callback) {
  DCHECK(ready_state_ == ReadyState::kInitializing);

  if (result == mojom::Result::OK) {
    StartServices();
  } else {
    BLOG(0, "Failed to initialize wallet " << result);
  }

  ready_state_ = ReadyState::kReady;
  ready_event_.Signal();

  callback(result);
}

void RewardsEngineImpl::StartServices() {
  DCHECK(ready_state_ == ReadyState::kInitializing);

  publisher()->SetPublisherServerListTimer();
  contribution()->SetAutoContributeTimer();
  contribution()->SetMonthlyContributionTimer();
  promotion()->Refresh(false);
  contribution()->Initialize();
  promotion()->Initialize();
  api()->Initialize();
  recovery_.Check();
}

void RewardsEngineImpl::OnAllDone(mojom::Result result,
                                  LegacyResultCallback callback) {
  database()->Close(std::move(callback));
}

template <typename F, typename... Args>
void RewardsEngineImpl::WhenReady(F callback, Args&&... args) {
  ready_event_.Post(FROM_HERE,
                    base::BindOnce(std::move(callback), base::Unretained(this),
                                   std::forward<Args>(args)...));
}

}  // namespace brave_rewards::internal
