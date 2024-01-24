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
import com.sequoiadb.message.SdbProtocolVersion;
import com.sequoiadb.message.SysInfoHeader;

import java.nio.ByteBuffer;

/**
 * @since 2.9
 */
public class SysInfoRequest extends SysInfoHeader implements Request {
    @Override
    public int length() {
        return HEADER_LENGTH;
    }

    @Override
    public int opCode() {
        return MsgOpCode.SYS_INFO_REQ;
    }

    @Override
    public void setRequestId(long requestId) {
    }

    @Override
    public void encode( ByteBuffer out, SdbProtocolVersion version ) {
        out.putInt(SYS_INFO_SPECIAL_LEN);
        out.putInt(SYS_INFO_EYE_CATCHER);
        out.putInt(HEADER_LENGTH);
    }
}
