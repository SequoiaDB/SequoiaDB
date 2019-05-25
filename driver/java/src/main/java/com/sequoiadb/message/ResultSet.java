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

package com.sequoiadb.message;

import com.sequoiadb.util.Helper;
import org.bson.BSONObject;

import java.nio.ByteBuffer;

/**
 * @since 2.9
 */
public final class ResultSet {
    private final ByteBuffer buffer;
    private final int num;
    private int index;

    public ResultSet(ByteBuffer buffer, int num) {
        this.buffer = buffer;
        this.num = num;
    }

    public final int getNum() {
        return num;
    }

    public final int getIndex() {
        return index;
    }

    public final boolean hasNext() {
        return index < num;
    }

    public BSONObject getNext() {
        if (hasNext()) {
            index++;
            return Helper.decodeBSONObject(buffer);
        } else {
            return null;
        }
    }

    public byte[] getNextRaw() {
        if (hasNext()) {
            index++;
            return Helper.decodeBSONBytes(buffer);
        } else {
            return null;
        }
    }
}
