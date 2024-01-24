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
import com.sequoiadb.message.SdbMsgHeader;
import com.sequoiadb.message.SdbProtocolVersion;

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
    public void decode( ByteBuffer in, SdbProtocolVersion version ) {
        switch ( version ) {
            case SDB_PROTOCOL_VERSION_V1:
                decodeMsgHeaderV1( in );
                break;
            case SDB_PROTOCOL_VERSION_V2:
                decodeMsgHeaderV2( in );
                break;
            default:
                throw new BaseException( SDBError.SDB_NET_BROKEN_MSG,
                        "Message protocol version error!" );
        }
        decodeBody(in, version);
    }

    protected void decodeMsgHeaderV1(ByteBuffer in) {
        length = in.getInt();
        opCode = in.getInt();
        tid = in.getInt();
        routeId = in.getLong();
        requestId = in.getLong();
    }

    protected void decodeMsgHeaderV2(ByteBuffer in) {
        length = in.getInt();
        eye = in.getInt();
        tid = in.getInt();
        routeId = in.getLong();
        requestId = in.getLong();
        opCode = in.getInt();
        version = in.getShort();
        flags = in.getShort();
        globalID.decode( in );
        for ( int i = 0; i < reserve.length; i++ ){
            reserve[i] = in.get();
        }
    }

    protected abstract void decodeBody(ByteBuffer in, SdbProtocolVersion version);
}
