/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_SKU_SKU_TRANSACTION_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_SKU_SKU_TRANSACTION_H_

#include <map>
#include <memory>
#include <string>

#include "base/types/expected.h"
#include "brave/components/brave_rewards/core/database/database_external_transactions.h"
#include "brave/components/brave_rewards/core/endpoint/payment/payment_server.h"
#include "brave/components/brave_rewards/core/ledger.h"

namespace ledger {
class LedgerImpl;

namespace sku {

class SKUTransaction {
 public:
  explicit SKUTransaction(LedgerImpl* ledger);
  ~SKUTransaction();

  void Create(mojom::SKUOrderPtr order,
              const std::string& destination,
              const std::string& wallet_type,
              ledger::LegacyResultCallback callback);

  void SendExternalTransaction(mojom::Result result,
                               const mojom::SKUTransaction& transaction,
                               ledger::LegacyResultCallback callback);

 private:
  void OnTransactionSaved(mojom::Result result,
                          const mojom::SKUTransaction& transaction,
                          const std::string& destination,
                          const std::string& wallet_type,
                          const std::string& contribution_id,
                          ledger::LegacyResultCallback callback);

  void OnTransfer(mojom::Result result,
                  const mojom::SKUTransaction& transaction,
                  const std::string& contribution_id,
                  const std::string& destination,
                  ledger::LegacyResultCallback callback);

  void OnGetExternalTransaction(
      ledger::LegacyResultCallback,
      mojom::SKUTransaction&&,
      base::expected<mojom::ExternalTransactionPtr,
                     database::GetExternalTransactionError>);

  void OnSaveSKUExternalTransaction(mojom::Result result,
                                    const mojom::SKUTransaction& transaction,
                                    ledger::LegacyResultCallback callback);

  void OnSendExternalTransaction(mojom::Result result,
                                 ledger::LegacyResultCallback callback);

  LedgerImpl* ledger_;  // NOT OWNED
  std::unique_ptr<endpoint::PaymentServer> payment_server_;
};

}  // namespace sku
}  // namespace ledger

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_SKU_SKU_TRANSACTION_H_
