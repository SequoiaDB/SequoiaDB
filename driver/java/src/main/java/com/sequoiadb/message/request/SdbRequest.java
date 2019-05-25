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

import com.sequoiadb.message.SdbMsgHeader;
import com.sequoiadb.util.Helper;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;

import static com.sequoiadb.util.Helper.bsonEndianConvert;

public abstract class SdbRequest extends SdbMsgHeader implements Request {
    protected SdbRequest() {
        length = HEADER_LENGTH;
    }

    @Override
    public void setRequestId(long requestId) {
        this.requestId = requestId;
    }

    @Override
    public int length() {
        return length;
    }

    @Override
    public int opCode() {
        return opCode;
    }

    @Override
    public void encode(ByteBuffer out) {
        encodeMsgHeader(out);
        encodeBody(out);
    }

    protected void encodeMsgHeader(ByteBuffer out) {
        out.putInt(length);
        out.putInt(opCode);
        out.putInt(tid);
        out.putLong(routeId);
        out.putLong(requestId);
    }

    protected abstract void encodeBody(ByteBuffer out);

    protected static void encodeBSONBytes(byte[] objBytes, ByteBuffer out) {
        if (out.order() == ByteOrder.BIG_ENDIAN) {
            bsonEndianConvert(objBytes, 0, objBytes.length, true);
        }
        out.put(objBytes);

        int paddingLen = Helper.alignedSize(objBytes.length) - objBytes.length;
        if (paddingLen > 0) {
            out.put(new byte[paddingLen]);
        }
    }
}
