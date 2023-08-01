/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/browser/ai_chat_database.h"

#include <vector>

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/path_service.h"
#include "base/task/thread_pool.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/threading/sequence_bound.h"
#include "sql/database.h"
#include "sql/init_status.h"
#include "sql/meta_table.h"
#include "sql/statement.h"
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
  void TearDown() override { EXPECT_TRUE(temp_directory_.Delete()); }

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

  base::ScopedTempDir temp_directory_;
  base::test::TaskEnvironment task_environment_;
  base::SequenceBound<AIChatDatabase> db_;
};

TEST_F(AIChatDatabaseTest, InitDatabase) {
  EXPECT_FALSE(base::PathExists(db_file_path()));

  db_.AsyncCall(&AIChatDatabase::Init)
      .WithArgs(db_file_path())
      .Then(base::BindLambdaForTesting([&](sql::InitStatus status) {
        EXPECT_FALSE(status);
        EXPECT_TRUE(base::PathExists(db_file_path()));
      }));

  task_environment_.RunUntilIdle();
}

TEST_F(AIChatDatabaseTest, DatabaseHasTables) {}

}  // namespace ai_chat
