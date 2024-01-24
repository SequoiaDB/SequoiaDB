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

import org.apache.flink.calcite.shaded.com.google.common.collect.Queues;
import org.apache.flink.connector.base.source.reader.RecordsWithSplitIds;

import javax.annotation.Nullable;
import java.util.Collections;
import java.util.List;
import java.util.Queue;
import java.util.Set;

/**
 * SDBRecords includes a sub-set of records from a SDBSplit. It will pass
 * from fetcher to the SDBReader (then write to OutputSource).
 */
public class SDBRecords implements RecordsWithSplitIds<byte[]> {

    private final String splitId;

    private final Queue<byte[]> remaining = Queues.newArrayDeque();
    private final Set<String> finishedSplits;

    private volatile boolean read = false;

    public SDBRecords(String splitId, List<byte[]> records, Set<String> finishedSplits) {
        this.splitId = splitId;
        this.finishedSplits = finishedSplits == null ? Collections.emptySet() : finishedSplits;
        if (records != null) {
            remaining.addAll(records);
        }
    }

    @Nullable
    @Override
    public String nextSplit() {
        if (read) {
            return null;
        } else {
            read = true;
            return splitId;
        }
    }

    @Nullable
    @Override
    public byte[] nextRecordFromSplit() {
        return remaining.poll();
    }

    @Override
    public Set<String> finishedSplits() {
        return finishedSplits;
    }

    @Override
    public void recycle() { }

    public static SDBRecords forRecords(String splitId, List<byte[]> records) {
        return new SDBRecords(splitId, records, Collections.emptySet());
    }

    public static SDBRecords finishedSplits(String splitId, List<byte[]> records) {
        return new SDBRecords(splitId, records, Collections.singleton(splitId));
    }

}
