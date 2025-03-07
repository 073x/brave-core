/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_CATEGORY_CONTENT_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_CATEGORY_CONTENT_INFO_H_

#include <string>

#include "brave/components/brave_ads/common/interfaces/brave_ads.mojom-shared.h"
#include "brave/components/brave_ads/core/export.h"

namespace brave_ads {

struct ADS_EXPORT CategoryContentInfo final {
  std::string category;
  mojom::UserReactionType user_reaction_type =
      mojom::UserReactionType::kNeutral;
};

bool operator==(const CategoryContentInfo&, const CategoryContentInfo&);
bool operator!=(const CategoryContentInfo&, const CategoryContentInfo&);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_CATEGORY_CONTENT_INFO_H_
