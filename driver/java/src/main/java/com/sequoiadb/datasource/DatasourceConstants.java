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

package com.sequoiadb.datasource;

/**
 * Created by tanzhaobo on 2018/1/20.
 */
final class DatasourceConstants {
    private DatasourceConstants() {
    }

    // Compatible with old versions
    final static String FIELD_NAME_PREFERRED_INSTANCE_LEGACY = "PreferedInstance";
    final static String FIELD_NAME_PREFERRED_INSTANCE_MODE_LEGACY = "PreferedInstanceMode";

    final static String FIELD_NAME_PREFERRED_INSTANCE = "PreferredInstance";
    final static String FIELD_NAME_PREFERRED_INSTANCE_MODE = "PreferredInstanceMode";
    final static String FIELD_NAME_SESSION_TIMEOUT = "Timeout";

    final static String PREFERRED_INSTANCE_MODE_RANDOM = "random";
    final static String PREFERRED_INSTANCE_MODE_ORDERED = "ordered";
}
