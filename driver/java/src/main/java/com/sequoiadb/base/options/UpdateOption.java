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

import com.sequoiadb.util.Helper;
import org.bson.BSONObject;

/**
 * The optional parameter of the update operation.
 *
 * @since 3.4.5/5.0.3
 */
public class UpdateOption extends BaseOption {
    /**
     * The flag represent whether to update only one matched record or all matched records.
     */
    public final static int FLG_UPDATE_ONE = 0x00000002;

    /**
     * The sharding key in update rule is not filtered, when executing update or upsert.
     */
    public final static int FLG_UPDATE_KEEP_SHARDINGKEY = 0x00008000;

    private BSONObject hint = null;
    private int flag = 0;

    /**
     * Gets the hint option.
     *
     * @return A BSONObject of the hint option.
     */
    public BSONObject getHint() {
        return hint;
    }

    /**
     * Sets the hint option, default is null.
     *
     * @param hint Specified the index used to scan data. e.g. {"":"ageIndex"} means using index
     *             "ageIndex" to scan data(index scan); {"":null} means table scan. when hint is
     *             null, database automatically match the optimal index to scan data.
     * @return this
     */
    public UpdateOption setHint( BSONObject hint ) {
        this.hint = hint;
        return this;
    }

    /**
     * Gets the flag option.
     *
     * @return The value of flag option.
     */
    public int getFlag() {
        return flag;
    }

    /**
     * Sets the flag option.
     *
     * @param flag The update flag, default to be 0, the following values:
     *             <ul>
     *                 <li>{@link UpdateOption#FLG_UPDATE_KEEP_SHARDINGKEY}
     *                 <li>{@link UpdateOption#FLG_UPDATE_ONE}
     *             </ul>
     * @return this
     */
    public UpdateOption setFlag( int flag ) {
        this.flag = flag;
        return this;
    }

    /**
     * Appends a new flag to the update option.
     *
     * @param flag The update flag, the following values:
     *             <ul>
     *                 <li>{@link UpdateOption#FLG_UPDATE_KEEP_SHARDINGKEY}
     *                 <li>{@link UpdateOption#FLG_UPDATE_ONE}
     *             </ul>
     */
    public void appendFlag( int flag ){
        this.flag |= flag;
    }

    /**
     * Erases the specified flag from the update option.
     *
     * @param flag The update flag, the following values:
     *             <ul>
     *                 <li>{@link UpdateOption#FLG_UPDATE_KEEP_SHARDINGKEY}
     *                 <li>{@link UpdateOption#FLG_UPDATE_ONE}
     *             </ul>
     */
    public void eraseFlag( int flag ){
        this.flag = Helper.eraseFlag( this.flag, flag );
    }

    @Override
    public String toString() {
        return "UpdateOption{" +
                "hint=" + hint +
                ", flag=" + flag +
                '}';
    }

    @Override
    public Object clone() throws CloneNotSupportedException {
        return super.clone();
    }
}
