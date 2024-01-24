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

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.message.MsgOpCode;
import com.sequoiadb.util.Helper;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;

import java.io.UnsupportedEncodingException;
import java.nio.ByteBuffer;

public class QueryRequest extends SdbRequest {
    private static final byte[] EMPTY_BSON_BYTES = Helper.encodeBSONObj(new BasicBSONObject());
    private static final int ALIGNED_EMPTY_BSON_LENGTH = Helper.alignedSize(EMPTY_BSON_BYTES.length);
    private static final int FIXED_LENGTH = 88;
    private static final int version = 1;
    private static final short w = 0;
    private static final short padding = 0;
    private int flag;
    private long skipNum;
    private long returnedNum;
    private byte[] clNameBytes;
    private byte[] matcherBytes;
    private byte[] selectorBytes;
    private byte[] orderBytes;
    private byte[] hintBytes;

    public QueryRequest(String collectionName,
                        BSONObject matcher, BSONObject selector, BSONObject orderBy, BSONObject hint,
                        long skipNum, long returnedNum, int flag) {
        opCode = MsgOpCode.QUERY_REQ;
        length = FIXED_LENGTH;

        this.flag = flag;
        this.skipNum = skipNum;
        if (returnedNum < 0) {
            returnedNum = -1;
        }
        this.returnedNum = returnedNum;

        if (collectionName == null || collectionName.length() == 0) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "Collection name or command is null or empty");
        }

        try {
            this.clNameBytes = collectionName.getBytes(Helper.ENCODING_TYPE);
            length += Helper.alignedSize(this.clNameBytes.length + 1);
        }catch (UnsupportedEncodingException e) {
            throw new BaseException(SDBError.SDB_INVALIDARG, e);
        }

        if (matcher == null) {
            matcherBytes = EMPTY_BSON_BYTES.clone();
            length += ALIGNED_EMPTY_BSON_LENGTH;
        } else {
            matcherBytes = Helper.encodeBSONObj(matcher);
            length += Helper.alignedSize(matcherBytes.length);
        }

        if (selector == null) {
            selectorBytes = EMPTY_BSON_BYTES.clone();
            length += ALIGNED_EMPTY_BSON_LENGTH;
        } else {
            selectorBytes = Helper.encodeBSONObj(selector);
            length += Helper.alignedSize(selectorBytes.length);
        }

        if (orderBy == null) {
            orderBytes = EMPTY_BSON_BYTES.clone();
            length += ALIGNED_EMPTY_BSON_LENGTH;
        } else {
            orderBytes = Helper.encodeBSONObj(orderBy);
            length += Helper.alignedSize(orderBytes.length);
        }

        if (hint == null) {
            hintBytes = EMPTY_BSON_BYTES.clone();
            length += ALIGNED_EMPTY_BSON_LENGTH;
        } else {
            hintBytes = Helper.encodeBSONObj(hint);
            length += Helper.alignedSize(hintBytes.length);
        }
    }

    @Override
    protected void encodeBody(ByteBuffer out) {
        out.putInt(version);
        out.putShort(w);
        out.putShort(padding);
        out.putInt(flag);
        out.putInt(clNameBytes.length);
        out.putLong(skipNum);
        out.putLong(returnedNum);
        out.put(clNameBytes);
        out.put((byte) 0);
        int length = clNameBytes.length + 1;
        int paddingLen = Helper.alignedSize(length) - length;
        Helper.fillZero(out, paddingLen);
        encodeBSONBytes(matcherBytes, out);
        encodeBSONBytes(selectorBytes, out);
        encodeBSONBytes(orderBytes, out);
        encodeBSONBytes(hintBytes, out);
    }
}
