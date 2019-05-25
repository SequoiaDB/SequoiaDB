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

package com.sequoiadb.message.response;

import com.sequoiadb.message.SdbMsgHeader;

import java.nio.ByteBuffer;

public abstract class SdbResponse extends SdbMsgHeader implements Response {
    public long getRequestId() {
        return requestId;
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
    public void decode(ByteBuffer in) {
        decodeMsgHeader(in);
        decodeBody(in);
    }

    protected void decodeMsgHeader(ByteBuffer in) {
        length = in.getInt();
        opCode = in.getInt();
        tid = in.getInt();
        routeId = in.getLong();
        requestId = in.getLong();
    }

    protected abstract void decodeBody(ByteBuffer in);
}
