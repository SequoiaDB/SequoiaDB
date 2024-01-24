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
import com.sequoiadb.util.Helper;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;

public class AuthVerifySHA256Request extends AuthRequest {

    private AuthVerifySHA256Request(BSONObject obj) {
        opCode = MsgOpCode.AUTH_VERIFY1_REQ;

        bsonBytes = Helper.encodeBSONObj(obj);
        length += Helper.alignedSize(bsonBytes.length);
    }

    public static AuthVerifySHA256Request step1(String userName,
                                                String clientNonceBast64) {
        BSONObject obj = new BasicBSONObject();
        obj.put(MsgConstants.AUTH_USER, userName);
        obj.put(MsgConstants.AUTH_STEP, MsgConstants.AUTH_SHA265_STEP_1);
        obj.put(MsgConstants.AUTH_NONCE, clientNonceBast64);
        obj.put(MsgConstants.AUTH_TYPE, MsgConstants.AUTH_TYPE_MD5_PWD);
        return new AuthVerifySHA256Request(obj);
    }

    public static AuthVerifySHA256Request step2(String userName,
                                                String combineNonceBase64, String clientProofBase64) {
        BSONObject obj = new BasicBSONObject();
        obj.put(MsgConstants.AUTH_USER, userName);
        obj.put(MsgConstants.AUTH_STEP, MsgConstants.AUTH_SHA265_STEP_2);
        obj.put(MsgConstants.AUTH_NONCE, combineNonceBase64);
        obj.put(MsgConstants.AUTH_IDENTIFY, MsgConstants.AUTH_JAVA_IDENTIFY);
        obj.put(MsgConstants.AUTH_PROOF, clientProofBase64);
        obj.put(MsgConstants.AUTH_TYPE, MsgConstants.AUTH_TYPE_MD5_PWD);
        return new AuthVerifySHA256Request(obj);
    }

}
