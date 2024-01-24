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

import java.io.IOException;
import java.io.Serializable;

import org.apache.flink.core.io.SimpleVersionedSerializer;
import org.apache.flink.core.memory.DataInputDeserializer;
import org.apache.flink.core.memory.DataInputView;
import org.apache.flink.core.memory.DataOutputSerializer;
import org.bson.BSON;
import org.bson.BSONObject;

public class SDBBulkSerializer implements SimpleVersionedSerializer<SDBBulk>, Serializable{

    private static final short MAGIC_NUMBER = (short) 0xbeef;

    private int version = 1;

    public SDBBulkSerializer() { }

    @Override
    public int getVersion() {
        return version;
    }

    /*
     * serialize data from SDBBulk to binary
     * by calling versioned encoding method
     * @param sdbBulk            SDBBulk is about to serialize
     * @return byte[]            SDBBulk binary
     */
    @Override
    public byte[] serialize(SDBBulk sdbBulk) throws IOException {
        DataOutputSerializer out = new DataOutputSerializer(256);
        serializeSDBBulkV1(sdbBulk, out);
        return out.getSharedBuffer();
    }
    /*
     * here we encode 
     * 1. a magic_number to check if data is corrupted 
     * 2. number of BSONObjects in SDBBulk
     * 3. encode List of BSONObjects :
     *  for every BSONObject in SDBBulk: 
     *    1. lenth of BSONObject binary
     *    2. BSONObject binary
     * @param sdbBulk           SDBBulk us about to serialize
     * @param out               the binary output (passed in pointer)
     */ 
    private void serializeSDBBulkV1(SDBBulk sdbBulk, DataOutputSerializer out) throws IOException {
        out.writeShort(MAGIC_NUMBER);
        out.writeInt(sdbBulk.size());
        for (BSONObject bsonObject : sdbBulk.getBsonObjects()) {
            byte[] documentBytes = BSON.encode(bsonObject);
            out.writeInt(documentBytes.length);
            out.write(documentBytes);
        }
    }

    /*
     * when deserialize 
     * first check version, if there is a rolling upgrade, it will need to handle both version
     * second validateMagicNumber, make sure it is not corrupted
     * third deserialize based on version
     */
    @Override
    public SDBBulk deserialize(int version, byte[] serialized) throws IOException {
        DataInputDeserializer in = new DataInputDeserializer(serialized);
        switch (version) {
            case 1:
                validateMagicNumber(in);
                return deserializeSDBBulkV1(in);
            default:
                throw new IOException("Unrecognized version or corrupt state: " + version);
        }
    }

    /*
     * deserialize steps:
     * 1. deserialize SDBBluk size
     * 2. for loop
     *      1. deserialize current BSONObject binary lenth
     *      2. deserialize BSONObject binary
     */
    private SDBBulk deserializeSDBBulkV1(DataInputDeserializer in) throws IOException {
        int len;
        int size = in.readInt();
        SDBBulk bulk = new SDBBulk(size);
        while (size > 0) {
            len = in.readInt();
            byte[] docBuffer = new byte[len];
            in.read(docBuffer);
            BSONObject bsonObject = BSON.decode(docBuffer);
            bulk.add(bsonObject);
            size--;
        }
        return bulk;
    }
    // check magic number
    private void validateMagicNumber(DataInputView in) throws IOException {
        short magicNumber = in.readShort();
        if (magicNumber != MAGIC_NUMBER) {
            throw new IOException(
                    String.format("Corrupt data: Unexpected magic number %04X", magicNumber));
        }
    }

}
