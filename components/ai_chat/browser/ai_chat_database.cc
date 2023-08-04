/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/browser/ai_chat_database.h"
#include "base/time/time.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace {

#define CONVERSATION_ROW_FIELDS "id, title, page_url"
#define CONVERSATION_ENTRY_FIELDS "id, date, character_type, conversation_id"
#define CONVERSATION_ENTRY_TEXT_FIELDS "id, text, conversation_entry_id"

// Do not use To/FromInternalValues in base::Time
// https://bugs.chromium.org/p/chromium/issues/detail?id=634507#c23

base::TimeDelta SerializeTimeToDelta(const base::Time& time) {
  return time.ToDeltaSinceWindowsEpoch();
}

base::Time DeserializeTime(const int64_t& serialized_time) {
  return base::Time() + base::Microseconds(serialized_time);
}

}  // namespace

namespace ai_chat {
AIChatDatabase::AIChatDatabase()
    : db_({.page_size = 4096, .cache_size = 1000}) {}

AIChatDatabase::~AIChatDatabase() = default;

sql::InitStatus AIChatDatabase::Init(const base::FilePath& db_file_path) {
  if (!GetDB().Open(db_file_path)) {
    return sql::INIT_FAILURE;
  }

  if (!CreateConversationTable() || !CreateConversationEntryTable() ||
      !CreateConversationEntryTextTable()) {
    DVLOG(0) << "Failure to create tables\n";
    return sql::INIT_FAILURE;
  }

  return sql::INIT_OK;
}

std::vector<mojom::ConversationPtr> AIChatDatabase::GetAllConversations() {
  sql::Statement statement(GetDB().GetUniqueStatement(
      "SELECT conversation.*, entries.date "
      "FROM conversation"
      " JOIN ("
      "SELECT conversation_id, date"
      " FROM conversation_entry"
      " GROUP BY conversation_id"
      " ORDER BY date DESC"
      ") AS entries"
      " ON conversation.id = entries.conversation_id"));

  std::vector<mojom::ConversationPtr> conversation_list;

  while (statement.Step()) {
    mojom::Conversation conversation;
    conversation.id = statement.ColumnInt64(0);
    conversation.title = statement.ColumnString(1);
    conversation.page_url = GURL(statement.ColumnString(2));
    conversation.date = DeserializeTime(statement.ColumnInt64(3));
    conversation_list.emplace_back(conversation.Clone());
  }

  return conversation_list;
}

std::vector<mojom::ConversationEntryPtr> AIChatDatabase::GetConversationEntry(
    int64_t conversation_id) {
  sql::Statement statement(GetDB().GetUniqueStatement(
      "SELECT conversation_entry.*, entry_text.* "
      "FROM conversation_entry"
      " JOIN ("
      " SELECT *"
      " FROM conversation_entry_text"
      ") AS entry_text"
      " ON conversation_entry.id = entry_text.conversation_entry_id"
      " WHERE conversation_id=?"
      " ORDER BY conversation_entry.date DESC"));

  statement.BindInt64(0, conversation_id);

  std::vector<mojom::ConversationEntryPtr> history;

  while (statement.Step()) {
    mojom::ConversationEntryText entry_text;
    entry_text.id = statement.ColumnInt64(4);
    entry_text.text = statement.ColumnString(5);
    int64_t conversation_entry_id = statement.ColumnInt64(6);

    auto iter = base::ranges::find_if(
        history,
        [&conversation_entry_id](const mojom::ConversationEntryPtr& entry) {
          return entry->id = conversation_entry_id;
        });

    // A ConversationEntry can include multiple generated texts
    if (iter != history.end()) {
      (*iter)->texts.emplace_back(entry_text.Clone());
      continue;
    }

    mojom::ConversationEntry entry;
    entry.id = statement.ColumnInt64(0);
    entry.date = DeserializeTime(statement.ColumnInt64(1));
    entry.character_type =
        static_cast<mojom::CharacterType>(statement.ColumnInt(2));
    entry.texts.emplace_back(entry_text.Clone());

    history.emplace_back(entry.Clone());
  }

  return history;
}

bool AIChatDatabase::AddConversation(mojom::ConversationPtr conversation) {
  sql::Statement statement(GetDB().GetUniqueStatement(
      "INSERT INTO conversation(" CONVERSATION_ROW_FIELDS
      ") VALUES(NULL, ?, ?)"));

  statement.BindString(0, conversation->title);
  if (conversation->page_url.has_value()) {
    statement.BindString(1, conversation->page_url->spec());
  }

  if (!statement.Run()) {
    DVLOG(0) << "Failed to execute 'conversation' insert statement";
    return false;
  }

  return true;
}

bool AIChatDatabase::AddConversationEntry(int64_t conversation_id,
                                          mojom::ConversationEntryPtr entry) {
  sql::Transaction transaction(&db_);
  if (!transaction.Begin()) {
    DVLOG(0) << "Transaction cannot begin\n";
    return false;
  }

  sql::Statement get_conversation_id_statement(
      GetDB().GetUniqueStatement("SELECT id FROM conversation"
                                 " WHERE id=?"));
  get_conversation_id_statement.BindInt64(0, conversation_id);

  if (!get_conversation_id_statement.Step()) {
    DVLOG(0) << "ID not found in 'conversation' table";
    return false;
  }

  sql::Statement insert_conversation_entry_statement(GetDB().GetUniqueStatement(
      "INSERT INTO conversation_entry(" CONVERSATION_ENTRY_FIELDS
      ") VALUES(NULL, ?, ?, ?)"));
  insert_conversation_entry_statement.BindTimeDelta(
      0, SerializeTimeToDelta(entry->date));
  insert_conversation_entry_statement.BindInt(
      1, static_cast<int>(entry->character_type));
  insert_conversation_entry_statement.BindInt64(
      2, get_conversation_id_statement.ColumnInt64(0));

  if (!insert_conversation_entry_statement.Run()) {
    DVLOG(0) << "Failed to execute 'conversation_entry' insert statement\n";
    return false;
  }

  int64_t conversation_entry_row_id = GetDB().GetLastInsertRowId();

  for (const auto& text : entry->texts) {
    sql::Statement insert_conversation_text_statement(
        GetDB().GetUniqueStatement(
            "INSERT INTO "
            "conversation_entry_text(" CONVERSATION_ENTRY_TEXT_FIELDS
            ") VALUES(NULL, ?, ?)"));

    insert_conversation_text_statement.BindString(0, text->text);
    insert_conversation_text_statement.BindInt64(1, conversation_entry_row_id);

    if (!insert_conversation_text_statement.Run()) {
      DVLOG(0)
          << "Failed to execute 'conversation_entry_text' insert statement\n";
      return false;
    }
  }

  transaction.Commit();
  return true;
}

bool AIChatDatabase::DeleteConversation(int64_t conversation_id) {
  sql::Transaction transaction(&db_);
  if (!transaction.Begin()) {
    DVLOG(0) << "Transaction cannot begin\n";
    return false;
  }

  sql::Statement delete_conversation_statement(
      GetDB().GetUniqueStatement("DELETE FROM conversation"
                                 "WHERE id=?"));
  delete_conversation_statement.BindInt64(0, conversation_id);
  delete_conversation_statement.Run();

  sql::Statement select_conversation_entry_statement(
      GetDB().GetUniqueStatement("SELECT id FROM conversation_entry"
                                 "WHERE conversation_id=?"));

  sql::Statement delete_conversation_text_statement(
      GetDB().GetUniqueStatement("DELETE FROM conversation_entry_text"
                                 "WHERE conversation_entry_id=?"));

  // We remove all conversation text associated to |conversation_id|
  while (select_conversation_entry_statement.Step()) {
    delete_conversation_text_statement.BindInt64(
        0, select_conversation_entry_statement.ColumnInt64(0));
    delete_conversation_text_statement.Run();
  }

  sql::Statement delete_conversation_entry_statement(
      GetDB().GetUniqueStatement("DELETE FROM conversation_entry"
                                 "WHERE conversation_entry_id=?"));
  delete_conversation_entry_statement.BindInt64(0, conversation_id);
  // At last we remove all conversation entries associated to |conversation_id|
  delete_conversation_entry_statement.Run();

  transaction.Commit();
  return true;
}

bool AIChatDatabase::DropAllTables() {
  return GetDB().Execute("DROP TABLE conversation") &&
         GetDB().Execute("DROP TABLE conversation_entry") &&
         GetDB().Execute("DROP TABLE conversation_entry_text");
}

sql::Database& AIChatDatabase::GetDB() {
  return db_;
}

bool AIChatDatabase::CreateConversationTable() {
  return GetDB().Execute(
      "CREATE TABLE IF NOT EXISTS conversation("
      "id INTEGER PRIMARY KEY,"
      "title TEXT,"
      "page_url TEXT)");
}

bool AIChatDatabase::CreateConversationEntryTable() {
  return GetDB().Execute(
      "CREATE TABLE IF NOT EXISTS conversation_entry("
      "id INTEGER PRIMARY KEY,"
      "date INTEGER NOT NULL,"
      "character_type INTEGER NOT NULL,"
      "conversation_id INTEGER NOT NULL)");
}

bool AIChatDatabase::CreateConversationEntryTextTable() {
  return GetDB().Execute(
      "CREATE TABLE IF NOT EXISTS conversation_entry_text("
      "id INTEGER PRIMARY KEY,"
      "text TEXT NOT NULL,"
      "conversation_entry_id INTEGER NOT NULL)");
}
}  // namespace ai_chat
