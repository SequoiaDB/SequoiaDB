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

package com.sequoiadb.datasource;

/**
 * Created by tanzhaobo on 2018/1/20.
 */
final class DatasourceConstants {
    private DatasourceConstants() {
    }

    final static String FIELD_NAME_PREFERED_INSTANCE = "PreferedInstance";
    final static String FIELD_NAME_PREFERED_INSTANCE_MODE = "PreferedInstanceMode";
    final static String FIELD_NAME_SESSION_TIMEOUT = "Timeout";

    final static String PREFERED_INSTANCE_MODE_RANDON = "random";
    final static String PREFERED_INSTANCE_MODE_ORDERED = "ordered";
}
