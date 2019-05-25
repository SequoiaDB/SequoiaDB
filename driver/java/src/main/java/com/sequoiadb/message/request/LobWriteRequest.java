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

import java.nio.ByteBuffer;

public class LobWriteRequest extends LobRequest {
    private static final int FIXED_LENGTH = 68; // LOB_HEADER_LENGTH + 16
    private int bufLen;
    private static final int sequence = 0;
    private long offset;
    private ByteBuffer buffer;

    public LobWriteRequest(long contextId, byte[] buf, int off, int len, long lobOffset) {
        opCode = MsgOpCode.LOB_WRITE_REQ;
        length = FIXED_LENGTH;

        if (buf == null) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "obj is null");
        }
        if (off + len > buf.length) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "off + len is more than buf.length");
        }

        this.contextId = contextId;
        bufLen = len;
        buffer = ByteBuffer.wrap(buf, off, len);
        length += Helper.alignedSize(len);
        this.offset = lobOffset;
    }

    @Override
    protected void encodeLobBody(ByteBuffer out) {
        out.putInt(bufLen);
        out.putInt(sequence);
        out.putLong(offset);
        out.put(buffer);
        int paddingSize = Helper.alignedSize(bufLen) - bufLen;
        if (paddingSize > 0) {
            out.put(new byte[paddingSize]);
        }
    }
}
