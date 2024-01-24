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

package com.sequoiadb.message;

/**
 * @since 2.9
 */
public abstract class SdbMsgHeader {
    protected static final int HEADER_LENGTH = 56;
    protected static final int HEADER_LENGTH_V1 = 28;
    protected int length;
    protected int eye = 0;
    protected int tid;  // client thead id
    protected long routeId;
    protected long requestId;  // identifier for this message
    protected int opCode;
    protected short version = SdbProtocolVersion.SDB_PROTOCOL_VERSION_V2.getCode();
    protected short flags;  // Common flags for a query
    protected MsgGlobalID globalID = new MsgGlobalID();  // globalID 16 bytes
    protected byte reserve[] = new byte[ 4 ]; // 4 bytes, default is 0
}