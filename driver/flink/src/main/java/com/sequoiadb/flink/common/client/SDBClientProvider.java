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

package com.sequoiadb.flink.common.client;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.flink.config.SDBSinkOptions;

import java.io.Closeable;
import java.io.Serializable;
import java.util.List;

public interface SDBClientProvider extends Serializable, Closeable {

    CollectionSpace getCollectionSpace();

    DBCollection getCollection();


    class Builder {
        private List<String> hosts;
        private String collectionSpace;
        private String collection;
        private String username;
        private String password;
        private SDBSinkOptions sinkOptions;

        public Builder withHosts(List<String> hosts) {
            this.hosts = hosts;
            return this;
        }

        public Builder withCollectionSpace(String collectionSpace) {
            this.collectionSpace = collectionSpace;
            return this;
        }

        public Builder withCollection(String collection) {
            this.collection = collection;
            return this;
        }

        public Builder withUsername(String username) {
            this.username = username;
            return this;
        }

        public Builder withPassword(String password) {
            this.password = password;
            return this;
        }

        public Builder withOptions(SDBSinkOptions sinkOptions) {
            this.sinkOptions = sinkOptions;
            return this;
        }

        public SDBClientProvider build() {
            return new SDBCollectionProvider(
                    hosts,
                    collectionSpace,
                    collection,
                    username,
                    password,
                    sinkOptions);
        }
    }

    static Builder builder() {
        return new Builder();
    }

}
