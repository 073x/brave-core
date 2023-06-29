// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// mocks
import {
  mockSolDappSignAllTransactionsRequest,
  mockSolDappSignAndSendTransactionRequest,
  mockSolDappSignTransactionRequest
} from '../../../common/constants/mocks'
import {
  mockSolanaMainnetNetwork //
} from '../../../stories/mock-data/mock-networks'

// utils
import {
  deserializeTransaction //
} from '../../../utils/model-serialization-utils'

// components
import {
  WalletPanelStory //
} from '../../../stories/wrappers/wallet-panel-story-wrapper'
import { SignTransactionPanel } from './sign-transaction-panel'
import { BraveWallet } from '../../../constants/types'
import { SignDataSteps } from '../../../common/hooks/use-sign-solana-tx-queue'

export const _SignAllSolanaTxPanel = () => {
  const [step, setStep] = React.useState(SignDataSteps.SignRisk)

  return (
    <WalletPanelStory
      panelStateOverride={{
        selectedPanel: 'signTransaction',
        signTransactionRequests: [mockSolDappSignTransactionRequest],
        signAllTransactionsRequests: [mockSolDappSignAllTransactionsRequest]
      }}
      uiStateOverride={{
        selectedPendingTransactionId:
          mockSolDappSignAndSendTransactionRequest.id
      }}
      walletApiDataOverrides={{
        transactionInfos: [
          deserializeTransaction({
            ...mockSolDappSignAndSendTransactionRequest,
            txStatus: BraveWallet.TransactionStatus.Unapproved
          })
        ]
      }}
    >
      <SignTransactionPanel
        signMode={'signAllTxs'}
        isSigningDisabled={false}
        network={mockSolanaMainnetNetwork}
        onAcceptSigningRisks={function (): void | Promise<void> {
          setStep(SignDataSteps.SignData)
        }}
        onCancelSign={function (): void | Promise<void> {
          throw new Error('Function not implemented.')
        }}
        queueNextSignTransaction={function (): void {
          throw new Error('Function not implemented.')
        }}
        onSign={function (): void | Promise<void> {
          throw new Error('Function not implemented.')
        }}
        selectedQueueData={mockSolDappSignAllTransactionsRequest}
        signStep={step}
        txDatas={mockSolDappSignAllTransactionsRequest.txDatas.map(
          (u) => u.solanaTxData!
        )}
        queueLength={1}
        queueNumber={0}
      />
    </WalletPanelStory>
  )
}

_SignAllSolanaTxPanel.story = {
  name: 'Sign Solana All Transactions Panel'
}

export const _SignSolanaTxPanel = () => {
  // state
  const [step, setStep] = React.useState(SignDataSteps.SignRisk)
  return (
    <WalletPanelStory
      panelStateOverride={{
        selectedPanel: 'signTransaction',
        signTransactionRequests: [mockSolDappSignTransactionRequest],
        signAllTransactionsRequests: [mockSolDappSignAllTransactionsRequest]
      }}
      walletApiDataOverrides={{
        transactionInfos: [
          deserializeTransaction(mockSolDappSignAndSendTransactionRequest)
        ]
      }}
    >
      <SignTransactionPanel
        signMode='signTx'
        isSigningDisabled={false}
        network={mockSolanaMainnetNetwork}
        onAcceptSigningRisks={function (): void | Promise<void> {
          setStep(SignDataSteps.SignData)
        }}
        onCancelSign={function (): void | Promise<void> {
          throw new Error('Function not implemented.')
        }}
        queueNextSignTransaction={function (): void {
          throw new Error('Function not implemented.')
        }}
        onSign={function (): void | Promise<void> {
          throw new Error('Function not implemented.')
        }}
        selectedQueueData={mockSolDappSignTransactionRequest}
        signStep={step}
        txDatas={[mockSolDappSignTransactionRequest.txData.solanaTxData!]}
        queueLength={1}
        queueNumber={0}
      />
    </WalletPanelStory>
  )
}

_SignSolanaTxPanel.story = {
  name: 'Sign Solana Transaction Panel'
}

export default {
  parameters: {
    layout: 'centered'
  }
}
