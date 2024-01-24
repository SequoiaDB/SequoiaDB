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
import com.sequoiadb.message.MsgOpCode;
import com.sequoiadb.message.SdbAuthVersion;
import com.sequoiadb.message.SdbProtocolVersion;
import com.sequoiadb.message.SysInfoHeader;
import com.sequoiadb.util.Helper;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.Arrays;

/**
 * @since 2.9
 */
public class SysInfoResponse extends SysInfoHeader implements Response {
    private static final int LENGTH = 128;
    private int osType;
    private int authVersion;
    private long dbStartTime;
    private byte version;
    private byte subVersion;
    private byte fixVersion;
    private byte pad[] = new byte[93];
    // Fingerprint of the reply message. Actually part of the md5 value.
    private byte fingerprint[] = new byte[4];


    private ByteOrder byteOrder;
    private SdbProtocolVersion peerProtocolVersion = SdbProtocolVersion.SDB_PROTOCOL_VERSION_INVALID;

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

    public SdbProtocolVersion getPeerProtocolVersion() {
        return peerProtocolVersion;
    }

    public SdbAuthVersion getAuthVersion() {
        if ( authVersion == 1 ) {
            return SdbAuthVersion.SDB_AUTH_SCRAM_SHA256;
        } else {
            return SdbAuthVersion.SDB_AUTH_MD5;
        }
    }

    @Override
    public void decode(ByteBuffer in, SdbProtocolVersion protocolVersion) {
        // Java platform is BIG_ENDIAN
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
        authVersion = in.getInt();
        dbStartTime = in.getLong();
        version = in.get();
        subVersion = in.get();
        fixVersion = in.get();
        for ( int i = 0; i < pad.length; i++ ){
            pad[i] = in.get();
        }
        for ( int i = 0; i < fingerprint.length; i++ ){
            fingerprint[i] = in.get();
        }

        in.rewind();
        in.limit( realMsgLen - 4 );
        byte[] actualMD5 = Helper.genMD5( in, 4 );
        peerProtocolVersion = Arrays.equals( actualMD5, fingerprint ) ?
                SdbProtocolVersion.SDB_PROTOCOL_VERSION_V2 : SdbProtocolVersion.SDB_PROTOCOL_VERSION_V1;
    }
}
