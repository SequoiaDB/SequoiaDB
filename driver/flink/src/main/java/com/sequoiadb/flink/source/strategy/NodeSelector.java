/*
 * Copyright 2022 SequoiaDB Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

package com.sequoiadb.flink.source.strategy;

import com.sequoiadb.flink.config.PreferredInstanceMode;
import com.sequoiadb.flink.config.SDBSourceOptions;
import com.sequoiadb.flink.common.exception.SDBException;

import java.util.List;
import java.util.Random;
import java.util.stream.Collectors;

/**
 * NodeSelector is used to choose a suitable data node for data reading.
 * It depends on some strategy, like preferredinstance, preferredinstancemode,
 * preferredinstancestrict, etc.
 */
public class NodeSelector {

    private static final Random RANDOM = new Random();

    private NodeInfo getNodeByMasterTendency(List<NodeInfo> nodeInfos,
                                             SDBSourceOptions.PreferredInstance preferredInstance) {
        List<Integer> instanceIds = preferredInstance.getInstanceIds();

        if (!instanceIds.isEmpty()) {
            // traverse once to get primary one and candidate one.
            NodeInfo primaryNode = null, candidateNode = null;
            for (NodeInfo nodeInfo : nodeInfos) {
                if (nodeInfo.isPrimary() && instanceIds.contains(nodeInfo.instanceId)) {
                    primaryNode = nodeInfo;
                } else if (instanceIds.contains(nodeInfo.instanceId)) {
                    candidateNode = nodeInfo;
                }
            }

            if (primaryNode != null) {
                return primaryNode;
            }
            if (candidateNode != null) {
                return candidateNode;
            }
        }

        // strict mode
        if (preferredInstance.isPreferredInstanceStrict()) {
            throw new SDBException(
                    String.format("No nodes available in strict master tendency, nodes: %s, " +
                            "instanceids: %s.\n", nodeInfos, instanceIds));
        }

        for (NodeInfo nodeInfo : nodeInfos) {
            if (nodeInfo.isPrimary) {
                return nodeInfo;
            }
        }

        return nodeInfos.get(RANDOM.nextInt(nodeInfos.size()));
    }

    private NodeInfo getNodeBySlaveTendency(List<NodeInfo> nodeInfos,
                                            SDBSourceOptions.PreferredInstance preferredInstance) {
        List<Integer> instanceIds = preferredInstance.getInstanceIds();
        if (!instanceIds.isEmpty()) {
            List<NodeInfo> candidateNodes = nodeInfos.stream()
                    .filter(nodeInfo -> !nodeInfo.isPrimary() && instanceIds.contains(nodeInfo.instanceId))
                    .collect(Collectors.toList());
            if (!candidateNodes.isEmpty()) {
                return getNodeByMode(candidateNodes, instanceIds, preferredInstance.getMode());
            }

            candidateNodes = nodeInfos.stream()
                    .filter(nodeInfo -> instanceIds.contains(nodeInfo.instanceId))
                    .collect(Collectors.toList());
            if (!candidateNodes.isEmpty()) {
                return getNodeByMode(candidateNodes, instanceIds, preferredInstance.getMode());
            }
        }

        // strict mode
        if (preferredInstance.isPreferredInstanceStrict()) {
            throw new SDBException(
                    String.format("No nodes available in strict slave tendency, nodes: %s, " +
                            "instanceids: %s.\n", nodeInfos, instanceIds));
        }

        List<NodeInfo> candidateNodes = nodeInfos.stream()
                .filter(nodeInfo -> !nodeInfo.isPrimary())
                .collect(Collectors.toList());
        if (!candidateNodes.isEmpty()) {
            return candidateNodes.get(RANDOM.nextInt(candidateNodes.size()));
        }

        return nodeInfos.get(RANDOM.nextInt(nodeInfos.size()));
    }

    private NodeInfo getNodeByAnyTendency(List<NodeInfo> nodeInfos,
                                          SDBSourceOptions.PreferredInstance preferredInstance) {
        List<Integer> instanceIds = preferredInstance.getInstanceIds();
        if (!instanceIds.isEmpty()) {
            List<NodeInfo> candidateNodes = nodeInfos.stream()
                    .filter(nodeInfo -> instanceIds.contains(nodeInfo.instanceId))
                    .collect(Collectors.toList());
            if (!candidateNodes.isEmpty()) {
                return getNodeByMode(candidateNodes, instanceIds, preferredInstance.getMode());
            }
        }

        if (preferredInstance.isPreferredInstanceStrict()) {
            throw new SDBException(
                    String.format("No nodes available in strict any tendency, nodes: %s, " +
                            "instanceids: %s.\n", nodeInfos, instanceIds));
        }

        return nodeInfos.get(RANDOM.nextInt(nodeInfos.size()));
    }

    /**
     * Get node by mode, currently there are two mainly mode:
     *  1. ordered: depends on the order of instance ids specify by user
     *  2. random: choose one randomly
     */
    private NodeInfo getNodeByMode(List<NodeInfo> nodeInfos,
                                   List<Integer> instanceIds,
                                   PreferredInstanceMode preferredInstanceMode) {
        if (preferredInstanceMode == PreferredInstanceMode.ORDERED) {
            // sort node through preferred instance ids specified by user.
            nodeInfos.sort((lNode, rNode) -> {
                int lIdx = instanceIds.indexOf(lNode.instanceId);
                int rIdx = instanceIds.indexOf(rNode.instanceId);
                if (lIdx != -1 && rIdx != -1) {
                    return lIdx - rIdx;
                } else if (lIdx != -1) { // rIdx == -1
                    return -1;
                } else {                 // lIdx == -1 || (lIdx == -1 && rIdx == -1)
                    return 1;
                }
            });
            return nodeInfos.get(0);
        }
        return nodeInfos.get(RANDOM.nextInt(nodeInfos.size()));
    }

    /**
     * select data node via preferred instance strategy
     *
     * @param nodeInfos node infos of data nodes
     * @param preferredInstance preferred strategy
     * @return the suitable data node
     */
    public NodeInfo select(List<NodeInfo> nodeInfos,
                           SDBSourceOptions.PreferredInstance preferredInstance) {
        if (nodeInfos.isEmpty()) {
            return null;
        }

        if (preferredInstance.nonPreferred()) {
            return nodeInfos.get(RANDOM.nextInt(nodeInfos.size()));
        } else if (preferredInstance.isMasterTendency()) {
            return getNodeByMasterTendency(nodeInfos, preferredInstance);
        } else if (preferredInstance.isSlaveTendency()) {
            return getNodeBySlaveTendency(nodeInfos, preferredInstance);
        } else {
            return getNodeByAnyTendency(nodeInfos, preferredInstance);
        }
    }

}
