// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css'

import { AlertType } from '../../../constants/types'

import { LeoColors } from './leo-colors'

export const InlineAlertContainer = styled.div<{
  alertType: AlertType
  gap?: string
  padding?: string
  minHeight?: string
}>`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: flex-start;
  gap: ${(p) => p?.gap ?? '16px'};

  margin-top: 16px;
  margin-bottom: 16px;
  padding: ${(p) => p?.padding ?? '16px'};
  width: 100%;
  min-height: ${(p) => p?.minHeight ?? '56px'};

  border-radius: 8px;

  color: ${LeoColors['light.text.primary']};
  background-color: ${({ alertType }) =>
    alertType === 'danger'
      ? leo.color.systemfeedback.errorBackground
      : alertType === 'warning'
      ? leo.color.systemfeedback.warningBackground
      : 'unset'};
`
