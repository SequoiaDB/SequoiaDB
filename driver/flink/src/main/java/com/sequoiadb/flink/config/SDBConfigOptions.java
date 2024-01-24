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

package com.sequoiadb.flink.config;

import java.io.Serializable;

import org.apache.flink.configuration.ConfigOption;
import org.apache.flink.configuration.ConfigOptions;

public class SDBConfigOptions implements Serializable {

    // sdb
    public static final ConfigOption<String> HOSTS =
            ConfigOptions.key("hosts").stringType().noDefaultValue();
    public static final ConfigOption<String> COLLECTION_SPACE =
            ConfigOptions.key("collectionspace").stringType().noDefaultValue();
    public static final ConfigOption<String> COLLECTION =
            ConfigOptions.key("collection").stringType().noDefaultValue();
    public static final ConfigOption<String> FORMAT = ConfigOptions.key("format").stringType().defaultValue("bson");
    public static final ConfigOption<String> USERNAME = ConfigOptions.key("username").stringType().defaultValue("");
    public static final ConfigOption<String> PASSWORD_TYPE =
            ConfigOptions.key("passwordtype").stringType().defaultValue("cleartext");
    public static final ConfigOption<String> PASSWORD = ConfigOptions.key("password").stringType().defaultValue("");
    public static final ConfigOption<String> TOKEN = ConfigOptions.key("token").stringType().defaultValue("");

    // read
    public static final ConfigOption<String> SPLIT_MODE =
            ConfigOptions.key("splitmode").stringType().defaultValue("auto");
    public static final ConfigOption<Integer> SPLIT_BLOCK_NUM =
            ConfigOptions.key("splitblocknum").intType().defaultValue(4);

    public static final ConfigOption<Integer> SPLIT_MAX_NUM =
            ConfigOptions.key("splitmaxnum").intType().defaultValue(1000);
    public static final ConfigOption<String> PREFERRED_INSTANCE =
            ConfigOptions.key("preferredinstance").stringType().defaultValue("M");
    public static final ConfigOption<String> PREFERRED_INSTANCE_MODE =
            ConfigOptions.key("preferredinstancemode").stringType().defaultValue("random");
    public static final ConfigOption<Boolean> PREFERRED_INSTANCE_STRICT =
            ConfigOptions.key("preferredinstancestrict").booleanType().defaultValue(false);

    // write
    public static final ConfigOption<Boolean> IGNORE_NULL_FIELD =
            ConfigOptions.key("ignorenullfield").booleanType().defaultValue(false);
    public static final ConfigOption<Integer> BULK_SIZE = ConfigOptions.key("bulksize").intType().defaultValue(500);
    public static final ConfigOption<Integer> PAGE_SIZE = ConfigOptions.key("pagesize").intType().defaultValue(65536);
    public static final ConfigOption<String> DOMAIN = ConfigOptions.key("domain").stringType().noDefaultValue();
    public static final ConfigOption<String> SHARDING_KEY =
            ConfigOptions.key("shardingkey").stringType().noDefaultValue();
    public static final ConfigOption<String> SHARDING_TYPE =
            ConfigOptions.key("shardingtype").stringType().defaultValue("hash");
    public static final ConfigOption<Integer> REPL_SIZE = ConfigOptions.key("replsize").intType().defaultValue(1);
    public static final ConfigOption<String> COMPRESSION_TYPE =
            ConfigOptions.key("compressiontype").stringType().defaultValue("lzw");
    public static final ConfigOption<String> GROUP = ConfigOptions.key("group").stringType().noDefaultValue();
    public static final ConfigOption<Boolean> AUTO_PARTITION =
            ConfigOptions.key("autopartition").booleanType().defaultValue(true);
    public static final ConfigOption<Integer> SINK_PARALLELISM =
            ConfigOptions.key("parallelism").intType().defaultValue(1);
    public static final ConfigOption<Long> MAX_BULK_FILL_TIME =
            ConfigOptions.key("maxbulkfilltime").longType().defaultValue(300L);
    public static final ConfigOption<Boolean> OVERWRITE =
            ConfigOptions.key("overwrite").booleanType().defaultValue(true);

    public static final ConfigOption<String> WRITE_MODE =
            ConfigOptions.key("writemode").stringType().defaultValue("append-only");

    public static final ConfigOption<Boolean> SINK_RETRACT_PARTITIONED_SOURCE =
            ConfigOptions
                    .key("sink.retract.partitioned-source")
                    .booleanType()
                    .defaultValue(false)
                    .withDescription("whether upstreaming source is multi-partitioned or not.");

    public static final ConfigOption<String> SINK_RETRACT_EVENT_TS_FIELD_NAME =
            ConfigOptions
                    .key("sink.retract.event-ts-field-name")
                    .stringType()
                    .noDefaultValue()
                    .withDescription("In retract mode, user must specify event timestamp " +
                            "from business fields to ensure changelog order");

    public static final ConfigOption<Integer> SINK_RETRACT_STATE_TTL =
            ConfigOptions
                    .key("sink.retract.state-ttl")
                    .intType()
                    .defaultValue(1)
                    .withDescription("Time To Live in retract mode, used for state cleanup periodically.");

}
