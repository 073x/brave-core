/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PRIVACY_TOKENS_UNBLINDED_TOKENS_UNBLINDED_TOKEN_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PRIVACY_TOKENS_UNBLINDED_TOKENS_UNBLINDED_TOKEN_UTIL_H_

#include "brave/components/brave_ads/core/internal/privacy/tokens/unblinded_tokens/unblinded_token_info.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads::privacy {

absl::optional<UnblindedTokenInfo> MaybeGetUnblindedToken();

const UnblindedTokenList& GetAllUnblindedTokens();

void AddUnblindedTokens(const UnblindedTokenList& unblinded_tokens);

bool RemoveUnblindedToken(const UnblindedTokenInfo& unblinded_token);
void RemoveUnblindedTokens(const UnblindedTokenList& unblinded_tokens);
void RemoveAllUnblindedTokens();

bool UnblindedTokenExists(const UnblindedTokenInfo& unblinded_token);

bool UnblindedTokensIsEmpty();

size_t UnblindedTokenCount();

[[nodiscard]] bool IsValid(const UnblindedTokenInfo& unblinded_token);

}  // namespace brave_ads::privacy

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PRIVACY_TOKENS_UNBLINDED_TOKENS_UNBLINDED_TOKEN_UTIL_H_
