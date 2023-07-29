/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_BROWSER_AI_CHAT_SERVICE_H_
#define BRAVE_COMPONENTS_AI_CHAT_BROWSER_AI_CHAT_SERVICE_H_

#include "base/files/file_path.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/sequence_bound.h"
#include "brave/components/ai_chat/browser/ai_chat_database.h"
#include "components/keyed_service/core/keyed_service.h"

namespace base {
class SequencedTaskRunner;
}  // namespace base

namespace content {
class BrowserContext;
}  // namespace content

namespace ai_chat {

class AIChatService : public KeyedService {
 public:
  explicit AIChatService(content::BrowserContext* context);
  ~AIChatService() override;
  AIChatService(const AIChatService&) = delete;
  AIChatService& operator=(const AIChatService&) = delete;

 private:
  base::SequencedTaskRunner* GetTaskRunner();

  // KeyedService overrides:
  void Shutdown() override;

  const base::FilePath base_dir_;

  scoped_refptr<base::SequencedTaskRunner> task_runner_;

  base::SequenceBound<AIChatDatabase> ai_chat_db_;

  base::WeakPtrFactory<AIChatService> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_BROWSER_AI_CHAT_SERVICE_H_
