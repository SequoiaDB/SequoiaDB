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

import com.sequoiadb.base.options.UpdateOption;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.message.MsgOpCode;
import com.sequoiadb.util.Helper;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;

import java.io.UnsupportedEncodingException;
import java.nio.ByteBuffer;

public class UpdateRequest extends SdbRequest {
    private static final int FIXED_LENGTH = 72;
    private static final byte[] EMPTY_BSON_BYTES = Helper.encodeBSONObj(new BasicBSONObject());
    private static final int ALIGNED_EMPTY_BSON_LENGTH = Helper.alignedSize(EMPTY_BSON_BYTES.length);
    private static final int version = 1;
    private static final short w = 0;
    private static final short padding = 0;
    private int flag = 0;
    private byte[] clNameBytes;
    private byte[] matcherBytes;
    private byte[] modifierBytes;
    private byte[] hintBytes;

    public UpdateRequest(String collectionName, BSONObject matcher, BSONObject modifier, BSONObject hint, int flag) {
        opCode = MsgOpCode.UPDATE_REQ;
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

        if (matcher == null) {
            matcherBytes = EMPTY_BSON_BYTES.clone();
            length += ALIGNED_EMPTY_BSON_LENGTH;
        } else {
            matcherBytes = Helper.encodeBSONObj(matcher);
            length += Helper.alignedSize(matcherBytes.length);
        }

        if (modifier == null) {
            modifierBytes = EMPTY_BSON_BYTES.clone();
            length += ALIGNED_EMPTY_BSON_LENGTH;
        } else {
            modifierBytes = Helper.encodeBSONObj(modifier);
            length += Helper.alignedSize(modifierBytes.length);
        }

        if (hint == null) {
            hintBytes = EMPTY_BSON_BYTES.clone();
            length += ALIGNED_EMPTY_BSON_LENGTH;
        } else {
            hintBytes = Helper.encodeBSONObj(hint);
            length += Helper.alignedSize(hintBytes.length);
        }

        this.flag = flag;
    }

    @Override
    protected void encodeBody(ByteBuffer out) {
        out.putInt(version);
        out.putShort(w);
        out.putShort(padding);
        out.putInt(flag);
        out.putInt(clNameBytes.length);
        out.put(clNameBytes);
        out.put((byte) 0);
        int length = clNameBytes.length + 1;
        int paddingLen = Helper.alignedSize(length) - length;
        Helper.fillZero(out, paddingLen);
        encodeBSONBytes(matcherBytes, out);
        encodeBSONBytes(modifierBytes, out);
        encodeBSONBytes(hintBytes, out);
    }
}
