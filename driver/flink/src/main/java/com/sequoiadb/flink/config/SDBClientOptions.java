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

import java.io.File;
import java.io.Serializable;
import java.util.Arrays;
import java.util.List;

import com.sequoiadb.flink.common.exception.SDBException;
import com.sequoiadb.util.SdbDecrypt;
import com.sequoiadb.util.SdbDecryptUserInfo;

import org.apache.flink.configuration.ReadableConfig;

public class SDBClientOptions implements Serializable {

    private final List<String> hosts;
    private final String collectionSpace;
    private final String collection;
    private final String username;
    private final String password;

    private String sourceInfo;

    public SDBClientOptions(ReadableConfig options) {
        this.hosts = Arrays.asList(options.get(SDBConfigOptions.HOSTS)
                .split(","));
        this.collectionSpace = options.get(SDBConfigOptions.COLLECTION_SPACE);
        this.collection = options.get(SDBConfigOptions.COLLECTION);
        this.username = options.get(SDBConfigOptions.USERNAME);

        String token = options.get(SDBConfigOptions.TOKEN);
        String passwordType = options.get(SDBConfigOptions.PASSWORD_TYPE);
        String password = options.get(SDBConfigOptions.PASSWORD);

        if ("file".equals(passwordType)) {
            SdbDecrypt sdbDecrypt = new SdbDecrypt();
            SdbDecryptUserInfo userInfo = sdbDecrypt.parseCipherFile(username, token, new File(password));
            this.password = userInfo.getPasswd();
        } else if ("cleartext".equals(passwordType)) {
            this.password = options.get(SDBConfigOptions.PASSWORD);
        } else {
            throw new SDBException(String.format("password type %s is not in ['cleartext', 'file'].", passwordType));
        }
    }

    public List<String> getHosts() {
        return hosts;
    }

    public String getCollectionSpace() {
        return collectionSpace;
    }

    public String getCollection() {
        return collection;
    }

    public String getUsername() {
        return username;
    }

    public String getPassword() {
        return password;
    }

    public void setSourceInfo(String sourceInfo) {
        this.sourceInfo = sourceInfo;
    }

    public String getSourceInfo() {
        return sourceInfo;
    }

}
