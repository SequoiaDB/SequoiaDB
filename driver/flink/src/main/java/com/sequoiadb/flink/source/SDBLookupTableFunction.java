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
package com.sequoiadb.flink.source;


import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.flink.common.client.SDBClientProvider;
import com.sequoiadb.flink.common.client.SDBCollectionProvider;
import com.sequoiadb.flink.common.util.SDBInfoUtil;
import com.sequoiadb.flink.config.SDBSourceOptions;
import com.sequoiadb.flink.serde.SDBDataConverter;
import org.apache.flink.table.annotation.FunctionHint;
import org.apache.flink.table.data.GenericRowData;
import org.apache.flink.table.data.RowData;
import org.apache.flink.table.functions.FunctionContext;
import org.apache.flink.table.functions.TableFunction;
import org.apache.flink.table.types.DataType;
import org.apache.flink.table.types.logical.RowType;
import org.bson.BSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;


/**
 * A SDBTableFunctionSource that looks up rows of an external storage system
 * by one or more keys during runtime.Compared to ScanTableSource, the source
 * does not have to read the entire table and can lazily fetch individual
 * values from a (possibly continuously changing) external table when necessary.
 * The lookupTableSource only support emitting insert-only changes currently.
 * This class will be called by{@link com.sequoiadb.flink.table.SDBDynamicTableSource}
 * for obtaining a provider of runtime implementation.
 */

public class SDBLookupTableFunction extends TableFunction<RowData> {

    private static final Logger LOG = LoggerFactory.getLogger(SDBLookupTableFunction.class);

    // Table Fields construction
    private DataType produceType;
    // SDB parameter
    private final SDBSourceOptions sourceOptions;

    //  "RowData => BSONObject" converter
    private final SDBDataConverter rowDataToBsonConverter;
    // "BSONObject => RowData" converter
    private final SDBDataConverter bsonToRowDataConverter;

    private final SDBCollectionProvider sdbCollectionProvider;

    private DBCollection cl;


    public SDBLookupTableFunction(DataType produceType, SDBSourceOptions sourceOptions, RowType joinedRowType) {

        this.produceType = produceType;
        this.sourceOptions = sourceOptions;
        this.rowDataToBsonConverter = new SDBDataConverter(joinedRowType);
        this.bsonToRowDataConverter = new SDBDataConverter((RowType) produceType.getLogicalType());
        sdbCollectionProvider = (SDBCollectionProvider) SDBClientProvider.builder()
                .withHosts(sourceOptions.getHosts())
                .withUsername(sourceOptions.getUsername())
                .withPassword(sourceOptions.getPassword())
                .withCollectionSpace(sourceOptions.getCollectionSpace())
                .withCollection(sourceOptions.getCollection())
                .build();
    }


    /**
     * check if the index uses
     * lookupTableSource Thread start the first method of execution
     *
     * @param context save flink global runtime information
     * @throws Exception
     */
    @Override
    public void open(FunctionContext context) throws Exception {
        sdbCollectionProvider.setupSourceInfo(
                SDBInfoUtil.generateSourceInfo(context.getMetricGroup()));

        cl = sdbCollectionProvider.getCollection();
        // get sdb client to obtain indexes
        List<String> indexNameList = sdbCollectionProvider.getIndexColumnNames();

        // get joined fields
        RowType joinedRowType = rowDataToBsonConverter.getRowType();
        List<String> joinedFields = joinedRowType.getFieldNames();

        List<String> noIndexColumns = new ArrayList<>();

        // compare whether their joined fields includes an index
        for (String joinedField : joinedFields) {
            if (!indexNameList.contains(joinedField)) {
                noIndexColumns.add(joinedField);
            }
        }

        if (noIndexColumns.size() > 0) {
            LOG.warn("Column (" + String.join(",", noIndexColumns) + ") used by join are not part of any indexes in Sequoiadb!" +
                    " You may consider creating indexes using the join fields.");
        }
        super.open(context);
    }


    /**
     * Flink calls the eval() method through reflection.The eval() method
     * is the main logic implementation of flink lookup join.
     * The eval() method helps us realize the check effect of Lookup join
     * on Sequoiadb.
     *
     * @param rowKeys a object from streaming table
     */

    @FunctionHint
    public void eval(Object... rowKeys) throws IOException {

        // convert the incoming parameter to a RowData type
        RowData keyRow = GenericRowData.of(rowKeys);

        // get matcher converted to keyRow by sdbDataConverter
        BSONObject matcher = rowDataToBsonConverter.toExternal(keyRow, true);

        // send the dim result one by one
        try (DBCursor dimCursor = cl.query(matcher, null, null, null)) {
            while (dimCursor.hasNext()) {
                BSONObject dimBSONObj = dimCursor.getNext();
                RowData dimRowData = bsonToRowDataConverter.toInternal(dimBSONObj);
                collect(dimRowData);
            }
        }
    }

    /**
     * close sdbCollectionProvider
     *
     * @throws Exception
     */
    @Override
    public void close() throws Exception {
        sdbCollectionProvider.close();
        super.close();
    }
}
