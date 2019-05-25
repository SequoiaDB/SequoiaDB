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
import org.bson.types.ObjectId;

import java.io.UnsupportedEncodingException;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.List;

public class InsertRequest extends SdbRequest {
    private final static String OID = "_id";
    private static final int FIXED_LENGTH = 44;
    private static final int version = 1;
    private static final short w = 0;
    private static final short padding = 0;
    private int flag = 0;
    private String collectionName;
    private List<byte[]> docsBytes;

    private InsertRequest(String collectionName) {
        opCode = MsgOpCode.INSERT_REQ;
        length = FIXED_LENGTH;

        if (collectionName == null || collectionName.length() == 0) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "Collection name is null or empty");
        }

        this.collectionName = collectionName;
        length += Helper.alignedSize(collectionName.length() + 1);
    }

    public InsertRequest(String collectionName, BSONObject doc) {
        this(collectionName);

        if (doc == null) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "doc is null");
        }

        docsBytes = new ArrayList<byte[]>(1);
        byte[] docBytes = Helper.encodeBSONObj(doc);
        docsBytes.add(docBytes);
        length += Helper.alignedSize(docBytes.length);
    }

    public InsertRequest(String collectionName, List<BSONObject> docs, int flag, boolean ensureOID) {
        this(collectionName);

        this.flag = flag;

        if (docs == null || docs.size() == 0) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "docs is null or empty");
        }

        docsBytes = new ArrayList<byte[]>(docs.size());
        for (BSONObject doc : docs) {
            if (ensureOID && !doc.containsField(OID)) {
                doc.put(OID, ObjectId.get());
            }
            byte[] docBytes = Helper.encodeBSONObj(doc);
            docsBytes.add(docBytes);
            length += Helper.alignedSize(docBytes.length);
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
        for (byte[] docBytes : docsBytes) {
            encodeBSONBytes(docBytes, out);
        }
    }
}
