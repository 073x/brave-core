// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// types
import {
  BraveWallet,
  CoinTypes,
  SerializableTransactionInfo,
  TxSimulationOptInStatus
} from '../../../constants/types'
import { WalletApiEndpointBuilderParams } from '../api-base.slice'

// utils
import { toMojoUnion } from '../../../utils/mojo-utils'

/**
 * disabled until we have an opt-in screen
 *
 * will be `undefined` until a preference is set
 */
let txSimulationOptInStatus: TxSimulationOptInStatus = 'unset'

export const getIsTxSimulationOptInStatus = () => txSimulationOptInStatus

export const setTxSimulationOptInStatus = (
  enabled: TxSimulationOptInStatus
) => {
  txSimulationOptInStatus = enabled
}

export const transactionSimulationEndpoints = ({
  mutation,
  query
}: WalletApiEndpointBuilderParams) => {
  return {
    getIsTxSimulationOptInStatus: query<TxSimulationOptInStatus, void>({
      queryFn: async (arg, api, extraOptions, baseQuery) => {
        return {
          data: getIsTxSimulationOptInStatus()
        }
      },
      providesTags: ['TransactionSimulationsEnabled']
    }),

    setIsTxSimulationOptInStatus: mutation<
      TxSimulationOptInStatus,
      TxSimulationOptInStatus
    >({
      queryFn: async (status, api, extraOptions, baseQuery) => {
        setTxSimulationOptInStatus(status)
        return {
          data: status
        }
      },
      invalidatesTags: ['TransactionSimulationsEnabled']
    }),

    getEVMTransactionSimulation: query<
      BraveWallet.EVMSimulationResponse,
      Pick<SerializableTransactionInfo, 'chainId' | 'id'> & {
        coinType: BraveWallet.CoinType
      }
    >({
      queryFn: async (txArg, { dispatch }, extraOptions, baseQuery) => {
        try {
          if (getIsTxSimulationOptInStatus() !== 'allowed') {
            throw new Error('Transaction simulation is not enabled')
          }

          const api = baseQuery(undefined).data
          const { simulationService, txService } = api

          if (txArg.coinType !== BraveWallet.CoinType.ETH) {
            throw new Error(
              `Invalid EVM transaction argument cointype for EVM simulation: ${
                txArg.coinType //
              }`
            )
          }

          const { result } = await simulationService.hasTransactionScanSupport(
            txArg.chainId,
            txArg.coinType
          )

          if (!result) {
            throw new Error(
              `EVM transaction simulation not supported for chain/coin: ${
                txArg.chainId //
              }/${txArg.coinType}`
            )
          }

          const { transactionInfo: tx } = await txService.getTransactionInfo(
            txArg.coinType,
            txArg.chainId,
            txArg.id
          )

          if (!tx) {
            throw new Error(
              `could not lookup EVM tx by meta-ID/coin: ${
                txArg.id //
              }/${txArg.coinType}`
            )
          }

          const { errorResponse, errorString, response } =
            await simulationService.scanEVMTransaction(tx, navigator.language)

          if (errorResponse || errorString) {
            const error = `${errorString}: ${errorResponse}`
            const message = `EVM Transaction Simulation error: ${error}`
            console.log(message)
            return {
              error
            }
          }

          if (!response) {
            throw new Error('empty EVM simulation response')
          }

          return {
            data: response
          }
        } catch (error) {
          console.error(error)
          const message = `Failed to simulate EVM transaction: ${error}`
          console.log(message)
          return {
            error: message
          }
        }
      }
    }),

    getSolanaTransactionSimulation: query<
      BraveWallet.SolanaSimulationResponse,
      {
        mode: keyof Parameters<
          BraveWallet.SimulationServiceRemote['scanSolanaTransaction']
        >[0]
        chainId: string
        id: Pick<
          | BraveWallet.SignAllTransactionsRequest
          | BraveWallet.SignTransactionRequest
          | BraveWallet.TransactionInfo,
          'id'
        >['id']
      }
    >({
      queryFn: async (arg, { dispatch }, extraOptions, baseQuery) => {
        try {
          if (getIsTxSimulationOptInStatus() !== 'allowed') {
            throw new Error('Transaction simulation is not enabled')
          }

          const api = baseQuery(undefined).data
          const { simulationService, braveWalletService, txService } = api

          const params: Parameters<
            BraveWallet.SimulationServiceRemote['scanSolanaTransaction']
          >[0] = {
            signAllTransactionsRequest: undefined,
            signTransactionRequest: undefined,
            transactionInfo: undefined
          }

          switch (arg.mode) {
            case 'signAllTransactionsRequest': {
              const { requests } = await braveWalletService //
                .getPendingSignAllTransactionsRequests()

              params.signAllTransactionsRequest = requests.find(
                (r) => r.id === arg?.id
              )
              break
            }

            case 'signTransactionRequest': {
              const { requests } =
                await braveWalletService.getPendingSignTransactionRequests()
              params.signTransactionRequest = requests.find(
                (r) => r.id === arg?.id
              )
              break
            }

            case 'transactionInfo': {
              const { transactionInfo } = await txService.getTransactionInfo(
                CoinTypes.SOL,
                arg.chainId,
                arg.id.toString()
              )
              params.transactionInfo = transactionInfo || undefined
              break
            }

            default: {
              throw new Error(`unsupported SVM simulation mode: ${arg.mode}`)
            }
          }

          const request = params[arg.mode]
          if (!request) {
            throw new Error(
              `unable to find ${arg.mode || 'MODE'} with Id: ${arg.id}`
            )
          }

          const { result } = await simulationService.hasTransactionScanSupport(
            request.chainId,
            CoinTypes.SOL
          )

          if (!result) {
            throw new Error(
              `Solana transaction simulation not supported for chain/coin: ${
                request.chainId //
              }/${CoinTypes.SOL}`
            )
          }

          const { errorResponse, errorString, response } =
            await simulationService.scanSolanaTransaction(
              toMojoUnion(params, arg.mode),
              navigator.language
            )

          if (errorResponse || errorString) {
            const error = `scanSolanaTransaction({${JSON.stringify(
              toMojoUnion(params, arg.mode)
            )}}) failed -- ${errorString}: ${errorResponse}`
            throw new Error(error)
          }

          if (!response) {
            throw new Error('empty Solana simulation response')
          }

          return {
            data: response
          }
        } catch (error) {
          console.error(error)
          const message = `Failed to simulate Solana transaction: ${
            error?.message || error
          }`
          console.log(message)
          return {
            error: message
          }
        }
      }
    })
  }
}
