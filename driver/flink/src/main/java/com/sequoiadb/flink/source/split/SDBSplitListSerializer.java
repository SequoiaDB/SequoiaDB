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

import com.sequoiadb.flink.common.exception.SDBException;
import org.apache.flink.calcite.shaded.com.google.common.collect.Lists;
import org.apache.flink.core.io.SimpleVersionedSerializer;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.ArrayList;
import java.util.List;

/**
 * SDBSplitSerializer is using to serialize or deserialize the snapshot
 * of SDBSplitEnumerator. Currently, the snapshot stored pendingList, the
 * List of SplitSplits.
 * It's also compatible with different version of snapshot.
 */
public class SDBSplitListSerializer implements SimpleVersionedSerializer<List<SDBSplit>> {

    public static final SDBSplitSerializer SDB_SPLIT_SERIALIZER = new SDBSplitSerializer();

    private int version = 0;

    @Override
    public int getVersion() {
        return version;
    }

    /**
     * serialize List of SDBSplit, use list length + value to encode
     * the List of SDBSplit.
     */
    @Override
    public byte[] serialize(List<SDBSplit> splits) throws IOException {
        final int size = splits.size();

        List<byte[]> splitBytesList = new ArrayList<>();
        int totalBytes = 0;
        for (SDBSplit split : splits) {
            byte[] splitBytes = SDB_SPLIT_SERIALIZER.serialize(split);
            totalBytes += 4;
            totalBytes += splitBytes.length;

            splitBytesList.add(splitBytes);
        }

        byte[] targetBytes = new byte[4 + totalBytes];
        ByteBuffer bb = ByteBuffer.wrap(targetBytes).order(ByteOrder.LITTLE_ENDIAN);
        bb.putInt(size);
        for (byte[] splitBytes : splitBytesList) {
            bb.putInt(splitBytes.length);
            bb.put(splitBytes);
        }

        return targetBytes;
    }

    /**
     * deserialize SDBSplitEnumerator's snapshot through the given version
     *
     * @param version version of snapshot
     * @param serialized raw bytes the snapshot
     */
    @Override
    public List<SDBSplit> deserialize(int version, byte[] serialized) throws IOException {
        switch (version) {
            case 0:
                return deserializeV0(serialized);

            default:
                throw new SDBException(
                        String.format("deserialized failed, unrecognized version of SDBSplit List: %d.", version));
        }
    }

    /**
     * deserialize raw bytes to List of SDBSplit. decoded by following steps:
     *  1. get length of list
     *  2. for each to get each SDBSplit (len + value)
     *  3. using {@link SDBSplitSerializer} to deserialize each raw bytes of
     *     SDBSplit
     */
    private List<SDBSplit> deserializeV0(byte[] serialized) throws IOException {
        final ByteBuffer bb = ByteBuffer.wrap(serialized)
                .order(ByteOrder.LITTLE_ENDIAN);

        final int size = bb.getInt();
        List<SDBSplit> splits = Lists.newArrayListWithCapacity(size);
        for (int i = 0; i < size; i++) {
            int splitBytesLen = bb.getInt();
            byte[] splitBytes = new byte[splitBytesLen];
            bb.get(splitBytes);

            splits.add(SDB_SPLIT_SERIALIZER.deserialize(0, splitBytes));
        }

        return splits;
    }

}
