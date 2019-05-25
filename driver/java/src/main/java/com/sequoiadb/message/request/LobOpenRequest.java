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

import java.nio.ByteBuffer;

public class LobOpenRequest extends LobRequest {
    private byte[] bsonBytes;

    public LobOpenRequest(BSONObject obj, int flag) {
        opCode = MsgOpCode.LOB_OPEN_REQ;

        if (obj == null) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "obj is null");
        }

        bsonBytes = Helper.encodeBSONObj(obj);
        bsonLength = bsonBytes.length;
        length += Helper.alignedSize(bsonBytes.length);

        this.flag = flag;
    }

    @Override
    protected void encodeLobBody(ByteBuffer out) {
        encodeBSONBytes(bsonBytes, out);
    }
}
