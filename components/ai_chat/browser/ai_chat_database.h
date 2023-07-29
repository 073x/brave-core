/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_BROWSER_AI_CHAT_DATABASE_H_
#define BRAVE_COMPONENTS_AI_CHAT_BROWSER_AI_CHAT_DATABASE_H_

#include <vector>

#include "brave/components/ai_chat/common/mojom/ai_chat.mojom.h"
#include "sql/database.h"
#include "sql/init_status.h"
#include "url/gurl.h"

namespace sql {
class Database;
}  // namespace sql

namespace ai_chat {

class AIChatDatabase {
 public:
  AIChatDatabase();
  AIChatDatabase(const AIChatDatabase&) = delete;
  AIChatDatabase& operator=(const AIChatDatabase&) = delete;
  ~AIChatDatabase();

  sql::InitStatus Init(const base::FilePath& db_file_path);

  static int GetCurrentVersion();
  std::vector<mojom::Conversation> GetAllConversations();
  std::vector<mojom::ConversationEntry> GetConversationEntry(
      const int64_t& conversation_id);
  bool AddConversation(base::StringPiece title, const GURL& page_url = GURL());
  bool AddConversationEntry(const int64_t& conversation_id,
                            const mojom::ConversationEntry& entry);
  bool DeleteConversation(const int64_t& conversation_id);
  bool DropAllTables();

  sql::Database& GetDBForTesting();

 private:
  sql::Database& GetDB();

  bool CreateConversationTable();
  bool CreateConversationEntryTable();
  bool CreateConversationEntryTextTable();

  sql::Database db_;
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_BROWSER_AI_CHAT_DATABASE_H_
