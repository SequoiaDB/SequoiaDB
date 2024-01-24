/*
 * Copyright 2018 SequoiaDB Inc.
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

import com.sequoiadb.base.options.InsertOption;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.message.MsgOpCode;
import com.sequoiadb.util.Helper;
import com.sequoiadb.message.MsgConstants;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.bson.types.ObjectId;

import java.io.UnsupportedEncodingException;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.List;

public class InsertRequest extends SdbRequest {
    private final static String OID = "_id";
    private static final int FIXED_LENGTH = 72;
    private static final int version = 1;
    private static final short w = 0;
    private static final short padding = 0;
    private int flag = 0;
    private byte[] clNameBytes;
    private List<byte[]> docsBytes;
    private Object oid;

    private InsertRequest(String collectionName) {
        opCode = MsgOpCode.INSERT_REQ;
        length = FIXED_LENGTH;

        if (collectionName == null || collectionName.length() == 0) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "Collection name is null or empty");
        }

        try {
            this.clNameBytes = collectionName.getBytes(Helper.ENCODING_TYPE);
            length += Helper.alignedSize(this.clNameBytes.length + 1);
        }catch (UnsupportedEncodingException e) {
            throw new BaseException(SDBError.SDB_INVALIDARG, e);
        }
    }

    public InsertRequest(String collectionName, BSONObject doc, int flag) {
        this(collectionName);

        BSONObject extendObj = null;
        this.flag = flag;

        if (doc == null) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "doc is null");
        }

        Object objId = doc.get(OID);
        if (objId == null) {
            objId = ObjectId.get();
            extendObj = new BasicBSONObject(OID, objId);
        }
        if ((flag & InsertOption.FLG_INSERT_RETURN_OID) != 0) {
            oid = objId;
            // Compatible with previous behavior
            if (!doc.containsField(OID)) {
                doc.put(OID, objId);
            }
        }
        docsBytes = new ArrayList<byte[]>(1);
        byte[] docBytes = Helper.encodeBSONObj(doc, extendObj);
        docsBytes.add(docBytes);
        length += Helper.alignedSize(docBytes.length);
        
        // Inform coord or data nodes that the '_id' field is included in the record.
        this.flag |= MsgConstants.FLG_INSERT_HAS_ID_FIELD;
    }

    public InsertRequest(String collectionName, List<BSONObject> docs, int flag) {
        this(collectionName);

        this.flag = flag;

        if (docs == null || docs.size() == 0) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "docs is null or empty");
        }
        if ((flag & InsertOption.FLG_INSERT_RETURN_OID) != 0) {
            oid = new BasicBSONList();
        }
        docsBytes = new ArrayList<byte[]>(docs.size());
        int index = 0;
        BasicBSONObject extendObj = new BasicBSONObject();
        for (BSONObject doc : docs) {
            Object objId = doc.get(OID);
            if (objId == null) {
                objId = ObjectId.get();
                extendObj.put(OID, objId);
            }
            if ((flag & InsertOption.FLG_INSERT_RETURN_OID) != 0) {
                ((BasicBSONList) oid).put(index++, objId);
            }
            // Compatible with previous behavior
            if (!doc.containsField(OID)) {
                doc.put(OID, objId);
            }

            byte[] docBytes = Helper.encodeBSONObj(doc, extendObj);
            extendObj.clear();
            docsBytes.add(docBytes);
            length += Helper.alignedSize(docBytes.length);
        }

        // Inform coord or data nodes that the '_id' field is included in records.
        this.flag |= MsgConstants.FLG_INSERT_HAS_ID_FIELD;
    }

    public Object getOIDValue() {
        return oid;
    }

    @Override
    protected void encodeBody(ByteBuffer out) {
        out.putInt(version);
        out.putShort(w);
        out.putShort(padding);
        out.putInt(flag);
        out.putInt(clNameBytes.length);
        out.put(clNameBytes);
        out.put((byte) 0); // end of string
        int length = clNameBytes.length + 1;
        int paddingLen = Helper.alignedSize(length) - length;
        Helper.fillZero(out, paddingLen);
        for (byte[] docBytes : docsBytes) {
            encodeBSONBytes(docBytes, out);
        }
    }
}
