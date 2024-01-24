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

import com.sequoiadb.message.MsgOpCode;
import com.sequoiadb.util.Helper;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;

import java.nio.ByteBuffer;

public class LobLockRequest extends LobRequest {
    private static final String FIELD_NAME_OFFSET = "Offset";
    private static final String FIELD_NAME_LENGTH = "Length";
    private byte[] bsonBytes;

    public LobLockRequest(long contextId, long offset, long length) {
        opCode = MsgOpCode.LOB_LOCK_REQ;

        this.contextId = contextId;

        BSONObject obj = new BasicBSONObject();
        obj.put(FIELD_NAME_OFFSET, offset);
        obj.put(FIELD_NAME_LENGTH, length);

        bsonBytes = Helper.encodeBSONObj(obj);
        bsonLength = bsonBytes.length;
        this.length += Helper.alignedSize(bsonBytes.length);
    }

    @Override
    protected void encodeLobBody(ByteBuffer out) {
        encodeBSONBytes(bsonBytes, out);
    }
}
