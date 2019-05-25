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

import com.sequoiadb.message.MsgOpCode;

import java.nio.ByteBuffer;

public class GetMoreRequest extends SdbRequest {
    private static final int LENGTH = 40;
    private long contextId;
    private int returnedNum;

    public GetMoreRequest(long contextId, int returnedNum) {
        opCode = MsgOpCode.GET_MORE_REQ;
        length = LENGTH;
        this.contextId = contextId;
        this.returnedNum = returnedNum;
    }

    @Override
    protected void encodeBody(ByteBuffer out) {
        out.putLong(contextId);
        out.putInt(returnedNum);
    }
}
