/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/browser/ai_chat_database.h"
#include "base/time/time.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace {
const int kCurrentVersionNumber = 1;
#define CONVERSATION_ROW_FIELDS "id, title, page_url"
#define CONVERSATION_ENTRY_FIELDS "id, date, character_type, conversation_id"
#define CONVERSATION_ENTRY_TEXT_FIELDS "id, text, conversation_entry_id"
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

int AIChatDatabase::GetCurrentVersion() {
  return kCurrentVersionNumber;
}

std::vector<mojom::Conversation> AIChatDatabase::GetAllConversations() {
  sql::Statement statement(GetDB().GetUniqueStatement(
      "SELECT conversation.*, conversation_entry.date"
      "FROM conversation"
      "JOIN ("
      "SELECT DISTINCT conversation_id, date"
      "FROM conversation_entry"
      "ORDER BY date DESC"
      ") AS entries"
      "ON conversation.id = entries.conversation_id"));

  std::vector<mojom::Conversation> conversation_list;

  while (statement.Step()) {
    mojom::Conversation conversation;
    conversation.id = statement.ColumnInt64(0);
    conversation.title = statement.ColumnString(1);
    conversation.page_url = statement.ColumnString(2);
    conversation.date =
        base::Time().FromInternalValue(statement.ColumnInt64(3));
    conversation_list.emplace_back(conversation);
  }

  return conversation_list;
}

std::vector<mojom::ConversationEntry> AIChatDatabase::GetConversationEntry(
    const int64_t& conversation_id) {
  sql::Statement statement(GetDB().GetUniqueStatement(
      "SELECT conversation_entry.*, conversation_entry_text.text"
      "FROM conversation_entry"
      "JOIN ("
      "SELECT conversation_entry_id, text"
      "FROM conversation_entry_text"
      ") AS conversation_entry_text"
      "ON conversation_entry.id = conversation_entry_text.conversation_entry_id"
      "WHERE conversation_id=?"
      "ORDER BY conversation_entry.date DESC"));

  std::vector<mojom::ConversationEntry> conversation_history;

  while (statement.Step()) {
    mojom::ConversationEntry entry;
    entry.id = statement.ColumnInt64(0);
    entry.date = base::Time() + base::Microseconds(statement.ColumnInt64(1));
    entry.character_type =
        static_cast<mojom::CharacterType>(statement.ColumnInt(2));
    entry.text = statement.ColumnString(4);
  }

  return conversation_history;
}

bool AIChatDatabase::AddConversation(base::StringPiece title,
                                     const GURL& page_url) {
  sql::Statement statement(GetDB().GetUniqueStatement(
      "INSERT INTO conversation(" CONVERSATION_ROW_FIELDS
      ")VALUES(NULL, ?, ?)"));

  statement.BindString(1, title);
  if (!page_url.is_empty()) {
    statement.BindString(2, page_url.spec());
  }

  if (!statement.Run()) {
    DVLOG(0) << "Failed to execute 'conversation' insert statement";
    return false;
  }

  return true;
}

bool AIChatDatabase::AddConversationEntry(
    const int64_t& conversation_id,
    const mojom::ConversationEntry& entry) {
  sql::Statement get_id_statement(
      GetDB().GetUniqueStatement("SELECT id FROM conversation"
                                 "WHERE id=?"));
  get_id_statement.BindInt64(0, conversation_id);

  if (!get_id_statement.Step()) {
    DVLOG(0) << "ID not found in 'conversation' table";
    return false;
  }

  sql::Transaction transaction(&db_);
  if (!transaction.Begin()) {
    DVLOG(0) << "Transaction cannot begin\n";
    return false;
  }

  sql::Statement insert_conversation_entry_statement(GetDB().GetUniqueStatement(
      "INSERT INTO conversation_entry(" CONVERSATION_ENTRY_FIELDS
      ")VALUES(NULL, ?, ?, ?)"));
  insert_conversation_entry_statement.BindTimeDelta(
      1, base::Time::Now().ToDeltaSinceWindowsEpoch());
  insert_conversation_entry_statement.BindInt(
      2, static_cast<int>(entry.character_type));
  insert_conversation_entry_statement.BindInt64(
      3, get_id_statement.ColumnInt64(0));

  sql::Statement insert_conversation_text_statement(GetDB().GetUniqueStatement(
      "INSERT INTO conversation_entry_text(" CONVERSATION_ENTRY_TEXT_FIELDS
      ")VALUES(NULL, ?, ?)"));
  insert_conversation_text_statement.BindString(1, entry.text);
  insert_conversation_text_statement.BindInt64(2, GetDB().GetLastInsertRowId());

  if (!insert_conversation_entry_statement.Run()) {
    DVLOG(0) << "Failed to execute 'conversation_entry' insert statement\n";
    return false;
  }

  if (!insert_conversation_text_statement.Run()) {
    DVLOG(0)
        << "Failed to execute 'conversation_entry_text' insert statement\n";
    return false;
  }

  transaction.Commit();
  return true;
}

bool AIChatDatabase::DeleteConversation(const int64_t& conversation_id) {
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

sql::Database& AIChatDatabase::GetDBForTesting() {
  return db_;
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
