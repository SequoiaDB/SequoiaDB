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
import com.sequoiadb.flink.common.exception.SDBException;
import org.apache.flink.core.io.SimpleVersionedSerializer;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * SDBSplitSerializer is using to serialize or deserialize SDBSplit
 * It's compatible with different versions of SDBSplit.
 */
public class SDBSplitSerializer implements SimpleVersionedSerializer<SDBSplit> {

    private final int version = 0;

    @Override
    public int getVersion() {
        return version;
    }

    /**
     * serialize {@link SDBSplit}, using length + value to encode each field
     * in SDBSplit.
     */
    @Override
    public byte[] serialize(SDBSplit split) throws IOException {
        String urlStr = String.join(",", split.getUrls());

        final byte[] splitId = split.splitId().getBytes(StandardCharsets.UTF_8);
        final byte[] urls = urlStr.getBytes(StandardCharsets.UTF_8);
        final byte[] csName = split.getCsName().getBytes(StandardCharsets.UTF_8);
        final byte[] clName = split.getClName().getBytes(StandardCharsets.UTF_8);

        // 24 bytes for the following 6 integers:
        // splitIdLen, urlsLen, csNameLen, clNameLen, splitMode, dataBlockLen
        int totalBytes = 24
                + splitId.length
                + urls.length
                + csName.length
                + clName.length;

        // for datablocks list, need 4 bytes to store the length of datablocks list
        // and datablocks.size() * 4 bytes to store all elements in list.
        List<Integer> dataBlocks = split.getDataBlocks();
        if (dataBlocks != null) {
            totalBytes += dataBlocks.size() * 4;
        }
        final byte[] targetBytes = new byte[totalBytes];

        ByteBuffer bb = ByteBuffer.wrap(targetBytes).order(ByteOrder.LITTLE_ENDIAN);
        bb.putInt(splitId.length);
        bb.put(splitId);
        bb.putInt(urls.length);
        bb.put(urls);
        bb.putInt(csName.length);
        bb.put(csName);
        bb.putInt(clName.length);
        bb.put(clName);

        bb.putInt(split.getMode().getCode());

        bb.putInt(dataBlocks == null ? 0 : dataBlocks.size());
        if (dataBlocks != null) {
            for (int dataBlock : dataBlocks) {
                bb.putInt(dataBlock);
            }
        }

        return targetBytes;
    }

    /**
     * deserialize raw bytes of SDBSplit through the given version.
     * It's compatible with different versions of SDBSplit.
     *
     * @param version version of SDBSplit
     * @param serialized raw bytes of SDBSplit
     */
    @Override
    public SDBSplit deserialize(int version, byte[] serialized) throws IOException {
        switch (version) {
            case 0:
                return deserializeV0(serialized);

            default:
                throw new SDBException(
                        String.format("deserialized failed, unrecognized version of SDBSplit: %d.", version));
        }
    }

    /**
     * deserialization for SDBSplit version 0
     */
    private SDBSplit deserializeV0(byte[] serialized) {
        ByteBuffer bb = ByteBuffer.wrap(serialized).order(ByteOrder.LITTLE_ENDIAN);

        int splitIdLen = bb.getInt();
        final byte[] splitId = new byte[splitIdLen];
        bb.get(splitId);

        int urlsLen = bb.getInt();
        final byte[] urls = new byte[urlsLen];
        bb.get(urls);

        int csNameLen = bb.getInt();
        final byte[] csName = new byte[csNameLen];
        bb.get(csName);

        int clNameLen = bb.getInt();
        final byte[] clName = new byte[clNameLen];
        bb.get(clName);

        int splitMode = bb.getInt();

        int dataBlocksLen = bb.getInt();
        List<Integer> dataBlocks = new ArrayList<>();
        if (dataBlocksLen != 0) {
            for (int i = 0; i < dataBlocksLen; i++) {
                dataBlocks.add(bb.getInt());
            }
        }

        String[] urlsStr = new String(urls, StandardCharsets.UTF_8).split(",");
        SplitMode mode = SplitMode.AUTO;
        if (splitMode == SplitMode.DATA_BLOCK.getCode()) {
            mode = SplitMode.DATA_BLOCK;
        } else if (splitMode == SplitMode.SHARDING.getCode()) {
            mode = SplitMode.SHARDING;
        }

        return new SDBSplit(
                new String(splitId, StandardCharsets.UTF_8),
                Arrays.asList(urlsStr),
                new String(csName, StandardCharsets.UTF_8),
                new String(clName, StandardCharsets.UTF_8),
                mode,
                dataBlocks);
    }

}