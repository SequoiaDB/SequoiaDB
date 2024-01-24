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

import java.nio.ByteBuffer;

public abstract class LobRequest extends SdbRequest {
    protected static final int LOB_HEADER_LENGTH = 80; // HEADER_LENGTH + 24
    protected static final int version = 1;
    protected static final short w = 1;
    protected static final short padding = 0;
    protected int flag = 0;
    protected long contextId = -1;
    protected int bsonLength = 0;

    protected LobRequest() {
        length = LOB_HEADER_LENGTH;
    }

    @Override
    protected void encodeBody(ByteBuffer out) {
        encodeLobHeader(out);
        encodeLobBody(out);
    }

    protected void encodeLobHeader(ByteBuffer out) {
        out.putInt(version);
        out.putShort(w);
        out.putShort(padding);
        out.putInt(flag);
        out.putLong(contextId);
        out.putInt(bsonLength);
    }

    protected abstract void encodeLobBody(ByteBuffer out);
}
