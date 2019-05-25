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

import java.io.UnsupportedEncodingException;
import java.nio.ByteBuffer;

public class SQLRequest extends SdbRequest {
    private String sql;

    public SQLRequest(String sql) {
        opCode = MsgOpCode.SQL_REQ;

        this.sql = sql;
        length += sql.length() + 1;
    }

    @Override
    protected void encodeBody(ByteBuffer out) {
        try {
            out.put(sql.getBytes("UTF-8"));
            out.put((byte) 0);
        } catch (UnsupportedEncodingException e) {
            throw new BaseException(SDBError.SDB_INVALIDARG, e);
        }
    }
}