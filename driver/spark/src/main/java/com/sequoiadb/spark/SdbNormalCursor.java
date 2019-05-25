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

package com.sequoiadb.spark;

import com.sequoiadb.base.DBCursor;
import org.bson.BSONObject;

class SdbNormalCursor implements SdbCursor {
    private final DBCursor cursor;

    public SdbNormalCursor(DBCursor cursor) {
        if (cursor == null) {
            throw new SdbException("Cursor is null.");
        }

        this.cursor = cursor;
    }

    @Override
    public boolean hasNext() {
        return cursor.hasNext();
    }

    @Override
    public BSONObject next() {
        return cursor.getNext();
    }

    @Override
    public void close() {
        cursor.close();
    }
}
