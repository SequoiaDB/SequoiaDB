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
import org.bson.BasicBSONObject;

import java.nio.ByteBuffer;

public class AuthRequest extends SdbRequest {
    private final static String AUTH_USER = "User";
    private final static String AUTH_PASSWD = "Passwd";

    private byte[] bsonBytes;

    public enum AuthType {
        Verify,
        CreateUser,
        DeleteUser
    }

    public AuthRequest(String userName, String password, AuthType type) {
        switch (type) {
            case Verify:
                opCode = MsgOpCode.AUTH_VERIFY_REQ;
                break;
            case CreateUser:
                opCode = MsgOpCode.AUTH_CREATE_USER_REQ;
                break;
            case DeleteUser:
                opCode = MsgOpCode.AUTH_DELETE_USER_REQ;
                break;
            default:
                throw new BaseException(SDBError.SDB_INVALIDARG, "Invalid auth type");
        }

        String md5 = null;
        if (password != null) {
            md5 = Helper.md5(password);
        }
        BSONObject obj = new BasicBSONObject();
        obj.put(AUTH_USER, userName);
        obj.put(AUTH_PASSWD, md5);
        bsonBytes = Helper.encodeBSONObj(obj);
        length += Helper.alignedSize(bsonBytes.length);
    }

    @Override
    public void encodeBody(ByteBuffer out) {
        encodeBSONBytes(bsonBytes, out);
    }
}
