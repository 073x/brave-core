// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'

// Hooks
import {
  useGetIsTxSimulationOptInStatusQuery,
  useGetSolanaTransactionSimulationQuery
} from '../../../common/slices/api.slice'
import {
  useSignSolanaTransactionsQueue
} from '../../../common/hooks/use-sign-solana-tx-queue'

// Components
import {
  EnableTransactionSimulations, //
  LoadingSimulation
} from '../enable-transaction-simulations/enable-transaction-simulations'
import SignTransactionPanel from '../sign-panel/sign-transaction-panel'
import {
  SignSimulatedTransactionPanel //
} from '../confirm-transaction-panel/sign-simulated-tx-panel'

interface Props {
  signMode: 'signTx' | 'signAllTxs'
}

// TODO: WIP
// TODO: WIP
// TODO: WIP
export const PendingSignatureRequestPanel: React.FC<Props> = ({
  signMode
}) => {

  // custom
  const {
    isDisabled,
    network,
    onAcceptSigningRisks,
    onCancelSign,
    onSign,
    queueLength,
    queueNextSignTransaction,
    queueNumber,
    selectedQueueData,
    signStep,
    signingAccount,
    txDatas,
  } = useSignSolanaTransactionsQueue(signMode)

  // queries
  const { data: txSimulationOptIn } = useGetIsTxSimulationOptInStatusQuery()
  const {
    data: solanaSimulation,
    isLoading: isLoadingSolanaSimulation,
    isFetching: isFetchingSolanaSimulation,
    isError: hasSimulationError
  } = useGetSolanaTransactionSimulationQuery(
    txSimulationOptIn === 'allowed' &&
      selectedQueueData?.id !== undefined &&
      network
      ? {
          chainId: network.chainId,
          id: selectedQueueData.id,
          mode:
            signMode === 'signAllTxs'
              ? 'signAllTransactionsRequest'
              : 'signTransactionRequest'
        }
      : skipToken
  )

  // render

  console.log({
    network,
    signMode,
    txSimulationOptIn,
    isLoadingSolanaSimulation,
    isFetchingSolanaSimulation,
    solanaSimulation,
    selectedQueueData
  })

  // Simulations Opt-in screen
  if (txSimulationOptIn === 'unset') {
    return <EnableTransactionSimulations />
  }

  // Loading/Fetching Simulation or network
  if (
    !network ||
    (txSimulationOptIn === 'allowed' &&
      (isLoadingSolanaSimulation || isFetchingSolanaSimulation))
  ) {
    return <LoadingSimulation />
  }

  // Simulated Signature Request
  if (
    txSimulationOptIn === 'allowed' &&
    !hasSimulationError &&
    solanaSimulation
  ) {
    return (
      <SignSimulatedTransactionPanel
        signMode={signMode}
        key={'SVM'}
        txSimulation={solanaSimulation}
        isSigningDisabled={isDisabled}
        network={network}
        onAcceptSigningRisks={onAcceptSigningRisks}
        onCancelSign={onCancelSign}
        onSign={onSign}
        queueLength={queueLength}
        queueNextSignTransaction={queueNextSignTransaction}
        queueNumber={queueNumber}
        selectedQueueData={selectedQueueData}
        signStep={signStep}
        signingAccount={signingAccount}
        txDatas={txDatas}
      />
    )
  }

  // Default (not simulated)
  return <SignTransactionPanel
    signMode={signMode}
    isSigningDisabled={isDisabled}
    network={network}
    onAcceptSigningRisks={onAcceptSigningRisks}
    onCancelSign={onCancelSign}
    onSign={onSign}
    queueLength={queueLength}
    queueNextSignTransaction={queueNextSignTransaction}
    queueNumber={queueNumber}
    selectedQueueData={selectedQueueData}
    signStep={signStep}
    signingAccount={signingAccount}
    txDatas={txDatas}
  />
}

export default PendingSignatureRequestPanel
