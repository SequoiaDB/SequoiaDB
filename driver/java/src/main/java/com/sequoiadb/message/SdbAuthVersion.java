/*
 * Copyright 2022 SequoiaDB Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
 * in compliance with the License. You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the License
 * is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
 * or implied. See the License for the specific language governing permissions and limitations under
 * the License.
 */

package com.sequoiadb.message;

/**
 * Authenticate version of SequoiaDB.
 */
public enum SdbAuthVersion {
    SDB_AUTH_MD5(0),
    SDB_AUTH_SCRAM_SHA256(1);

    private final int version;

    SdbAuthVersion(int v) {
        this.version = v;
    }

    public int getVersion() {
        return this.version;
    }
}