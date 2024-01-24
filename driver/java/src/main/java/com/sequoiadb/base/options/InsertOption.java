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

/**
 * The optional parameter of the insert operation.
 *
 * @since 3.4.5/5.0.3
 */
public class InsertOption extends BaseOption {
    /**
     * The flag represent whether insert continue(no errors were reported) when hitting index key
     * duplicate error
     */
    public final static int FLG_INSERT_CONTONDUP = 0x00000001;

    /**
     * The flag represent whether insert return the "_id" field of the record for user
     */
    public final static int FLG_INSERT_RETURN_OID = 0x10000000;

    /**
     * The flag represent whether insert becomes update when hitting index key duplicate error.
     */
    public final static int FLG_INSERT_REPLACEONDUP = 0x00000004;

    /** 
     * The flag represent the error of the dup key will be ignored when the dup key is '_id'.
     */
    public final static int FLG_INSERT_CONTONDUP_ID = 0x00000020;
    
    /**
     * The flag represents the error of the dup key will be ignored when the dup key is '_id',
     * and the original record will be replaced by new record.
     */
    public final static int FLG_INSERT_REPLACEONDUP_ID = 0x00000040;

    private int flag = 0;

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
     * @param flag The insert flag, default to be 0, the following values:
     *             <ul>
     *                 <li>{@link InsertOption#FLG_INSERT_CONTONDUP}
     *                 <li>{@link InsertOption#FLG_INSERT_RETURN_OID}
     *                 <li>{@link InsertOption#FLG_INSERT_REPLACEONDUP}
     *                 <li>{@link InsertOption#FLG_INSERT_CONTONDUP_ID}
     *                 <li>{@link InsertOption#FLG_INSERT_REPLACEONDUP_ID}
     *             </ul>
     * @return this
     */
    public InsertOption setFlag( int flag ) {
        this.flag = flag;
        return this;
    }

    /**
     * Appends a new flag to the insert option.
     *
     * @param flag The insert flag, the following values:
     *             <ul>
     *                 <li>{@link InsertOption#FLG_INSERT_CONTONDUP}
     *                 <li>{@link InsertOption#FLG_INSERT_RETURN_OID}
     *                 <li>{@link InsertOption#FLG_INSERT_REPLACEONDUP}
     *                 <li>{@link InsertOption#FLG_INSERT_CONTONDUP_ID}
     *                 <li>{@link InsertOption#FLG_INSERT_REPLACEONDUP_ID}
     *             </ul>
     */
    public void appendFlag( int flag ){
        this.flag |= flag;
    }

    /**
     * Erases the specified flag from the insert option.
     *
     * @param flag The insert flag, the following values:
     *             <ul>
     *                 <li>{@link InsertOption#FLG_INSERT_CONTONDUP}
     *                 <li>{@link InsertOption#FLG_INSERT_RETURN_OID}
     *                 <li>{@link InsertOption#FLG_INSERT_REPLACEONDUP}
     *                 <li>{@link InsertOption#FLG_INSERT_CONTONDUP_ID}
     *                 <li>{@link InsertOption#FLG_INSERT_REPLACEONDUP_ID}
     *             </ul>
     */
    public void eraseFlag( int flag ){
        this.flag = Helper.eraseFlag( this.flag, flag );
    }

    @Override
    public String toString() {
        return "InsertOption{" +
                "flag=" + flag +
                '}';
    }

    @Override
    public Object clone() throws CloneNotSupportedException {
        return super.clone();
    }
}
