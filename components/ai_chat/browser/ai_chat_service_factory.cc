/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/browser/ai_chat_service_factory.h"

#include "base/no_destructor.h"
#include "brave/components/ai_chat/browser/ai_chat_service.h"
#include "brave/components/ai_chat/common/features.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

namespace ai_chat {

// static
AIChatServiceFactory* AIChatServiceFactory::GetInstance() {
  static base::NoDestructor<AIChatServiceFactory> instance;
  return instance.get();
}

// static
AIChatService* AIChatServiceFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  DCHECK(context);
  if (features::IsAIChatEnabled()) {
    return static_cast<AIChatService*>(
        GetInstance()->GetServiceForBrowserContext(context, true));
  }

  return nullptr;
}

AIChatServiceFactory::AIChatServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "AIChatService",
          BrowserContextDependencyManager::GetInstance()) {}

AIChatServiceFactory::~AIChatServiceFactory() = default;

std::unique_ptr<KeyedService>
AIChatServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  return std::make_unique<AIChatService>(context);
}

}  // namespace ai_chat
