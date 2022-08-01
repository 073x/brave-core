/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/edge/edge_structure.h"

#include <ostream>
#include <sstream>

#include "brave/third_party/blink/renderer/core/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/types.h"

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/html/node_html.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/html/node_html_element.h"

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/edge/edge.h"

namespace brave_page_graph {

EdgeStructure::EdgeStructure(PageGraph* const graph,
                             Node* const out_node,
                             Node* const in_node)
    : Edge(graph, out_node, in_node) {}

EdgeStructure::EdgeStructure(const NodeHTMLElement* const out_node,
                             NodeHTML* const in_node)
    : Edge(const_cast<NodeHTMLElement*>(out_node), in_node) {}

EdgeStructure::~EdgeStructure() {}

ItemName EdgeStructure::GetItemName() const {
  return "structure";
}

bool EdgeStructure::IsEdgeStructure() const {
  return true;
}

}  // namespace brave_page_graph
