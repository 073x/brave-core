/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { useSelector } from 'react-redux'

import {
  Playlist,
  PlaylistItem
} from 'gen/brave/components/playlist/common/mojom/playlist.mojom.m.js'

import { getItemDurationInSeconds } from '../utils/timeFormatter'
import { getFormattedTotalBytes } from '../utils/bytesFormatter'

// For security reason, we're embedding player to an <iframe>. And these two
// states are mutually exclusive.
export interface ApplicationState {
  // Used by app.tsx
  playlistData: PlaylistData | undefined

  // Used by a player.tsx
  playerState: PlayerState | undefined
}

export interface CachingProgress {
  id: string
  totalBytes: bigint
  receivedBytes: bigint
  percentComplete: number
  timeRemaining: string
}

export interface PlaylistData {
  lists: Playlist[]
  currentList: Playlist | undefined
  // TODO(sko) Investigate if it's possible to remove this and use ApplicationState.playerState.
  lastPlayerState: PlayerState | undefined

  cachingProgress: Map<string, CachingProgress>
}

export interface PlayerState {
  currentItem: PlaylistItem | undefined
  playing: boolean
}

export const usePlaylist = (id?: string) =>
  useSelector<ApplicationState, Playlist | undefined>(applicationState =>
    applicationState.playlistData?.lists.find(e => e.id === id)
  )

export const useTotalDuration = (playlist?: Playlist) => {
  return React.useMemo(() => {
    // TODO(sko) Duration value could be missing. Only Youtube could work.
    //  * We need to update duration when Playlist player plays a video
    return playlist?.items?.reduce((sum, item) => {
      return sum + getItemDurationInSeconds(item)
    }, 0)
  }, [playlist])
}

export function useTotalSize (playlist?: Playlist) {
  return React.useMemo(() => {
    return getFormattedTotalBytes(playlist?.items ?? [])
  }, [playlist])
}
