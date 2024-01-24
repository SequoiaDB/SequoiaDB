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

package com.sequoiadb.flink.source.split;

import com.sequoiadb.flink.config.SplitMode;
import org.apache.flink.api.connector.source.SourceSplit;

import java.util.List;
import java.util.UUID;

/**
 * SDBSplit is the smallest granularity for data reading, there are two type
 * for different split mode: datablocks, sharding.
 *  1. in sharding mode, SDBSplit represent a sharding of SequoiaDB;
 *  2. in datablocks mode, SDBSplit represent a sub-set of SequoiaDB's datablocks
 *     on a specified data node. The number of datablocks in a single SDBSplit
 *     depends on the source options {@param splitblocknum}.
 */
public class SDBSplit implements SourceSplit {

    private final String splitId;

    private final List<String> urls;
    private final String csName;
    private final String clName;
    private final SplitMode mode;
    private final List<Integer> dataBlocks;

    public SDBSplit(List<String> urls, String csName, String clName, SplitMode mode,
                    List<Integer> dataBlocks) {
        this(UUID.randomUUID().toString(), urls, csName, clName, mode, dataBlocks);
    }

    public SDBSplit(String splitId, List<String> urls, String csName, String clName, SplitMode mode,
            List<Integer> dataBlocks) {
        this.splitId = splitId;

        this.urls = urls;
        this.csName = csName;
        this.clName = clName;
        this.mode = mode;
        this.dataBlocks = dataBlocks;
    }

    @Override
    public String splitId() {
        return splitId;
    }

    public List<String> getUrls() {
        return urls;
    }

    public String getCsName() {
        return csName;
    }

    public String getClName() {
        return clName;
    }

    public SplitMode getMode() {
        return mode;
    }

    public List<Integer> getDataBlocks() {
        return dataBlocks;
    }

    @Override
    public String toString() {
        return "SDBSplit{" +
                "splitId=" + splitId +
                ", urls=" + urls +
                ", csName='" + csName + '\'' +
                ", clName='" + clName + '\'' +
                ", mode=" + mode +
                ", dataBlocks=" + dataBlocks +
                '}';
    }

}
