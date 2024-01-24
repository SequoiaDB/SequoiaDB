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

package com.sequoiadb.message.response;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.message.SdbProtocolVersion;
import com.sequoiadb.util.Helper;
import org.bson.BSONObject;

import java.nio.ByteBuffer;

public abstract class CommonResponse extends SdbResponse {
    protected static final int FIXED_LENGTH = 48;
    protected long contextId;
    protected int flag;
    protected int startFrom;
    protected int returnedNum;
    private int returnedMask = 0;
    private int dataLen;

    private BSONObject errorObj;
    private BSONObject resultBSON;
    private BSONObject processBSON;

    protected CommonResponse() {
        length = FIXED_LENGTH;
    }

    public long getContextId() {
        return contextId;
    }

    public int getFlag() {
        return flag;
    }

    public int getStartFrom() {
        return startFrom;
    }

    public int getReturnedNum() {
        return returnedNum;
    }

    public BSONObject getErrorObj() {
        return errorObj;
    }

    @Override
    protected void decodeBody(ByteBuffer in, SdbProtocolVersion version) {
        switch ( version ) {
            case SDB_PROTOCOL_VERSION_V1:
                decodeCommonBodyV1( in );
                break;
            case SDB_PROTOCOL_VERSION_V2:
                decodeCommonBodyV2( in );
                break;
            default:
                throw new BaseException( SDBError.SDB_NET_BROKEN_MSG,
                        "Message protocol version error!" );
        }
    }

    private void decodeCommonBodyV1(ByteBuffer in) {
        contextId = in.getLong();
        flag = in.getInt();
        startFrom = in.getInt();
        returnedNum = in.getInt();
        if (flag == 0) {
            decodeData(in);
        } else {
            if (in.hasRemaining()) {
                errorObj = Helper.decodeBSONObject(in);
            }
        }
    }

    private void decodeCommonBodyV2(ByteBuffer in) {
        contextId = in.getLong();
        flag = in.getInt();
        startFrom = in.getInt();
        returnedNum = in.getInt();
        returnedMask = in.getInt();
        dataLen = in.getInt();
        if (flag == 0) {
            decodeData(in);
        } else {
            if (in.hasRemaining()) {
                errorObj = Helper.decodeBSONObject(in);
            }
        }
        decodeTailContent();
    }

    public void decodeTailContent() {
        // resultBSON, processBSON is an extension, currently not in use.
        // temporarily not resolved.
    }

    protected abstract void decodeData(ByteBuffer in);
}
