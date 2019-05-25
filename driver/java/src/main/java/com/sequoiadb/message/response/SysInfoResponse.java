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

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.message.MsgOpCode;
import com.sequoiadb.message.SysInfoHeader;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;

/**
 * @since 2.9
 */
public class SysInfoResponse extends SysInfoHeader implements Response {
    private static final int LENGTH = 128;
    private int osType;
    private ByteOrder byteOrder;

    @Override
    public int length() {
        return LENGTH;
    }

    @Override
    public int opCode() {
        return MsgOpCode.SYS_INFO_RESP;
    }

    public int osType() {
        return osType;
    }

    public ByteOrder byteOrder() {
        return byteOrder;
    }

    @Override
    public void decode(ByteBuffer in) {
        in.order(ByteOrder.BIG_ENDIAN);
        specialSysInfoLen = in.getInt();
        eyeCatcher = in.getInt();
        if (eyeCatcher == SYS_INFO_EYE_CATCHER) {
            byteOrder = ByteOrder.BIG_ENDIAN;
        } else if (eyeCatcher == SYS_INFO_EYE_CATCHER_REVERT) {
            byteOrder = ByteOrder.LITTLE_ENDIAN;
        } else {
            throw new BaseException(SDBError.SDB_INVALIDARG, String.format("Invalid eyecatcher: %x", eyeCatcher));
        }
        in.order(byteOrder);
        realMsgLen = in.getInt();
        osType = in.getInt();
    }
}
