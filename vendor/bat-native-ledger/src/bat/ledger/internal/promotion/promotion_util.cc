/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "bat/ledger/internal/promotion/promotion_util.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/time/time.h"

namespace braveledger_promotion {

std::string ParseOSToString(ledger::OperatingSystem os) {
  switch (static_cast<int>(os)) {
    case static_cast<int>(ledger::OperatingSystem::WINDOWS):  {
      return "windows";
    }
    case static_cast<int>(ledger::OperatingSystem::MACOS):  {
      return "osx";
    }
    case static_cast<int>(ledger::OperatingSystem::LINUX):  {
      return "linux";
    }
    case static_cast<int>(ledger::OperatingSystem::UNDEFINED):  {
      return "undefined";
    }
    default: {
      NOTREACHED();
      return "";
    }
  }
}

std::string ParseClientInfoToString(ledger::ClientInfoPtr info) {
  if (!info) {
    return "";
  }

  switch (static_cast<int>(info->platform)) {
    case static_cast<int>(ledger::Platform::ANDROID_R):  {
      return "android";
    }
    case static_cast<int>(ledger::Platform::IOS):  {
      return "ios";
    }
    case static_cast<int>(ledger::Platform::DESKTOP):  {
      return ParseOSToString(info->os);
    }
    default: {
      NOTREACHED();
      return "";
    }
  }
}

ledger::PromotionType ConvertStringToPromotionType(const std::string& type) {
  if (type == "ugp") {
    return ledger::PromotionType::UGP;
  }

  if (type == "ads") {
    return ledger::PromotionType::ADS;
  }

  // unknown promotion type, returning dummy value.
  NOTREACHED();
  return ledger::PromotionType::UGP;
}

ledger::ReportType ConvertPromotionTypeToReportType(
    const ledger::PromotionType type) {
  switch (static_cast<int>(type)) {
    case static_cast<int>(ledger::PromotionType::UGP): {
      return ledger::ReportType::GRANT_UGP;
    }
    case static_cast<int>(ledger::PromotionType::ADS): {
      return ledger::ReportType::GRANT_AD;
    }
    default: {
      NOTREACHED();
      return ledger::ReportType::GRANT_UGP;
    }
  }
}

ledger::Result ParseFetchResponse(
    const std::string& response,
    ledger::PromotionList* list) {
  if (!list) {
    return ledger::Result::LEDGER_ERROR;
  }

  base::Optional<base::Value> value = base::JSONReader::Read(response);
  if (!value || !value->is_dict()) {
    return ledger::Result::LEDGER_ERROR;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    return ledger::Result::LEDGER_ERROR;
  }

  auto* promotions = dictionary->FindListKey("promotions");
  if (promotions) {
    const auto promotion_size = promotions->GetList().size();
    for (auto& item : promotions->GetList()) {
      ledger::PromotionPtr promotion = ledger::Promotion::New();

      const auto* id = item.FindStringKey("id");
      if (!id) {
        continue;
      }
      promotion->id = *id;

      const auto version = item.FindIntKey("version");
      if (!version) {
        continue;
      }
      promotion->version = *version;

      const auto* type = item.FindStringKey("type");
      if (!type) {
        continue;
      }
      promotion->type = ConvertStringToPromotionType(*type);

      const auto suggestions = item.FindIntKey("suggestionsPerGrant");
      if (!suggestions) {
        continue;
      }
      promotion->suggestions = *suggestions;

      const auto* approximate_value = item.FindStringKey("approximateValue");
      if (!approximate_value) {
        continue;
      }
      promotion->approximate_value = std::stod(*approximate_value);

      const auto available = item.FindBoolKey("available");
      if (!available) {
        continue;
      }
      if (*available) {
        promotion->status = ledger::PromotionStatus::ACTIVE;
      } else {
        promotion->status = ledger::PromotionStatus::OVER;
      }

      auto* expires_at = item.FindStringKey("expiresAt");
      if (!expires_at) {
        continue;
      }

      base::Time time;
      bool success =  base::Time::FromUTCString((*expires_at).c_str(), &time);
      if (success) {
        promotion->expires_at = time.ToDoubleT();
      }

      auto* public_keys = item.FindListKey("publicKeys");
      if (!public_keys || public_keys->GetList().empty()) {
        continue;
      }

      std::string keys_json;
      base::JSONWriter::Write(*public_keys, &keys_json);
      promotion->public_keys = keys_json;

      auto legacy_claimed = item.FindBoolKey("legacyClaimed");
      promotion->legacy_claimed = legacy_claimed.value_or(false);

      list->push_back(std::move(promotion));
    }

    if (promotion_size != list->size()) {
      return ledger::Result::CORRUPTED_DATA;
    }
  }

  return ledger::Result::LEDGER_OK;
}

std::vector<ledger::PromotionType> GetEligiblePromotions() {
  return {
    ledger::PromotionType::ADS
  };
}

}  // namespace braveledger_promotion
