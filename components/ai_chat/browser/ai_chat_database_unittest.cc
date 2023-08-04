/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/browser/ai_chat_database.h"

#include <stdint.h>
#include <utility>
#include <vector>

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/path_service.h"
#include "base/task/thread_pool.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/threading/sequence_bound.h"
#include "sql/init_status.h"
#include "sql/test/test_helpers.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

class AIChatDatabaseTest : public testing::Test {
 public:
  AIChatDatabaseTest()
      : task_environment_(base::test::TaskEnvironment(
            base::test::TaskEnvironment::ThreadPoolExecutionMode::QUEUED)),
        db_(base::SequenceBound<AIChatDatabase>(CreateTaskRunner())) {}

  void SetUp() override { ASSERT_TRUE(temp_directory_.CreateUniqueTempDir()); }
  void TearDown() override { ASSERT_TRUE(temp_directory_.Delete()); }

  base::FilePath db_dir() { return temp_directory_.GetPath(); }

  base::FilePath db_file_path() {
    return temp_directory_.GetPath().Append("ai_chat");
  }

 protected:
  scoped_refptr<base::SequencedTaskRunner> CreateTaskRunner() {
    return base::ThreadPool::CreateSequencedTaskRunner(
        {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
         base::ThreadPolicy::PREFER_BACKGROUND});
  }

  template <typename Func>
  void InitDBAndDoWork(Func operation) {
    db_.AsyncCall(&AIChatDatabase::Init)
        .WithArgs(db_file_path())
        .Then(base::BindLambdaForTesting([&](sql::InitStatus status) {
          ASSERT_FALSE(status);  // 0 is INIT_OK
          ASSERT_TRUE(base::PathExists(db_file_path()));
          operation();
        }));

    task_environment_.RunUntilIdle();
  }

  base::ScopedTempDir temp_directory_;
  base::test::TaskEnvironment task_environment_;
  base::SequenceBound<AIChatDatabase> db_;
};

TEST_F(AIChatDatabaseTest, WriteConversation) {
  // We need static storage variables throughout the depths of lambdas
  static std::vector<mojom::ConversationEntryTextPtr> texts;
  texts.emplace_back(mojom::ConversationEntryText::New(1, "Hello"));

  InitDBAndDoWork([&]() {
    auto on_conversation_entry_added = [&](bool has_run) {
      EXPECT_TRUE(has_run);
    };

    auto on_conversation_added = [&](bool has_run) {
      EXPECT_TRUE(has_run);

      db_.AsyncCall(&AIChatDatabase::AddConversationEntry)
          .WithArgs(INT64_C(1),
                    mojom::ConversationEntry::New(
                        INT64_C(-1), base::Time::Now(),
                        mojom::CharacterType::HUMAN, std::move(texts)))
          .Then(base::BindLambdaForTesting(on_conversation_entry_added));
    };

    db_.AsyncCall(&AIChatDatabase::AddConversation)
        .WithArgs(mojom::Conversation::New(INT64_C(1), base::Time::Now(),
                                           "Improve label description", GURL()))
        .Then(base::BindLambdaForTesting(on_conversation_added));
  });
}

TEST_F(AIChatDatabaseTest, ReadConversation) {
  // We need static storage variables throughout the depths of lambdas
  static const int64_t kConversationId = INT64_C(1);
  static const base::Time kCreatedAt(base::Time::Now());

  static std::vector<mojom::ConversationEntryTextPtr> kTexts;
  kTexts.emplace_back(
      mojom::ConversationEntryText::New(1, "This is a generated response"));
  kTexts.emplace_back(
      mojom::ConversationEntryText::New(2, "This is a re-generated response"));

  InitDBAndDoWork([&]() {
    db_.AsyncCall(&AIChatDatabase::AddConversation)
        .WithArgs(mojom::Conversation::New(kConversationId, kCreatedAt,
                                           "Summarizing Brave",
                                           GURL("https://brave.com/")))
        .Then(base::BindLambdaForTesting([&](bool has_run) {
          db_.AsyncCall(&AIChatDatabase::AddConversationEntry)
              .WithArgs(kConversationId,
                        mojom::ConversationEntry::New(
                            INT64_C(-1), kCreatedAt,
                            mojom::CharacterType::ASSISTANT, std::move(kTexts)))
              .Then(base::BindLambdaForTesting([&](bool has_run) {
                db_.AsyncCall(&AIChatDatabase::GetAllConversations)
                    .Then(base::BindLambdaForTesting(
                        [&](std::vector<mojom::ConversationPtr> conversations) {
                          EXPECT_FALSE(conversations.empty());
                          EXPECT_EQ(conversations[0]->title,
                                    "Summarizing Brave");
                          EXPECT_EQ(conversations[0]->page_url->spec(),
                                    "https://brave.com/");
                          EXPECT_EQ(conversations[0]
                                        ->date.ToDeltaSinceWindowsEpoch()
                                        .InMicroseconds(),
                                    kCreatedAt.ToDeltaSinceWindowsEpoch()
                                        .InMicroseconds());

                          db_.AsyncCall(&AIChatDatabase::GetConversationEntry)
                              .WithArgs(1)
                              .Then(base::BindLambdaForTesting(
                                  [&](std::vector<mojom::ConversationEntryPtr>
                                          entries) {
                                    EXPECT_EQ(UINT64_C(1), entries.size());
                                    EXPECT_EQ(UINT64_C(2),
                                              entries[0]->texts.size());

                                    EXPECT_EQ(entries[0]->character_type,
                                              mojom::CharacterType::ASSISTANT);
                                    EXPECT_EQ(entries[0]->texts[0]->text,
                                              "This is a generated response");
                                    EXPECT_EQ(
                                        entries[0]->texts[1]->text,
                                        "This is a re-generated response");
                                  }));
                        }));
              }));
        }));
  });
}

}  // namespace ai_chat
