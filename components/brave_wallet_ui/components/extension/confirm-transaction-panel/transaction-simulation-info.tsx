// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
/* eslint-disable @typescript-eslint/key-spacing */

import * as React from 'react'
import { color } from '@brave/leo/tokens/css'

// types & magics
import {
  BlockExplorerUrlTypes,
  BraveWallet,
  MetaplexTokenStandard,
  SafeBlowfishEvmResponse,
  SafeBlowfishSolanaResponse,
  SafeEVMStateChange,
  SafeEvmApprovalEvent,
  SafeEvmTransferEvent,
  SafeSolTransferEvent,
  SafeSolanaStakeChangeEvent,
  SafeSolanaStateChange,
  SafeSplTransferEvent
} from '../../../constants/types'
import {
  BLOWFISH_UNLIMITED_VALUE,
  NATIVE_ASSET_CONTRACT_ADDRESS_0X
} from '../../../common/constants/magics'

// utils
import Amount from '../../../utils/amount'
import { getLocale } from '../../../../common/locale'
import { reduceAddress } from '../../../utils/reduce-address'
import { isDataURL } from '../../../utils/string-utils'
import withPlaceholderIcon from '../../shared/create-placeholder-icon'

// hooks
import {
  useSelectedPendingTransaction //
} from '../../../common/hooks/use-pending-transaction'
import useExplorer from '../../../common/hooks/explorer'
import { useGetTokensRegistryQuery } from '../../../common/slices/api.slice'

// components
import { CopyTooltip } from '../../shared/copy-tooltip/copy-tooltip'
import { NftIcon } from '../../shared/nft-icon/nft-icon'

// style
import {
  InlineAddressButton,
  TransactionTitle,
  TransactionTypeText
} from './style'
import {
  groupSimulatedEVMStateChanges,
  decodeSimulatedSVMStateChanges,
  decodeSimulatedTxResponseActionsAndWarnings
} from '../../../utils/tx-simulation-utils'
import {
  ArrowRightIcon,
  StateChangeText,
  UnverifiedTokenIndicator
} from './transaction-simulation-info.style'
import {
  AssetIconFactory,
  AssetIconProps,
  Column,
  IconsWrapper,
  NetworkIconWrapper,
  Row,
  Text
} from '../../shared/style'
import {
  CollapseHeaderDivider,
  Divider,
  LaunchIcon,
  TransactionChangeCollapse,
  TransactionChangeCollapseContainer,
  TransactionChangeCollapseContent
} from './confirm-simulated-tx-panel.styles'


const assetIconSizeProps = {
  width: '24px',
  height: 'auto'
}

const NFT_ICON_STYLE = { width: 'auto', height: 24 }

export const AssetIcon = AssetIconFactory<AssetIconProps>(assetIconSizeProps)

const AssetIconWithPlaceholder = withPlaceholderIcon(AssetIcon, {
  size: 'small',
  marginLeft: 0,
  marginRight: 8
})

const NftAssetIconWithPlaceholder = withPlaceholderIcon(NftIcon, {
  size: 'small',
  marginLeft: 0,
  marginRight: 8
})

type TransactionInfoProps = (
  | {
      simulationType: 'EVM'
      simulation: BraveWallet.EVMSimulationResponse | SafeBlowfishEvmResponse
    }
  | {
      simulationType: 'SVM'
      simulation:
        | BraveWallet.SolanaSimulationResponse
        | SafeBlowfishSolanaResponse
    }
) & {
  network: Pick<
    BraveWallet.NetworkInfo,
    | 'chainId'
    | 'symbol'
    | 'iconUrls'
    | 'chainName'
    | 'blockExplorerUrls'
    | 'coin'
  >
}

function InlineViewOnBlockExplorerIconButton({
  address,
  id,
  network,
  urlType
}: {
  address: string
  id?: string | undefined
  network: Pick<
    BraveWallet.NetworkInfo,
    'chainId' | 'symbol' | 'iconUrls' | 'chainName' | 'blockExplorerUrls'
  >
  urlType: BlockExplorerUrlTypes
}) {
  // custom hooks
  const onClickViewOnBlockExplorer = useExplorer(network)

  return (
    <InlineAddressButton
      style={{ marginLeft: '4px' }}
      title={
        'view on block explorer' // TODO: locale
      }
      onClick={onClickViewOnBlockExplorer(urlType, address, id)}
    >
      <LaunchIcon />
    </InlineAddressButton>
  )
}

const NativeAssetOrErc20TokenTransfer = ({
  network,
  transfer
}: {
  transfer:
    | BraveWallet.BlowfishERC20TransferData
    | BraveWallet.BlowfishNativeAssetTransferData
  network: Pick<
    BraveWallet.NetworkInfo,
    'chainId' | 'symbol' | 'iconUrls' | 'chainName' | 'blockExplorerUrls'
  >
}): JSX.Element => {
  // memos
  const asset = React.useMemo(() => {
    return {
      contractAddress: transfer.asset.address,
      isErc721: false,
      isNft: false,
      logo: transfer.asset.imageUrl || '',
      name: transfer.asset.name,
      symbol: transfer.asset.symbol
    }
  }, [transfer.asset])

  const normalizedAmount = React.useMemo(() => {
    return new Amount(transfer.amount.after)
      .minus(transfer.amount.before)
      .divideByDecimals(transfer.asset.decimals)
  }, [transfer])

  // computed
  const isReceive = normalizedAmount.isPositive()
  const isNativeAsset =
    transfer.asset.address === NATIVE_ASSET_CONTRACT_ADDRESS_0X ||
    transfer.asset.address === ''

  // render
  return (
    <Column alignItems='flex-start' padding={0} margin={'0px 0px 8px 0px'}>
      <Text textSize='12px' color={color.text.secondary}>
        {
          isReceive ? 'Receive' : 'Send' // TODO: locale
        }
      </Text>
      <Row
        margin={'4px 0px 0px 0px'}
        alignItems='center'
        justifyContent='flex-start'
      >
        <IconsWrapper
          marginRight='0px'
          title={
            // TODO: locale
            transfer.asset.verified && transfer.asset.lists.length > 0
              ? `This token is verified on ${transfer.asset.lists.length} lists`
              : transfer.asset.verified
              ? 'This token is verified'
              : 'This token is unverified'
          }
        >
          <AssetIconWithPlaceholder asset={asset} network={network} />
          {!transfer.asset.verified && (
            <NetworkIconWrapper>
              <UnverifiedTokenIndicator />
            </NetworkIconWrapper>
          )}
        </IconsWrapper>

        <Row alignItems='center' gap={'4px'} justifyContent='flex-start'>
          <StateChangeText
            color={isReceive ? color.systemfeedback.successIcon : undefined}
          >
            <strong>
              {isReceive ? ' + ' : ' - '}
              {normalizedAmount
                .toAbsoluteValue()
                .formatAsAsset(6, transfer.asset.symbol)}
            </strong>
          </StateChangeText>
          {!isNativeAsset && (
            <InlineViewOnBlockExplorerIconButton
              address={asset.contractAddress}
              network={network}
              urlType='token'
            />
          )}
        </Row>
      </Row>
    </Column>
  )
}

const Erc721TokenTransfer = ({
  transfer,
  network
}: {
  transfer: BraveWallet.BlowfishERC721TransferData
  network: Pick<
    BraveWallet.NetworkInfo,
    'chainId' | 'symbol' | 'iconUrls' | 'chainName' | 'blockExplorerUrls'
  >
}): JSX.Element => {
  // memos
  const asset = React.useMemo(() => {
    return {
      contractAddress: transfer.contract.address,
      isErc721: false,
      isNft: false,
      logo: transfer.metadata.rawImageUrl || '',
      name: transfer.name,
      symbol: transfer.symbol,
      tokenId: transfer.tokenId
    }
  }, [transfer])

  const NftAssetIconWithPlaceholder = React.useMemo(() => {
    return withPlaceholderIcon(
      (!isDataURL(asset.logo) ? NftIcon : AssetIcon) as typeof NftIcon,
      {
        size: 'small',
        marginLeft: 0,
        marginRight: 8
      }
    )
  }, [asset.logo])

  const isReceive = React.useMemo(() => {
    return new Amount(transfer.amount.after).gt(transfer.amount.before)
  }, [transfer.amount])

  // render
  return (
    <Column alignItems='flex-start' padding={0} margin={'0px 0px 8px 0px'}>
      <Text textSize='12px' color={color.text.secondary}>
        {
          isReceive ? 'Receive' : 'Send' // TODO: locale
        }
      </Text>
      <Row
        margin={'4px 0px 0px 0px'}
        alignItems='center'
        justifyContent='flex-start'
      >
        <IconsWrapper marginRight='0px'>
          <NftAssetIconWithPlaceholder
            asset={asset}
            network={network}
            iconStyles={NFT_ICON_STYLE}
          />
          {/* {!isVerified && (
              <NetworkIconWrapper>
                <UnverifiedTokenIndicator />
              </NetworkIconWrapper>
            )} */}
        </IconsWrapper>
        <Row alignItems='center' gap={'4px'} justifyContent='flex-start'>
          <StateChangeText
            color={isReceive ? color.systemfeedback.successIcon : undefined}
          >
            <strong>
              {isReceive ? '+' : '-'} {asset.name}{' '}
              {asset.tokenId
                ? `#${new Amount(asset.tokenId).toNumber()}`
                : null}
            </strong>
          </StateChangeText>
          <InlineViewOnBlockExplorerIconButton
            address={asset.contractAddress}
            id={asset.tokenId}
            network={network}
            urlType='nft'
          />
        </Row>
      </Row>
    </Column>
  )
}

const Erc1155TokenTransfer = ({
  network,
  transfer,
  humanReadableDiff
}: {
  network: Pick<
    BraveWallet.NetworkInfo,
    | 'chainId'
    | 'symbol'
    | 'iconUrls'
    | 'chainName'
    | 'coin'
    | 'blockExplorerUrls'
  >
  transfer: BraveWallet.BlowfishERC1155TransferData
  humanReadableDiff?: string
}): JSX.Element => {
  // memos
  const asset = React.useMemo(() => {
    return {
      contractAddress: transfer.contract.address,
      isErc721: !!transfer.tokenId,
      isNft: !!transfer.tokenId,
      logo: transfer.metadata.rawImageUrl || '',
      tokenId: transfer.tokenId,
      symbol: transfer.name, // TODO: How to get symbol?
      name: transfer.name
    }
  }, [transfer])

  const isReceive = React.useMemo(() => {
    return new Amount(transfer.amount.after).gt(transfer.amount.before)
  }, [transfer.amount])

  // render
  return (
    <Column alignItems='flex-start' padding={0} margin={'0px 0px 8px 0px'}>
      <Text textSize='12px' color={color.text.secondary}>
        {
          isReceive ? 'Receive' : 'Send' // TODO: locale
        }
      </Text>
      <Row
        margin={'4px 0px 0px 0px'}
        alignItems='center'
        justifyContent='flex-start'
      >
        <IconsWrapper>
          {asset.isNft ? (
            <NftAssetIconWithPlaceholder
              asset={asset}
              network={network}
              iconStyles={NFT_ICON_STYLE}
            />
          ) : (
            <AssetIconWithPlaceholder asset={asset} network={network} />
          )}
        </IconsWrapper>

        <Row alignItems='center' gap={'4px'} justifyContent='flex-start'>
          <StateChangeText
            color={isReceive ? color.systemfeedback.successIcon : undefined}
          >
            <strong>
              {isReceive ? ' + ' : ' - '}
              {asset?.isNft
                ? asset.name
                : new Amount(transfer.amount.after)
                    .minus(transfer.amount.before)
                    .toAbsoluteValue()
                    .formatAsAsset(
                      6,
                      asset.name || asset.tokenId ? `${asset.tokenId}` : '???'
                    )}
            </strong>
          </StateChangeText>

          <InlineViewOnBlockExplorerIconButton
            address={asset.contractAddress}
            id={asset.tokenId}
            network={network}
            urlType={asset.isNft ? 'nft' : 'token'}
          />
        </Row>
      </Row>
    </Column>
  )
}

const SPLTokenTransfer = ({
  network,
  transfer
}: {
  transfer: BraveWallet.BlowfishSPLTransferData
  network: Pick<
    BraveWallet.NetworkInfo,
    'chainId' | 'symbol' | 'iconUrls' | 'chainName' | 'blockExplorerUrls'
  >
}): JSX.Element => {
  const { assetIsKnown, assetLogo = '' } = useGetTokensRegistryQuery(
    undefined,
    {
      selectFromResult: (res) => {
        const assetId = res.data?.idsByChainId[network.chainId].find((id) =>
          id.toString().includes(transfer.mint)
        )
        const asset = assetId ? res.data?.entities[assetId] : undefined
        return {
          isLoading: res.isLoading,
          assetIsKnown: Boolean(asset),
          assetLogo: asset?.logo
        }
      }
    }
  )

  const asset: Pick<
    BraveWallet.BlockchainToken,
    'symbol' | 'name' | 'contractAddress' | 'isErc721' | 'isNft' | 'logo'
  > = {
    contractAddress: transfer.mint,
    isErc721: false,
    logo: assetLogo,
    name: transfer.name,
    symbol: transfer.symbol,
    isNft:
      (transfer.metaplexTokenStandard as MetaplexTokenStandard) ===
        'non_fungible' ||
      (transfer.metaplexTokenStandard as MetaplexTokenStandard) ===
        'non_fungible_edition'
  }

  // memos
  const normalizedAmount = React.useMemo(() => {
    return new Amount(transfer.diff.digits.toString()).divideByDecimals(
      transfer.decimals
    )
  }, [transfer])

  // computed
  const isReceive = transfer.diff.sign.toLowerCase() !== 'minus'
  const isVerified = assetIsKnown

  // render
  return (
    <Column alignItems='flex-start' padding={0} margin={'0px 0px 8px 0px'}>
      <Text textSize='12px' color={color.text.secondary}>
        {
          isReceive ? 'Receive' : 'Send' // TODO: locale
        }
      </Text>
      <Row
        margin={'4px 0px 0px 0px'}
        alignItems='center'
        justifyContent='flex-start'
      >
        <IconsWrapper
          marginRight='0px'
          title={
            // TODO: locale
            isVerified
              ? 'This token is verified is 1 list'
              : 'This token is unverified'
          }
        >
          {/* TODO: NFT ICON PLACEHOLDER */}
          {asset.isNft ? (
            <NftAssetIconWithPlaceholder
              asset={asset}
              network={network}
              iconStyles={NFT_ICON_STYLE}
            />
          ) : (
            <AssetIconWithPlaceholder asset={asset} network={network} />
          )}
          {!isVerified && (
            <NetworkIconWrapper>
              <UnverifiedTokenIndicator />
            </NetworkIconWrapper>
          )}
        </IconsWrapper>

        <Row alignItems='center' gap={'4px'} justifyContent='flex-start'>
          <StateChangeText
            color={isReceive ? color.systemfeedback.successIcon : undefined}
          >
            <strong>
              {isReceive ? ' + ' : ' - '}
              {asset.isNft
                ? asset.name
                : normalizedAmount
                    .toAbsoluteValue()
                    .formatAsAsset(6, asset.symbol)}
            </strong>
          </StateChangeText>

          <InlineViewOnBlockExplorerIconButton
            address={asset.contractAddress}
            network={network}
            urlType={'contract'}
          />
        </Row>
      </Row>
    </Column>
  )
}

const SOLTransfer = ({
  network,
  transfer
}: {
  transfer: BraveWallet.BlowfishSOLTransferData
  network: Pick<
    BraveWallet.NetworkInfo,
    'chainId' | 'symbol' | 'iconUrls' | 'chainName' | 'blockExplorerUrls'
  >
}): JSX.Element => {
  // memos
  const asset = React.useMemo(() => {
    return {
      contractAddress: '',
      isErc721: false,
      isNft: false,
      logo: network.iconUrls[0],
      name: transfer.name,
      symbol: transfer.symbol
    }
  }, [transfer, network])

  const normalizedAmount = React.useMemo(() => {
    return new Amount(transfer.diff.digits.toString()).divideByDecimals(
      transfer.decimals
    )
  }, [transfer])

  // computed
  const isReceive = transfer.diff.sign.toLowerCase() !== 'minus'
  const isVerified = true // TODO: look at our tokens list

  // render
  return (
    <Column alignItems='flex-start' padding={0} margin={'0px 0px 8px 0px'}>
      <Text textSize='12px' color={color.text.secondary}>
        {
          isReceive ? 'Receive' : 'Send' // TODO: locale
        }
      </Text>
      <Row
        margin={'4px 0px 0px 0px'}
        alignItems='center'
        justifyContent='flex-start'
      >
        <IconsWrapper
          marginRight='0px'
          title={
            // TODO: locale
            isVerified ? 'This token is verified' : 'This token is unverified'
          }
        >
          <AssetIconWithPlaceholder asset={asset} network={network} />
          {!isVerified && (
            <NetworkIconWrapper>
              <UnverifiedTokenIndicator />
            </NetworkIconWrapper>
          )}
        </IconsWrapper>
        <Row alignItems='center' gap={'4px'} justifyContent='flex-start'>
          <StateChangeText
            color={isReceive ? color.systemfeedback.successIcon : undefined}
          >
            <strong>
              {isReceive ? ' + ' : ' - '}
              {normalizedAmount
                .toAbsoluteValue()
                .formatAsAsset(6, transfer.symbol)}
            </strong>
          </StateChangeText>
        </Row>
      </Row>
    </Column>
  )
}

/**
 * Works for ERC20, ERC721, ERC1155
 */
const TokenApproval = ({
  approval,
  isERC20,
  network,
  isApprovalForAll
}: {
  network: Pick<
    BraveWallet.NetworkInfo,
    'chainId' | 'symbol' | 'iconUrls' | 'chainName' | 'blockExplorerUrls'
  >
} & (
  | {
      isERC20?: undefined | false
      isApprovalForAll?: undefined | boolean
      approval:
        | BraveWallet.BlowfishERC721ApprovalData
        | BraveWallet.BlowfishERC721ApprovalForAllData
        | BraveWallet.BlowfishERC1155ApprovalForAllData
    }
  | {
      isERC20: true
      approval: BraveWallet.BlowfishERC20ApprovalData
      isApprovalForAll?: undefined | false
    }
)): JSX.Element => {
  // memos
  /**
   * Not available for `BraveWallet.BlowfishERC1155ApprovalForAllData`
   */
  const assetSymbol = React.useMemo(() => {
    if (isERC20) {
      return approval?.asset?.symbol || undefined
    }

    return (
      (
        approval as
          | BraveWallet.BlowfishERC721ApprovalForAllData
          | BraveWallet.BlowfishERC721ApprovalData
      )?.symbol ?? undefined
    )
  }, [approval, isERC20])

  const beforeAmount = React.useMemo(() => {
    const before = new Amount(approval.amount.before)

    // TODO: fix UNLIMITED display
    if (
      before.gte(BLOWFISH_UNLIMITED_VALUE) ||
      (before.gt(0) && isApprovalForAll)
    ) {
      return 'Unlimited' // TODO: locale
    }

    if (isERC20) {
      return before
        .divideByDecimals(approval.asset.decimals)
        .formatAsAsset(6, assetSymbol)
    }
    return before.formatAsAsset(6, assetSymbol)
  }, [approval, isERC20])

  const afterAmount = React.useMemo(() => {
    const after = new Amount(approval.amount.after)

    if (
      after.gte(BLOWFISH_UNLIMITED_VALUE) ||
      (after.gt(0) && isApprovalForAll)
    ) {
      return 'Unlimited' // TODO: locale
    }

    if (isERC20) {
      return after
        .divideByDecimals(approval.asset.decimals)
        .formatAsAsset(6, assetSymbol)
    }
    return after.formatAsAsset(6, assetSymbol)
  }, [approval, isERC20, assetSymbol, isApprovalForAll])

  return (
    // TODO: Approval styles / component
    <Column
      style={{ color: 'white' }}
      margin={'0px 0px 6px 0px'}
      alignItems='flex-start'
      justifyContent='center'
    >
      <Row gap={'4px'} alignItems='center' justifyContent='flex-start'>
        <StateChangeText>
          <span>
            {/* TODO: locale */}
            {'From'}
          </span>
          <strong>{beforeAmount}</strong>

          <ArrowRightIcon />

          <span>
            {/* TODO: locale */}
            {'To'}
          </span>
          <strong>{afterAmount}</strong>
        </StateChangeText>
      </Row>
      <Row alignItems='center' justifyContent='flex-start'>
        <CopyTooltip
          isAddress
          text={approval.spender.address}
          tooltipText={approval.spender.address}
          position='left'
          verticalPosition='below'
        >
          <Text textSize='11px'>
            {/* TODO */}
            Spender: {reduceAddress(approval.spender.address)}{' '}
          </Text>
        </CopyTooltip>

        <InlineViewOnBlockExplorerIconButton
          address={approval.spender.address}
          network={network}
          urlType={'address'}
        />
      </Row>
    </Column>
  )
}

function SolStakingAuthChange({
  approval,
  network
}: {
  approval: SafeSolanaStakeChangeEvent
  network: Pick<
    BraveWallet.NetworkInfo,
    'chainId' | 'symbol' | 'iconUrls' | 'chainName' | 'blockExplorerUrls'
  >
}) {
  // computed from props
  const changedData = approval.rawInfo.data.solStakeAuthorityChangeData
  const hasStakerChange =
    changedData.currAuthorities.staker !== changedData.futureAuthorities.staker
  const hasWithdrawerChange =
    changedData.currAuthorities.withdrawer !==
    changedData.futureAuthorities.withdrawer

  const AddressChange = React.useCallback(
    ({
      fromAddress,
      toAddress
    }: {
      fromAddress: string
      toAddress: string
    }) => {
      return (
        <Row
          alignItems='center'
          justifyContent='flex-start'
          padding={'8px 0px'}
        >
          <StateChangeText>
            {/* TODO: locale */}
            <strong>
              {reduceAddress(fromAddress)}
              <InlineViewOnBlockExplorerIconButton
                address={fromAddress}
                network={network}
                urlType='address'
              />
            </strong>

            <ArrowRightIcon />

            <span>
              {/* TODO: locale */}
              {'To '}
              <strong>
                {reduceAddress(toAddress)}
                <InlineViewOnBlockExplorerIconButton
                  address={toAddress}
                  network={network}
                  urlType='address'
                />
              </strong>
            </span>
          </StateChangeText>
        </Row>
      )
    },
    [network]
  )

  // render
  return (
    // TODO: Approval styles / component
    <Column
      style={{ color: 'white' }}
      margin={'0px 0px 6px 0px'}
      alignItems='flex-start'
      justifyContent='center'
    >
      {hasStakerChange ? (
        <Column justifyContent='center' alignItems='flex-start'>
          <Row alignItems='center' justifyContent='flex-start'>
            <StateChangeText>
              {
                // TODO: locale
                'Staker'
              }
            </StateChangeText>
          </Row>
          <AddressChange
            fromAddress={changedData.currAuthorities.staker}
            toAddress={changedData.futureAuthorities.staker}
          />
        </Column>
      ) : null}

      {hasWithdrawerChange ? (
        <Column justifyContent='center' alignItems='flex-start'>
          <Row alignItems='center' justifyContent='flex-start'>
            <StateChangeText>
              {
                // TODO: locale
                'Withdrawer'
              }
            </StateChangeText>
          </Row>
          <AddressChange
            fromAddress={changedData.currAuthorities.withdrawer}
            toAddress={changedData.futureAuthorities.withdrawer}
          />
        </Column>
      ) : null}
    </Column>
  )
}

export const TransactionSimulationInfo = ({
  simulation,
  simulationType,
  network
}: TransactionInfoProps) => {
  const { simulationResults } =
    decodeSimulatedTxResponseActionsAndWarnings(simulation)
  const { expectedStateChanges } = simulationResults

  // custom hooks
  const tx = useSelectedPendingTransaction()
  const sendOptions = tx?.txDataUnion.solanaTxData?.sendOptions

  // memos
  const { evmChanges, svmChanges } = React.useMemo(() => {
    if (simulationType === 'EVM') {
      return {
        evmChanges: groupSimulatedEVMStateChanges(
          expectedStateChanges as SafeEVMStateChange[]
        ),
        svmChanges: undefined
      }
    }
    if (simulationType === 'SVM') {
      return {
        svmChanges: decodeSimulatedSVMStateChanges(
          expectedStateChanges as SafeSolanaStateChange[]
        ),
        evmChanges: undefined
      }
    }
    return {
      evmChanges: undefined,
      svmChanges: undefined
    }
  }, [expectedStateChanges, simulationType])

  // computed
  const hasApprovals = Boolean(
    evmChanges?.evmApprovals.length || svmChanges?.splApprovals.length
  )

  const hasTransfers = Boolean(
    evmChanges?.evmTransfers.length || svmChanges?.svmTransfers.length
  )

  const hasSolStakingAuthChanges = Boolean(
    svmChanges?.svmStakeAuthorityChanges.length
  )

  const hasMultipleCategories =
    [hasApprovals, hasTransfers, hasSolStakingAuthChanges].filter(Boolean)
      .length > 1

  // state
  const [isTransfersSectionOpen, setTransfersSectionOpen] = React.useState(true)
  const [isApprovalsSectionOpen, setIsApprovalsSectionOpen] = React.useState(
    !hasMultipleCategories
  )
  const [isSolStakingAuthSectionOpen, setIsSolStakingAuthSectionOpen] =
    React.useState(!hasMultipleCategories)

  // methods
  const onToggleTransfersSection = React.useCallback(() => {
    if (!hasMultipleCategories) {
      return
    }

    if (isTransfersSectionOpen) {
      setTransfersSectionOpen(false)
      return
    }

    setTransfersSectionOpen(true)
    setIsApprovalsSectionOpen(false)
    setIsSolStakingAuthSectionOpen(false)
  }, [isTransfersSectionOpen, hasMultipleCategories])

  const onToggleApprovalsSection = React.useCallback(() => {
    if (!hasMultipleCategories) {
      return
    }

    if (isApprovalsSectionOpen) {
      setIsApprovalsSectionOpen(false)
      return
    }

    setIsApprovalsSectionOpen(true)

    setTransfersSectionOpen(false)
    setIsSolStakingAuthSectionOpen(false)
  }, [isApprovalsSectionOpen, hasMultipleCategories])

  const onToggleSolStakingAuthSection = React.useCallback(() => {
    if (!hasMultipleCategories) {
      return
    }

    if (isSolStakingAuthSectionOpen) {
      setIsSolStakingAuthSectionOpen(false)
      return
    }

    setIsSolStakingAuthSectionOpen(true)

    setTransfersSectionOpen(false)
    setIsApprovalsSectionOpen(false)
  }, [isSolStakingAuthSectionOpen, hasMultipleCategories])

  // render
  return (
    <TransactionChangeCollapseContainer
      hasMultipleCategories={hasMultipleCategories}
    >
      {/* Transferred Assets */}
      {hasTransfers ? (
        <TransactionChangeCollapse
          onToggle={onToggleTransfersSection}
          hasMultipleCategories={hasMultipleCategories}
          title={
            'Balance Changes' // TODO: locale
          }
          isOpen={isTransfersSectionOpen}
          key={'transfers'}
        >
          <TransactionChangeCollapseContent>
            <CollapseHeaderDivider key={'Transfers-Divider'} />

            {evmChanges?.evmTransfers.map((transfer, i, arr) => {
              return (
                <React.Fragment key={'EVM-Transfer-' + i}>
                  {getComponentForEvmTransfer(transfer, network)}
                  {i < arr.length - 1 ? <Divider /> : null}
                </React.Fragment>
              )
            })}

            {svmChanges?.svmTransfers.map((transfer, i, arr) => (
              <React.Fragment key={'SVM-Transfer' + i}>
                {getComponentForSvmTransfer(transfer, network)}
                {i < arr.length - 1 ? <Divider /> : null}
              </React.Fragment>
            ))}
          </TransactionChangeCollapseContent>
        </TransactionChangeCollapse>
      ) : null}

      {hasApprovals && (
        <TransactionChangeCollapse
          onToggle={onToggleApprovalsSection}
          hasMultipleCategories={hasMultipleCategories}
          isOpen={isApprovalsSectionOpen}
          title={
            'Approvals' // TODO: locale
          }
          key='approvals'
        >
          <TransactionChangeCollapseContent>
            <CollapseHeaderDivider key={'EVM-Approvals-Divider'} />

            {evmChanges?.evmApprovals.map((approval, i, arr) => (
              <React.Fragment key={'EVM-Token-Approval-' + i}>
                {getComponentForEvmApproval(approval, network)}
                {i < arr.length - 1 ? <Divider /> : null}
              </React.Fragment>
            ))}
          </TransactionChangeCollapseContent>
        </TransactionChangeCollapse>
      )}

      {hasSolStakingAuthChanges && (
        <TransactionChangeCollapse
          onToggle={onToggleSolStakingAuthSection}
          hasMultipleCategories={hasMultipleCategories}
          isOpen={isSolStakingAuthSectionOpen}
          title={
            'SOL Staking changes' // TODO: locale
          }
          key='SOL-staking-changes'
        >
          <TransactionChangeCollapseContent>
            <CollapseHeaderDivider key={'SolStakingAuthChanges-Divider'} />
            {svmChanges?.svmStakeAuthorityChanges.map((approval, i, arr) => (
              <React.Fragment key={'SolStakingAuthChanges-' + i}>
                <SolStakingAuthChange approval={approval} network={network} />
                {i < arr.length - 1 ? <Divider /> : null}
              </React.Fragment>
            ))}
          </TransactionChangeCollapseContent>
        </TransactionChangeCollapse>
      )}

      {/* SEND OPTIONS */}
      {sendOptions && (
        <Column margin={'16px 4px 0px 4px'}>
          {!!Number(sendOptions?.maxRetries?.maxRetries) && (
            <Row justifyContent='flex-start' gap={'4px'}>
              <TransactionTitle>
                {getLocale('braveWalletSolanaMaxRetries')}
              </TransactionTitle>
              <TransactionTypeText>
                {Number(sendOptions?.maxRetries?.maxRetries)}
              </TransactionTypeText>
            </Row>
          )}

          {sendOptions?.preflightCommitment && (
            <Row justifyContent='flex-start' gap={'4px'}>
              <TransactionTitle>
                {getLocale('braveWalletSolanaPreflightCommitment')}
              </TransactionTitle>
              <TransactionTypeText>
                {sendOptions.preflightCommitment}
              </TransactionTypeText>
            </Row>
          )}

          {sendOptions?.skipPreflight && (
            <Row justifyContent='flex-start' gap={'4px'}>
              <TransactionTitle>
                {getLocale('braveWalletSolanaSkipPreflight')}
              </TransactionTitle>
              <TransactionTypeText>
                {sendOptions.skipPreflight.skipPreflight.toString()}
              </TransactionTypeText>
            </Row>
          )}
        </Column>
      )}
    </TransactionChangeCollapseContainer>
  )
}

function getComponentForEvmApproval(
  approval: SafeEvmApprovalEvent,
  network: Pick<
    BraveWallet.NetworkInfo,
    | 'symbol'
    | 'chainId'
    | 'iconUrls'
    | 'chainName'
    | 'blockExplorerUrls'
    | 'coin'
  >
) {
  switch (approval.rawInfo.kind) {
    case 'ERC20_APPROVAL': {
      return (
        <TokenApproval
          key={approval.humanReadableDiff}
          approval={approval.rawInfo.data.erc20ApprovalData}
          network={network}
          isERC20={true}
        />
      )
    }
    case 'ERC1155_APPROVAL_FOR_ALL': {
      return (
        <TokenApproval
          key={approval.humanReadableDiff}
          approval={approval.rawInfo.data.erc1155ApprovalForAllData}
          network={network}
          isApprovalForAll={true}
        />
      )
    }
    case 'ERC721_APPROVAL': {
      return (
        <TokenApproval
          key={approval.humanReadableDiff}
          approval={approval.rawInfo.data.erc721ApprovalData}
          network={network}
        />
      )
    }
    case 'ERC721_APPROVAL_FOR_ALL': {
      return (
        <TokenApproval
          key={approval.humanReadableDiff}
          approval={approval.rawInfo.data.erc721ApprovalForAllData}
          network={network}
          isApprovalForAll={true}
        />
      )
    }
  }
}

function getComponentForSvmTransfer(
  transfer: SafeSolTransferEvent | SafeSplTransferEvent,
  network: Pick<
    BraveWallet.NetworkInfo,
    | 'symbol'
    | 'chainId'
    | 'iconUrls'
    | 'chainName'
    | 'blockExplorerUrls'
    | 'coin'
  >
) {
  return transfer.rawInfo.kind === 'SOL_TRANSFER' ? (
    <SOLTransfer
      key={transfer.humanReadableDiff}
      transfer={transfer.rawInfo.data.solTransferData}
      network={network}
    />
  ) : (
    <SPLTokenTransfer
      key={transfer.humanReadableDiff}
      transfer={transfer.rawInfo.data.splTransferData}
      network={network}
    />
  )
}

function getComponentForEvmTransfer(
  transfer: SafeEvmTransferEvent,
  network: Pick<
    BraveWallet.NetworkInfo,
    | 'symbol'
    | 'chainId'
    | 'iconUrls'
    | 'chainName'
    | 'blockExplorerUrls'
    | 'coin'
  >
) {
  const { kind, data } = transfer.rawInfo

  switch (kind) {
    case 'ERC1155_TRANSFER':
      return (
        <Erc1155TokenTransfer
          humanReadableDiff={transfer.humanReadableDiff}
          key={transfer.humanReadableDiff}
          transfer={data.erc1155TransferData}
          network={network}
        />
      )
    case 'ERC20_TRANSFER':
      return (
        <NativeAssetOrErc20TokenTransfer
          key={transfer.humanReadableDiff}
          transfer={data.erc20TransferData}
          network={network}
        />
      )
    case 'NATIVE_ASSET_TRANSFER':
      return (
        <NativeAssetOrErc20TokenTransfer
          key={transfer.humanReadableDiff}
          transfer={data.nativeAssetTransferData}
          network={network}
        />
      )
    case 'ERC721_TRANSFER':
      return (
        <Erc721TokenTransfer
          key={transfer.humanReadableDiff}
          transfer={data.erc721TransferData}
          network={network}
        />
      )
  }
}
