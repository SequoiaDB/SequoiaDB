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

package com.sequoiadb.flink.sink.state;

import org.apache.flink.core.io.SimpleVersionedSerializer;
import org.apache.flink.core.memory.DataInputDeserializer;
import org.apache.flink.core.memory.DataInputView;
import org.apache.flink.core.memory.DataOutputSerializer;
import org.apache.flink.table.data.TimestampData;
import org.apache.flink.table.runtime.typeutils.TimestampDataSerializer;
import org.bson.BSON;
import org.bson.BSONObject;

import java.io.IOException;
import java.io.Serializable;
import java.util.HashMap;
import java.util.Map;

/**
 * Serializer for {@link EventState}.
 */
public class EventStateSerializer implements
        Serializable,
        SimpleVersionedSerializer<Map<BSONObject, EventState>> {

    // magic number for data validation
    private static final short MAGIC_NUMBER = (short) 0xbeef;

    private static final TimestampDataSerializer TD_SERIALIZER = new TimestampDataSerializer(9);
    private static final int DEFAULT_BUFFER_SIZE = 4 * 1024 * 1024;

    /**
     * The serializer has a version (returned by getVersion()) which can be attached
     * to the serialized data. When the serializer evolves, the version should be upgrade too.
     * version can be used to identify with which prior version the data was serialized.
     */
    private int version = 1;

    public EventStateSerializer() {}

    @Override
    public int getVersion() {
        return version;
    }

    @Override
    public byte[] serialize(Map<BSONObject, EventState> stateMap) throws IOException {
        DataOutputSerializer out
                = new DataOutputSerializer(DEFAULT_BUFFER_SIZE);
        serializeHelperV1(stateMap, out);
        return out.getCopyOfBuffer();
    }

    /**
     * serialize state map (Map<BSONObject, EventState>) to bytes
     *
     * using lv to encode, such as
     * -------------------------------------
     * +  length +          value          +
     * -------------------------------------
     * +   117   + fe 12 ................. +
     * -------------------------------------
     *
     * @param stateMap latest primary key update time's map
     * @param out output buffer
     * @throws IOException
     */
    private void serializeHelperV1(
            Map<BSONObject, EventState> stateMap,
            DataOutputSerializer out) throws IOException {
        DataOutputSerializer reused = new DataOutputSerializer(256);

        out.writeShort(MAGIC_NUMBER);
        out.writeInt(stateMap.size());

        // foreach entry, convert to bytes and write to output
        for (Map.Entry<BSONObject, EventState> state : stateMap.entrySet()) {
            BSONObject pk = state.getKey();
            EventState eventState = state.getValue();

            byte[] rawBson = BSON.encode(pk);
            // matcher
            out.writeInt(rawBson.length);
            out.write(rawBson);

            // event time
            reused.clear();
            TD_SERIALIZER.serialize(eventState.getEventTime(), reused);
            byte[] et = reused.getCopyOfBuffer();
            out.writeInt(et.length);
            out.write(et);

            // processing time
            reused.clear();
            TD_SERIALIZER.serialize(eventState.getProcessingTime(), reused);
            byte[] pt = reused.getCopyOfBuffer();
            out.writeInt(pt.length);
            out.write(pt);
        }
    }

    /**
     * deserialize bytes to stateMap (Map<BSONObject, EventState>)
     *
     * @param version The version in which the data was serialized
     * @param serialized The serialized data
     * @return
     * @throws IOException
     */
    @Override
    public Map<BSONObject, EventState> deserialize(int version, byte[] serialized) throws IOException {
        DataInputDeserializer in = new DataInputDeserializer(serialized);

        switch (version) {
            case 1:
                validateMagicNumber(in);
                return deserializeHelperV1(in);

            default:
                throw new IOException(String.format(
                        "unrecognized version of event state serializer: %s",
                        version));
        }
    }

    private Map<BSONObject, EventState> deserializeHelperV1(DataInputDeserializer in)
            throws IOException {
        Map<BSONObject, EventState> stateMap = new HashMap<>();

        int mapSize = in.readInt();

        byte[] rawBytes;
        int len;
        while (mapSize-- > 0) {
            // primary key
            len = in.readInt();
            rawBytes = new byte[len];
            in.read(rawBytes);
            BSONObject matcher = BSON.decode(rawBytes);

            // event state
            // event time
            len = in.readInt();
            rawBytes = new byte[len];
            TimestampData et = TD_SERIALIZER
                    .deserialize(new DataInputDeserializer(rawBytes));

            // processing time
            len = in.readInt();
            rawBytes = new byte[len];
            TimestampData pt = TD_SERIALIZER
                    .deserialize(new DataInputDeserializer(rawBytes));

            stateMap.put(matcher, new EventState(et, pt));
        }

        return stateMap;
    }

    // check magic number
    private void validateMagicNumber(DataInputView in) throws IOException {
        short magic = in.readShort();
        if (magic != MAGIC_NUMBER) {
            throw new IOException(String.format(
                    "corrupt data: unexpected magic number %04X",
                    magic));
        }
    }

}
