/*
 * Copyright 2022 SequoiaDB Inc.
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

package com.sequoiadb.flink.source.iterator;

import com.sequoiadb.base.ConfigOptions;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.flink.common.exception.SDBException;
import com.sequoiadb.flink.common.util.SDBInfoUtil;
import com.sequoiadb.flink.config.SDBSourceOptions;
import com.sequoiadb.flink.config.SplitMode;
import com.sequoiadb.flink.source.split.SDBSplit;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;

import java.io.Closeable;
import java.io.IOException;
import java.util.Iterator;

/**
 * SDBIterator is for reading bson raw bytes from SequoiaDB.
 * It's a wrapper of DBCursor, so that some extra work can be done
 * in one place when constructing the DBCursor, such as obtain cs/cl,
 * build hint object, etc.
 */
public class SDBIterator implements Iterator<byte[]>, Closeable {

    private DBCursor cursor;
    private Sequoiadb sdb;

    public SDBIterator(SDBSplit split, SDBSourceOptions sourceOptions, BSONObject matcher, BSONObject selector,
                       long limit) {
        init(split, sourceOptions, matcher, selector, limit);
    }

    private void init(SDBSplit split, SDBSourceOptions sourceOptions, BSONObject matcher, BSONObject selector,
                      long limit) {
        sdb = new Sequoiadb(
                split.getUrls(),
                sourceOptions.getUsername(),
                sourceOptions.getPassword(),
                new ConfigOptions()
        );

        // set up source info in session attr, ignore failure and just
        // print warning log when throws exception.
        SDBInfoUtil.setupSourceSessionAttrIgnoreFailures(sdb, sourceOptions.getSourceInfo());

        DBCollection cl = null;
        try {
            cl = sdb.getCollectionSpace(split.getCsName())
                    .getCollection(split.getClName());
        } catch (BaseException ex) {
            if (ex.getErrorCode() == SDBError.SDB_DMS_CS_NOTEXIST.getErrorCode()) {
                throw new SDBException(
                        String.format("collection space %s does not exist on node: %s.",
                                sourceOptions.getCollectionSpace(), sdb.getNodeName()));
            } else if (ex.getErrorCode() == SDBError.SDB_DMS_NOTEXIST.getErrorCode()) {
                throw new SDBException(
                        String.format("collection %s does not exist on node: %s.",
                                sourceOptions.getCollection(), sdb.getNodeName()));
            }
            throw ex;
        }

        // construct hint object for datablock mode (aka tbscan).
        // hint object including datablocks and scan type.
        BSONObject hint = new BasicBSONObject();
        if (split.getMode() == SplitMode.DATA_BLOCK) {
            BasicBSONList dataBlocks = new BasicBSONList();
            dataBlocks.addAll(split.getDataBlocks());

            BSONObject metaObj = new BasicBSONObject();
            metaObj.put("Datablocks", dataBlocks);
            metaObj.put("ScanType", "tbscan");

            hint.put("$Meta", metaObj);
        }

        cursor = cl.query(matcher, selector, null, hint, 0, limit, 0);
    }

    @Override
    public boolean hasNext() {
        return cursor.hasNext();
    }

    @Override
    public byte[] next() {
        return cursor.getNextRaw();
    }

    @Override
    public void close() throws IOException {
        if (!sdb.isClosed()) {
            cursor.close();
            sdb.close();
        }
    }

}
