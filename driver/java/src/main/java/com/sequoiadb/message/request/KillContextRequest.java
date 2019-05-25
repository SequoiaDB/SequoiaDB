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

import java.nio.ByteBuffer;

public class KillContextRequest extends SdbRequest {
    private final static int FIXED_LENGTH = HEADER_LENGTH + 8;
    private final static int reserved = 0;
    private long[] contextIds;

    public KillContextRequest(long[] contextIds) {
        opCode = MsgOpCode.KILL_CONTEXT_REQ;
        length = FIXED_LENGTH;

        if (contextIds == null) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "contextIds is null");
        }

        this.contextIds = contextIds;
        length += contextIds.length * 8;
    }

    @Override
    protected void encodeBody(ByteBuffer out) {
        out.putInt(reserved);
        out.putInt(contextIds.length);
        for (long contextId : contextIds) {
            out.putLong(contextId);
        }
    }
}
