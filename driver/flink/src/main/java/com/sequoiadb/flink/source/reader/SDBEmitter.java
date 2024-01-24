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

package com.sequoiadb.flink.source.reader;

import com.sequoiadb.flink.serde.SDBDataConverter;
import com.sequoiadb.flink.source.split.SDBSplit;
import org.apache.flink.api.connector.source.SourceOutput;
import org.apache.flink.connector.base.source.reader.RecordEmitter;
import org.apache.flink.table.data.RowData;
import org.bson.BSON;

/**
 * SDBRecordEmitter process and emit bson raw bytes to SourceOutput
 */
public class SDBEmitter implements RecordEmitter<byte[], RowData, SDBSplit> {

    private final SDBDataConverter dataConverter;

    public SDBEmitter(SDBDataConverter dataConverter) {
        this.dataConverter = dataConverter;
    }

    /**
     * emit single record read from SequoiaDB
     *
     * @param element bson raw bytes
     * @param output output to flink
     * @param splitState The state of the split. for batch read, split is stateless.
     * @throws Exception
     */
    @Override
    public void emitRecord(byte[] element,
                           SourceOutput<RowData> output,
                           SDBSplit splitState) throws Exception {
        output.collect(dataConverter.toInternal(BSON.decode(element)));
    }

}
