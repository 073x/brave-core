/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <map>
#include <utility>
#include <vector>

#include "base/strings/stringprintf.h"
#include "brave/components/brave_rewards/core/constants.h"
#include "brave/components/brave_rewards/core/database/database_publisher_info.h"
#include "brave/components/brave_rewards/core/database/database_util.h"
#include "brave/components/brave_rewards/core/global_constants.h"
#include "brave/components/brave_rewards/core/publisher/publisher.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"

using std::placeholders::_1;

namespace {

const char kTableName[] = "publisher_info";

}  // namespace

namespace brave_rewards::internal {
namespace database {

DatabasePublisherInfo::DatabasePublisherInfo(RewardsEngineImpl& engine)
    : DatabaseTable(engine) {}

DatabasePublisherInfo::~DatabasePublisherInfo() = default;

void DatabasePublisherInfo::InsertOrUpdate(mojom::PublisherInfoPtr info,
                                           LegacyResultCallback callback) {
  if (!info || info->id.empty()) {
    BLOG(1, "Info is empty");
    callback(mojom::Result::FAILED);
    return;
  }

  auto transaction = mojom::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(publisher_id, excluded, name, url, provider, favIcon) "
      "VALUES (?, ?, ?, ?, ?, "
      "(SELECT IFNULL( "
      "(SELECT favicon FROM %s "
      "WHERE publisher_id = ?), '')));",
      kTableName, kTableName);

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::RUN;
  command->command = query;

  BindString(command.get(), 0, info->id);
  BindInt(command.get(), 1, static_cast<int>(info->excluded));
  BindString(command.get(), 2, info->name);
  BindString(command.get(), 3, info->url);
  BindString(command.get(), 4, info->provider);
  BindString(command.get(), 5, info->id);

  transaction->commands.push_back(std::move(command));

  std::string favicon = info->favicon_url;
  if (!favicon.empty() && !info->provider.empty()) {
    const std::string query_icon = base::StringPrintf(
        "UPDATE %s SET favIcon = ? WHERE publisher_id = ?;", kTableName);

    auto command_icon = mojom::DBCommand::New();
    command_icon->type = mojom::DBCommand::Type::RUN;
    command_icon->command = query_icon;

    if (favicon == constant::kClearFavicon) {
      favicon.clear();
    }

    BindString(command_icon.get(), 0, favicon);
    BindString(command_icon.get(), 1, info->id);

    transaction->commands.push_back(std::move(command_icon));
  }

  auto transaction_callback = std::bind(&OnResultCallback, _1, callback);

  engine_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabasePublisherInfo::GetRecord(const std::string& publisher_key,
                                      GetPublisherInfoCallback callback) {
  if (publisher_key.empty()) {
    BLOG(1, "Publisher key is empty");
    callback(mojom::Result::FAILED, {});
    return;
  }

  auto transaction = mojom::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "SELECT pi.publisher_id, pi.name, pi.url, pi.favIcon, pi.provider, "
      "spi.status, spi.updated_at, pi.excluded "
      "FROM %s as pi "
      "LEFT JOIN server_publisher_info AS spi "
      "ON spi.publisher_key = pi.publisher_id "
      "WHERE publisher_id=?",
      kTableName);

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::READ;
  command->command = query;

  BindString(command.get(), 0, publisher_key);

  command->record_bindings = {mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::INT64_TYPE,
                              mojom::DBCommand::RecordBindingType::INT64_TYPE,
                              mojom::DBCommand::RecordBindingType::INT_TYPE};

  transaction->commands.push_back(std::move(command));

  auto transaction_callback =
      std::bind(&DatabasePublisherInfo::OnGetRecord, this, _1, callback);

  engine_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabasePublisherInfo::OnGetRecord(mojom::DBCommandResponsePtr response,
                                        GetPublisherInfoCallback callback) {
  if (!response ||
      response->status != mojom::DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Response is wrong");
    callback(mojom::Result::FAILED, {});
    return;
  }

  if (response->result->get_records().size() != 1) {
    callback(mojom::Result::NOT_FOUND, {});
    return;
  }

  auto* record = response->result->get_records()[0].get();

  auto info = mojom::PublisherInfo::New();
  info->id = GetStringColumn(record, 0);
  info->name = GetStringColumn(record, 1);
  info->url = GetStringColumn(record, 2);
  info->favicon_url = GetStringColumn(record, 3);
  info->provider = GetStringColumn(record, 4);
  info->status = static_cast<mojom::PublisherStatus>(GetInt64Column(record, 5));
  info->status_updated_at = GetInt64Column(record, 6);
  info->excluded =
      static_cast<mojom::PublisherExclude>(GetIntColumn(record, 7));

  callback(mojom::Result::OK, std::move(info));
}

void DatabasePublisherInfo::GetPanelRecord(
    mojom::ActivityInfoFilterPtr filter,
    GetPublisherPanelInfoCallback callback) {
  if (!filter || filter->id.empty()) {
    BLOG(1, "Filter is empty");
    callback(mojom::Result::FAILED, {});
    return;
  }

  auto transaction = mojom::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "SELECT pi.publisher_id, pi.name, pi.url, pi.favIcon, "
      "pi.provider, spi.status, pi.excluded, "
      "("
      "  SELECT IFNULL(percent, 0) FROM activity_info WHERE "
      "  publisher_id = ? AND reconcile_stamp = ? "
      ") as percent "
      "FROM %s AS pi "
      "LEFT JOIN server_publisher_info AS spi "
      "ON spi.publisher_key = pi.publisher_id "
      "WHERE pi.publisher_id = ? LIMIT 1",
      kTableName);

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::READ;
  command->command = query;

  BindString(command.get(), 0, filter->id);
  BindInt64(command.get(), 1, filter->reconcile_stamp);
  BindString(command.get(), 2, filter->id);

  command->record_bindings = {mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::INT64_TYPE,
                              mojom::DBCommand::RecordBindingType::INT_TYPE,
                              mojom::DBCommand::RecordBindingType::INT_TYPE};

  transaction->commands.push_back(std::move(command));

  auto transaction_callback =
      std::bind(&DatabasePublisherInfo::OnGetPanelRecord, this, _1, callback);

  engine_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabasePublisherInfo::OnGetPanelRecord(
    mojom::DBCommandResponsePtr response,
    GetPublisherPanelInfoCallback callback) {
  if (!response ||
      response->status != mojom::DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Response is wrong");
    callback(mojom::Result::FAILED, {});
    return;
  }

  if (response->result->get_records().size() != 1) {
    callback(mojom::Result::NOT_FOUND, {});
    return;
  }

  auto* record = response->result->get_records()[0].get();

  auto info = mojom::PublisherInfo::New();
  info->id = GetStringColumn(record, 0);
  info->name = GetStringColumn(record, 1);
  info->url = GetStringColumn(record, 2);
  info->favicon_url = GetStringColumn(record, 3);
  info->provider = GetStringColumn(record, 4);
  info->status = static_cast<mojom::PublisherStatus>(GetInt64Column(record, 5));
  info->excluded =
      static_cast<mojom::PublisherExclude>(GetIntColumn(record, 6));
  info->percent = GetIntColumn(record, 7);

  callback(mojom::Result::OK, std::move(info));
}

void DatabasePublisherInfo::RestorePublishers(ResultCallback callback) {
  auto transaction = mojom::DBTransaction::New();
  const std::string query = base::StringPrintf(
      "UPDATE %s SET excluded=? WHERE excluded=?", kTableName);

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::RUN;
  command->command = query;

  BindInt(command.get(), 0, static_cast<int>(mojom::PublisherExclude::DEFAULT));
  BindInt(command.get(), 1,
          static_cast<int>(mojom::PublisherExclude::EXCLUDED));

  transaction->commands.push_back(std::move(command));

  engine_->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&DatabasePublisherInfo::OnRestorePublishers,
                     base::Unretained(this), std::move(callback)));
}

void DatabasePublisherInfo::OnRestorePublishers(
    ResultCallback callback,
    mojom::DBCommandResponsePtr response) {
  if (!response ||
      response->status != mojom::DBCommandResponse::Status::RESPONSE_OK) {
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  engine_->publisher()->OnRestorePublishers(mojom::Result::OK,
                                            std::move(callback));
}

void DatabasePublisherInfo::GetExcludedList(GetExcludedListCallback callback) {
  auto transaction = mojom::DBTransaction::New();
  const std::string query = base::StringPrintf(
      "SELECT pi.publisher_id, spi.status, pi.name,"
      "pi.favicon, pi.url, pi.provider "
      "FROM %s as pi "
      "LEFT JOIN server_publisher_info AS spi "
      "ON spi.publisher_key = pi.publisher_id "
      "WHERE pi.excluded = 1",
      kTableName);

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::READ;
  command->command = query;

  command->record_bindings = {mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::INT64_TYPE,
                              mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::STRING_TYPE};

  transaction->commands.push_back(std::move(command));

  auto transaction_callback =
      std::bind(&DatabasePublisherInfo::OnGetExcludedList, this, _1, callback);

  engine_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabasePublisherInfo::OnGetExcludedList(
    mojom::DBCommandResponsePtr response,
    GetExcludedListCallback callback) {
  if (!response ||
      response->status != mojom::DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Response is wrong");
    callback({});
    return;
  }

  std::vector<mojom::PublisherInfoPtr> list;
  for (auto const& record : response->result->get_records()) {
    auto info = mojom::PublisherInfo::New();
    auto* record_pointer = record.get();

    info->id = GetStringColumn(record_pointer, 0);
    info->status =
        static_cast<mojom::PublisherStatus>(GetInt64Column(record_pointer, 1));
    info->name = GetStringColumn(record_pointer, 2);
    info->favicon_url = GetStringColumn(record_pointer, 3);
    info->url = GetStringColumn(record_pointer, 4);
    info->provider = GetStringColumn(record_pointer, 5);

    list.push_back(std::move(info));
  }

  callback(std::move(list));
}

}  // namespace database
}  // namespace brave_rewards::internal
