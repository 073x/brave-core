/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/browser/ai_chat_service.h"

#include <utility>

#include "base/task/thread_pool.h"
#include "content/public/browser/browser_context.h"

namespace ai_chat {
namespace {
constexpr base::FilePath::StringPieceType kBaseDirName =
    FILE_PATH_LITERAL("ai_chat");
}  // namespace

AIChatService::AIChatService(content::BrowserContext* context)
    : base_dir_(context->GetPath().Append(kBaseDirName)) {
  ai_chat_db_ = base::SequenceBound<AIChatDatabase>(GetTaskRunner());

  auto on_response = base::BindOnce([](sql::InitStatus status) {
    DVLOG(1) << "Init with status: " << status << "\n";
  });

  ai_chat_db_.AsyncCall(&AIChatDatabase::Init)
      .WithArgs(base_dir_)
      .Then(std::move(on_response));
}

AIChatService::~AIChatService() = default;

base::SequencedTaskRunner* AIChatService::GetTaskRunner() {
  if (!task_runner_) {
    task_runner_ = base::ThreadPool::CreateSequencedTaskRunner(
        {base::MayBlock(), base::WithBaseSyncPrimitives(),
         base::TaskPriority::BEST_EFFORT,
         base::TaskShutdownBehavior::BLOCK_SHUTDOWN});
  }

  return task_runner_.get();
}

void AIChatService::Shutdown() {
  task_runner_.reset();
}

}  // namespace ai_chat
