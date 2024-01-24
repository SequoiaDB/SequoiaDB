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

package com.sequoiadb.flink.table;

import com.sequoiadb.flink.common.exception.SDBException;
import com.sequoiadb.flink.common.util.LookupUtil;
import com.sequoiadb.flink.config.SDBSourceOptions;
import com.sequoiadb.flink.serde.SDBDataConverter;
import com.sequoiadb.flink.source.SDBLookupTableFunction;
import com.sequoiadb.flink.source.SDBSource;
import com.sequoiadb.flink.table.pushdown.FilterPushDownSupport;

import org.apache.flink.table.connector.ChangelogMode;
import org.apache.flink.table.connector.source.TableFunctionProvider;
import org.apache.flink.table.connector.source.DynamicTableSource;
import org.apache.flink.table.connector.source.LookupTableSource;
import org.apache.flink.table.connector.source.ScanTableSource;
import org.apache.flink.table.connector.source.SourceProvider;
import org.apache.flink.table.connector.source.abilities.SupportsFilterPushDown;
import org.apache.flink.table.connector.source.abilities.SupportsLimitPushDown;
import org.apache.flink.table.connector.source.abilities.SupportsProjectionPushDown;
import org.apache.flink.table.expressions.ResolvedExpression;
import org.apache.flink.table.types.DataType;
import org.apache.flink.table.types.FieldsDataType;
import org.apache.flink.table.types.logical.LogicalType;
import org.apache.flink.table.types.logical.RowType;
import org.apache.flink.types.RowKind;
import org.bson.BSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.List;
import java.util.ArrayList;
import java.util.Set;
import java.util.HashSet;

public class SDBDynamicTableSource implements ScanTableSource,
        SupportsProjectionPushDown,
        SupportsFilterPushDown,
        SupportsLimitPushDown,
        LookupTableSource {

    private static final Logger LOG = LoggerFactory.getLogger(SDBDynamicTableSource.class);

    private final SDBSourceOptions sourceOptions;

    private DataType producedDatatype;
    private long limit = -1;

    private BSONObject matcher;

    public SDBDynamicTableSource(SDBSourceOptions sourceOptions,
                                 DataType produceDatatype) {
        LOG.info("source options: {}", sourceOptions);
        this.sourceOptions = sourceOptions;
        this.producedDatatype = produceDatatype;
    }

    @Override
    public DynamicTableSource copy() {
        return new SDBDynamicTableSource(sourceOptions, producedDatatype);
    }

    @Override
    public String asSummaryString() {
        return "SequoiaDB Dynamic Table Source";
    }

    @Override
    public ChangelogMode getChangelogMode() {
        return ChangelogMode.newBuilder()
                .addContainedKind(RowKind.INSERT)
                .build();
    }

    @Override
    public ScanRuntimeProvider getScanRuntimeProvider(ScanContext runtimeProviderContext) {
        SDBDataConverter dataConverter
                = new SDBDataConverter(((RowType) producedDatatype.getLogicalType()));

        return SourceProvider.of(new SDBSource(
                dataConverter,
                sourceOptions,
                ((RowType) producedDatatype.getLogicalType()).getFieldNames(),
                matcher,
                limit));
    }

    @Override
    public void applyLimit(long limit) {
        this.limit = limit;
    }

    @Override
    public boolean supportsNestedProjection() {
        return false;
    }

    @Override
    public void applyProjection(int[][] requiredColumns) {
        final List<RowType.RowField> updatedFields = new ArrayList<>();
        final List<DataType> updatedChildren = new ArrayList<>();
        Set<String> nameDomain = new HashSet<>();

        int duplicateCount = 0;
        DataType dataType = producedDatatype;
        for (int[] requiredColumn : requiredColumns) {
            DataType fieldType = dataType.getChildren().get(requiredColumn[0]);
            LogicalType fieldLogicalType = fieldType.getLogicalType();

            StringBuilder builder =
                    new StringBuilder(((RowType) dataType.getLogicalType())
                            .getFieldNames()
                            .get(requiredColumn[0]));

            for (int i = 1; i < requiredColumn.length; i++) {
                RowType rowType = (RowType) fieldLogicalType;
                builder.append("_").append(rowType.getFieldNames().get(requiredColumn[i]));
                fieldLogicalType = rowType.getFields().get(requiredColumn[i]).getType();
                fieldType = fieldType.getChildren().get(requiredColumn[i]);
            }

            String path = builder.toString();
            while (nameDomain.contains(path)) {
                path = builder.append("_$").append(duplicateCount++).toString();
            }
            updatedFields.add(new RowType.RowField(path, fieldLogicalType));
            updatedChildren.add(fieldType);
            nameDomain.add(path);
        }

        this.producedDatatype = new FieldsDataType(
                new RowType(dataType.getLogicalType().isNullable(), updatedFields),
                dataType.getConversionClass(),
                updatedChildren
        );
    }

    @Override
    public Result applyFilters(List<ResolvedExpression> resolvedExpressionList) {
        matcher = FilterPushDownSupport.toBsonMatcher(resolvedExpressionList);
        //return all expression to flink,internal processing returned expressions
        return Result.of(new ArrayList<>(), resolvedExpressionList);
    }

    /**
     * the below function implement to LookupTableSource Interface.
     * This function is called when the user uses the lookup syntax
     * in the flink-sdb connector.
     *
     * @param context provided by flink to get joined fields
     * @return LookupRuntimeProvider
     */

    @Override
    public LookupRuntimeProvider getLookupRuntimeProvider(LookupContext context) {

        int[][] keys = context.getKeys();

        if (LookupUtil.isNestedType(keys)) {
            throw new SDBException("LookupTableSource doesn't support nested types when using SequoiaDB as the source/sink.");
        }

        RowType joinedRowType = LookupUtil.getJoinedRowType(producedDatatype, keys);

        return TableFunctionProvider.of(
                new SDBLookupTableFunction(producedDatatype, sourceOptions, joinedRowType));
    }

}