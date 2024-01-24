/*
 * Copyright 2022 SequoiaDB Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
 * in compliance with the License. You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the License
 * is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
 * or implied. See the License for the specific language governing permissions and limitations under
 * the License.
 */

package com.sequoiadb.message.request;

import com.sequoiadb.message.MsgConstants;
import com.sequoiadb.message.MsgOpCode;
import com.sequoiadb.message.SdbAuthVersion;
import com.sequoiadb.util.Helper;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;

public class CreateUserRequest extends AuthRequest {

    public CreateUserRequest(String userName, String password, SdbAuthVersion authVersion, BSONObject options) {
        String md5Pwd = password != null ? Helper.md5(password) : null;
        BSONObject obj = new BasicBSONObject();
        obj.put(MsgConstants.AUTH_USER, userName);
        obj.put(MsgConstants.AUTH_PASSWD, md5Pwd);
        if (options != null) {
            obj.put(MsgConstants.AUTH_OPTIONS, options);
        }

        if (authVersion.getVersion() >= SdbAuthVersion.SDB_AUTH_SCRAM_SHA256.getVersion()) {
            obj.put(MsgConstants.AUTH_TEXT_PASSWD, password);
        }

        opCode = MsgOpCode.AUTH_CREATE_USER_REQ;

        bsonBytes = Helper.encodeBSONObj(obj);
        length += Helper.alignedSize(bsonBytes.length);
    }
}