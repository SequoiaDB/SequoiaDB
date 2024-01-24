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

package com.sequoiadb.flink.sink.writer;

import com.sequoiadb.flink.common.client.SDBClientProvider;
import com.sequoiadb.flink.common.exception.SDBException;
import com.sequoiadb.flink.common.util.RetryUtil;
import com.sequoiadb.flink.config.SDBSinkOptions;
import com.sequoiadb.flink.serde.SDBDataConverter;

import org.apache.commons.compress.utils.Lists;
import org.apache.flink.api.connector.sink.SinkWriter;
import org.apache.flink.table.data.RowData;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.IOException;
import java.util.List;

public class SDBUpsertSinkWriter implements SinkWriter<RowData, Void, Void> {

    private static final Logger LOG = LoggerFactory.getLogger(SDBUpsertSinkWriter.class);

    private static final String MODIFIER_SET = "$set";

    private final SDBDataConverter converter;
    private final SDBSinkOptions sinkOptions;
    private final SDBClientProvider provider;

    private final String[] upsertKeys;

    public SDBUpsertSinkWriter(SDBDataConverter converter, SDBSinkOptions sinkOptions) {
        this.converter = converter;
        this.sinkOptions = sinkOptions;

        this.provider = SDBClientProvider.builder()
                .withHosts(sinkOptions.getHosts())
                .withCollectionSpace(sinkOptions.getCollectionSpace())
                .withCollection(sinkOptions.getCollection())
                .withUsername(sinkOptions.getUsername())
                .withPassword(sinkOptions.getPassword())
                .withOptions(sinkOptions)
                .build();
        this.upsertKeys = sinkOptions.getUpsertKey();
    }

    /**
     * in upsert mode, the changelogs is always ordered (Primary Key level).
     * so we can flush each changelog directly.
     *
     * @param element The input record
     * @param context The additional information about the input record
     * @throws IOException
     * @throws InterruptedException
     */
    @Override
    public void write(RowData element, Context context)
            throws IOException, InterruptedException {
        BSONObject record = converter
                .toExternal(element, sinkOptions.getIgnoreNullField());

        RetryUtil.retryWhenRuntimeException(() -> {
            switch (element.getRowKind()) {
                case INSERT:
                case UPDATE_AFTER:
                    provider.getCollection().upsert(
                            createMatcher(record),
                            createModifier(MODIFIER_SET, record),
                            null,
                            record, 0);
                    break;

                case DELETE:
                    provider.getCollection().delete(createMatcher(record));
                    break;
            }
            return null;
        }, RetryUtil.DEFAULT_MAX_RETRY_TIMES, RetryUtil.DEFAULT_RETRY_DURATION, true);
    }

    private BSONObject createMatcher(BSONObject record) {
        BSONObject matcher = new BasicBSONObject();

        for (String upsertKey : upsertKeys) {
            matcher.put(upsertKey, record.get(upsertKey));
        }
        return matcher;
    }

    private BSONObject createModifier(String mType, BSONObject updater) {
        BSONObject modifier = new BasicBSONObject();
        modifier.put(mType, updater);
        return modifier;
    }

    @Override
    public List<Void> prepareCommit(boolean flush) throws IOException, InterruptedException {
        return Lists.newArrayList();
    }

    @Override
    public void close() throws Exception {
        if (provider != null) {
            provider.close();
        }
    }

}
