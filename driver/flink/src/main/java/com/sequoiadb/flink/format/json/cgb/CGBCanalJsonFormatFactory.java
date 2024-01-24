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

package com.sequoiadb.flink.format.json.cgb;

import com.sequoiadb.flink.common.exception.SDBException;
import org.apache.flink.api.common.serialization.DeserializationSchema;
import org.apache.flink.configuration.ConfigOption;
import org.apache.flink.configuration.ReadableConfig;
import org.apache.flink.formats.common.TimestampFormat;
import org.apache.flink.formats.json.JsonFormatOptionsUtil;
import org.apache.flink.table.connector.format.DecodingFormat;
import org.apache.flink.table.data.RowData;
import org.apache.flink.table.factories.DeserializationFormatFactory;
import org.apache.flink.table.factories.DynamicTableFactory;
import org.apache.flink.table.factories.FactoryUtil;

import java.util.Collections;
import java.util.HashSet;
import java.util.Set;

import static com.sequoiadb.flink.format.json.cgb.CGBCanalJsonFormatOptions.*;

public class CGBCanalJsonFormatFactory
        implements DeserializationFormatFactory {

    public static final String IDENTIFIER = "cgb-canal";

    private static final Set<ConfigOption<?>> OPTIONAL_OPTIONS = new HashSet<>();

    static {
        OPTIONAL_OPTIONS.add(IGNORE_PARSE_ERRORS);
        OPTIONAL_OPTIONS.add(TIMESTAMP_FORMAT);
        OPTIONAL_OPTIONS.add(JSON_MAP_NULL_KEY_MODE);
        OPTIONAL_OPTIONS.add(JSON_MAP_NULL_KEY_LITERAL);
        OPTIONAL_OPTIONS.add(PRIMARY_KEYS);
        OPTIONAL_OPTIONS.add(CHANGELOG_PARTITION_POLICY);
    }

    /** Validator for gdb canal decoding format. **/
    private static void validateDecodingFormatOptions(ReadableConfig tableOptions) {
        JsonFormatOptionsUtil.validateDecodingFormatOptions(tableOptions);
    }

    @Override
    public DecodingFormat<DeserializationSchema<RowData>> createDecodingFormat(
            DynamicTableFactory.Context context, ReadableConfig formatOptions) {

        FactoryUtil.validateFactoryOptions(this, formatOptions);
        validateDecodingFormatOptions(formatOptions);

        // get options
        final boolean ignoreParseErrors = formatOptions.get(IGNORE_PARSE_ERRORS);

        final TimestampFormat timestampFormat =
                JsonFormatOptionsUtil.getTimestampFormat(formatOptions);

        final String upsertKeyStr = formatOptions.get(PRIMARY_KEYS);
        if (upsertKeyStr == null) {
            throw new SDBException("can't perform changelogs streaming write without specifying primary keys.");
        }
        final String[] upsertKey = upsertKeyStr.split(",");

        String cPartitionPolicy = formatOptions.get(CHANGELOG_PARTITION_POLICY);

        return new CGBCanalJsonDecodingFormat(ignoreParseErrors, timestampFormat, upsertKey, cPartitionPolicy);
    }

    @Override
    public String factoryIdentifier() {
        return IDENTIFIER;
    }

    @Override
    public Set<ConfigOption<?>> requiredOptions() {
        return Collections.emptySet();
    }

    @Override
    public Set<ConfigOption<?>> optionalOptions() {
        return OPTIONAL_OPTIONS;
    }

}
