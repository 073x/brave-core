// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch } from 'react-redux'
import { skipToken } from '@reduxjs/toolkit/query/react'

// types
import { PanelActions } from '../../panel/actions'
import { BraveWallet } from '../../constants/types'

// hooks
import {
  useUnsafePanelSelector,
  useUnsafeWalletSelector
} from './use-safe-selector'
import { WalletSelectors } from '../selectors'
import { PanelSelectors } from '../../panel/selectors'
import { useGetNetworkQuery } from '../slices/api.slice'

export enum SignDataSteps {
  SignRisk = 0,
  SignData = 1
}

export const useSignSolanaTransactionsQueue = (
  signMode: 'signTx' | 'signAllTxs'
) => {
  // redux
  const dispatch = useDispatch()
  const accounts = useUnsafeWalletSelector(WalletSelectors.accounts)
  const signTransactionRequests = useUnsafePanelSelector(
    PanelSelectors.signTransactionRequests
  )
  const signAllTransactionsRequests = useUnsafePanelSelector(
    PanelSelectors.signAllTransactionsRequests
  )
  const signTransactionQueue =
    signMode === 'signTx'
      ? signTransactionRequests
      : signAllTransactionsRequests

  // queries
  const { data: network } = useGetNetworkQuery(
    signTransactionQueue[0] ?? skipToken
  )

  // state
  const [signStep, setSignStep] = React.useState<SignDataSteps>(
    SignDataSteps.SignRisk
  )
  const [selectedQueueData, setSelectedQueueData] = React.useState<
    | BraveWallet.SignTransactionRequest
    | BraveWallet.SignAllTransactionsRequest
    | undefined
  >(signTransactionQueue[0] || undefined)

  React.useEffect(() => {
    if (selectedQueueData) {
      return
    }

    setSelectedQueueData(signTransactionQueue?.[0])
  }, [signTransactionQueue?.[0], selectedQueueData])

  // memos
  const { queueLength, queueNumber } = React.useMemo(() => {
    return {
      queueLength: signTransactionQueue.length,
      queueNumber:
        signTransactionQueue.findIndex(
          (data) => data.id === selectedQueueData?.id
        ) + 1
    }
  }, [signTransactionQueue, selectedQueueData])

  // force signing messages in-order
  const isDisabled = React.useMemo(() => queueNumber !== 1, [queueNumber])

  const txDatas = React.useMemo(() => {
    return (
      (selectedQueueData as BraveWallet.SignAllTransactionsRequest)?.txDatas
        ? (
            selectedQueueData as BraveWallet.SignAllTransactionsRequest
          )?.txDatas.map(({ solanaTxData }) => solanaTxData)
        : [
            (selectedQueueData as BraveWallet.SignTransactionRequest)?.txData
              ?.solanaTxData
          ]
    ).filter((data): data is BraveWallet.SolanaTxData => !!data)
  }, [selectedQueueData])

  const account = React.useMemo(() => {
    return accounts.find(
      (acc) => acc.address === selectedQueueData?.fromAddress
    )
  }, [accounts, selectedQueueData?.fromAddress])

  // methods
  const onCancelSign = React.useCallback(() => {
    if (!selectedQueueData) {
      return
    }

    const payload = { approved: false, id: selectedQueueData.id }

    dispatch(
      signMode === 'signTx'
        ? PanelActions.signTransactionProcessed(payload)
        : PanelActions.signAllTransactionsProcessed(payload)
    )
  }, [selectedQueueData, signMode])

  const onSign = React.useCallback(() => {
    if (!selectedQueueData || !account) {
      return
    }

    const isHwAccount =
      account.accountId.kind === BraveWallet.AccountKind.kHardware

    if (signMode === 'signTx') {
      if (isHwAccount) {
        dispatch(
          PanelActions.signTransactionHardware(
            selectedQueueData as BraveWallet.SignTransactionRequest
          )
        )
        return
      }
      dispatch(
        PanelActions.signTransactionProcessed({
          approved: true,
          id: selectedQueueData.id
        })
      )
      return
    }

    if (signMode === 'signAllTxs') {
      if (isHwAccount) {
        dispatch(
          PanelActions.signAllTransactionsHardware(
            selectedQueueData as BraveWallet.SignAllTransactionsRequest
          )
        )
        return
      }
      dispatch(
        PanelActions.signAllTransactionsProcessed({
          approved: true,
          id: selectedQueueData.id
        })
      )
    }
  }, [selectedQueueData, account, signMode])

  const onAcceptSigningRisks = React.useCallback(() => {
    setSignStep(SignDataSteps.SignData)
  }, [])

  const queueNextSignTransaction = React.useCallback(() => {
    if (queueNumber === queueLength) {
      setSelectedQueueData(signTransactionQueue[0])
      return
    }
    setSelectedQueueData(signTransactionQueue[queueNumber])
  }, [queueLength, queueNumber, signTransactionQueue])

  // render
  return {
    signTransactionQueue,
    selectedQueueData,
    signStep,
    isDisabled,
    network,
    txDatas,
    signingAccount: account,
    onSign,
    onCancelSign,
    onAcceptSigningRisks,
    queueNextSignTransaction,
    queueLength,
    queueNumber
  }
}
