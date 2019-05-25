/*
 * Copyright 2017 SequoiaDB Inc.
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

package com.sequoiadb.message.request;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.message.MsgOpCode;
import com.sequoiadb.util.Helper;
import org.bson.BSONObject;

import java.io.UnsupportedEncodingException;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.List;

public class AggregateRequest extends SdbRequest {
    private static final int FIXED_LENGTH = 44;
    private static final int version = 1;
    private static final short w = 0;
    private static final short padding = 0;
    private static final int flag = 0;
    private String collectionName;
    private List<byte[]> objsBytes;

    public AggregateRequest(String collectionName, List<BSONObject> objects) {
        opCode = MsgOpCode.AGGREGATE_REQ;
        length = FIXED_LENGTH;

        if (collectionName == null || collectionName.length() == 0) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "Collection name is null or empty");
        }

        if (objects == null || objects.size() == 0) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "Aggregate objects is null or empty");
        }

        this.collectionName = collectionName;
        length += Helper.alignedSize(collectionName.length() + 1);

        objsBytes = new ArrayList<byte[]>(objects.size());
        for (BSONObject obj : objects) {
            byte[] objBytes = Helper.encodeBSONObj(obj);
            objsBytes.add(objBytes);
            length += Helper.alignedSize(objBytes.length);
        }
    }

    @Override
    protected void encodeBody(ByteBuffer out) {
        out.putInt(version);
        out.putShort(w);
        out.putShort(padding);
        out.putInt(flag);
        out.putInt(collectionName.length());
        try {
            out.put(collectionName.getBytes("UTF-8"));
            out.put((byte) 0); // end of string
            int length = collectionName.length() + 1;
            int paddingLen = Helper.alignedSize(length) - length;
            if (paddingLen > 0) {
                out.put(new byte[paddingLen]);
            }
        } catch (UnsupportedEncodingException e) {
            throw new BaseException(SDBError.SDB_INVALIDARG, e);
        }
        for (byte[] docBytes : objsBytes) {
            encodeBSONBytes(docBytes, out);
        }
    }
}
