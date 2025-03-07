// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react';
import { Cluster as Info } from 'gen/brave/components/brave_news/common/brave_news.mojom.m';
import Card from './Card';

interface Props {
  info: Info
}

export default function Cluster({info}: Props) {
  return <Card>
    Cluster: {info.type} {info.id}
  </Card>
}
