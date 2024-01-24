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

package com.sequoiadb.base.options;

import org.bson.BSONObject;

/**
 * The optional parameter of the upsert operation.
 *
 * @since 3.4.5/5.0.3
 */
public class UpsertOption extends UpdateOption{
    private BSONObject setOnInsert = null;

    /**
     * Gets the setOnInsert option.
     *
     * @return A BSONObject of the setOnInsert option.
     */
    public BSONObject getSetOnInsert() {
        return setOnInsert;
    }

    /**
     * Sets the setOnInsert option, default is null.
     *
     * @param setOnInsert The setOnInsert assigns the specified values to the fields when insert
     */
    public void setSetOnInsert( BSONObject setOnInsert ) {
        this.setOnInsert = setOnInsert;
    }

    @Override
    public String toString() {
        return "UpsertOption{" +
                "setOnInsert=" + setOnInsert +
                '}';
    }

    @Override
    public Object clone() throws CloneNotSupportedException {
        return super.clone();
    }
}