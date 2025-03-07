/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { Reducer } from 'redux'

import { types } from '../constants/playlist_types'
import { CachingProgress, PlaylistData } from './states'

import { Playlist } from 'gen/brave/components/playlist/common/mojom/playlist.mojom.m.js'

import { fixUpMediaURL, fixUpThumbnailURL } from '../utils/urlFixer'

const playlistReducer: Reducer<PlaylistData | undefined> = (
  state: PlaylistData | undefined,
  action
) => {
  if (state === undefined) {
    state = {
      lists: [],
      currentList: undefined,
      lastPlayerState: undefined,
      cachingProgress: new Map()
    }
  }

  switch (action.type) {
    case types.PLAYLIST_LOADED:
      const playlists = action.payload as Playlist[]

      // Fix up local thumbnail url to chrome-untrusted
      if (location.protocol === 'chrome-untrusted:') {
        for (const playlist of playlists) {
          for (const item of playlist.items) {
            fixUpThumbnailURL(item)
            fixUpMediaURL(item)
          }
        }
      }

      let currentList: Playlist | undefined
      if (state.currentList) {
        currentList = playlists.find((list: Playlist) => {
          return list.id === state?.currentList?.id
        })
      }
      if (!currentList) currentList = playlists[0]

      state = { ...state, currentList, lists: [...playlists] }
      break

    case types.PLAYLIST_SELECTED:
      const playlist = action.payload
      state = { ...state, currentList: playlist }
      break

    case types.PLAYLIST_PLAYER_STATE_CHANGED:
      state = { ...state, lastPlayerState: action.payload }
      break

    case types.PLAYLIST_CACHING_PROGRESS_CHANGED:
      const cachingProgress = action.payload as CachingProgress
      const newCachingProgress = new Map<string, CachingProgress>(
        state.cachingProgress
      )

      if (cachingProgress.percentComplete === 100) {
        newCachingProgress.delete(cachingProgress.id)
      } else {
        newCachingProgress.set(cachingProgress.id, cachingProgress)
      }

      state = { ...state, cachingProgress: newCachingProgress }
      break
  }
  return state
}

export default playlistReducer
