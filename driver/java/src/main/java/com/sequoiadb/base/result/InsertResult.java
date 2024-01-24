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

package com.sequoiadb.base.result;

import com.sequoiadb.util.Helper;
import org.bson.BSONObject;

import java.util.List;
import java.util.Objects;

/**
 * The result of insert operation.
 *
 * @since 3.4.5/5.0.3
 */
public class InsertResult extends BaseResult {
    private final Object oid;
    private final long insertNum;
    private final long duplicatedNum;
    private final long lastGenerateID;
    private final long modifiedNum;
    private boolean isBulkInsert = false;

    public InsertResult( BSONObject obj ) {
        this.oid = Helper.getValue( obj, "_id", null );
        if ( oid instanceof List ){
            isBulkInsert = true;
        }
        this.insertNum = ( long ) Helper.getValue( obj, "InsertedNum",
                Helper.INIT_LONG );
        this.duplicatedNum = ( long ) Helper.getValue( obj, "DuplicatedNum",
                Helper.INIT_LONG );
        this.lastGenerateID = ( long ) Helper.getValue( obj, "LastGenerateID",
                Helper.INIT_LONG );
        this.modifiedNum = ( long ) Helper.getValue( obj, "ModifiedNum",
                Helper.INIT_LONG );
    }

    /**
     * Gets the ObjectID of records inserted.
     *
     * @return The object of oid
     */
    public Object getOid() {
        return isBulkInsert ? null : oid;
    }

    /**
     * Gets the list of ObjectID for bulk inserting records.
     *
     * @return The list of oid
     */
    public List< Object > getOidList() {
        return isBulkInsert ? ( List< Object > ) oid : null;
    }

    /**
     * Gets the number of records inserted.
     *
     * @return The number of records inserted
     */
    public long getInsertNum() {
        return insertNum;
    }

    /**
     * Gets the number of records with duplicate key conflicts.
     *
     * @return The number of records with duplicate key conflicts
     */
    public long getDuplicatedNum() {
        return duplicatedNum;
    }

    /**
     * Gets the value of the auto-increment field (only displayed when the
     * collection contains [auto-increment][auto-increment]), the return
     * situation is as follows:
     * <ul>
     * <li>When inserting a single record, return the auto-incremented field
     * value corresponding to the record.
     * <li>When inserting multiple records, only return the increment field
     * value corresponding to the first record.
     * <li>When there are multiple auto-increment fields, insert a single record
     * and only return the maximum value of all auto-increment fields.
     * <li>When there are multiple auto-increment fields, insert multiple
     * records and only return the largest auto-increment field value
     * corresponding to the first record.
     * </ul>
     *
     * @return The value of auto-increment field
     */
    public long getLastGenerateID() {
        return lastGenerateID;
    }

    /**
     * Get the number of records successfully updated with data changes.
     *
     * @return The number of records updated
     */
    public long getModifiedNum() {
        return modifiedNum;
    }

    @Override
    public boolean equals( Object o ) {
        if ( this == o )
            return true;
        if ( o == null || getClass() != o.getClass() )
            return false;
        InsertResult result = ( InsertResult ) o;
        return insertNum == result.insertNum
                && duplicatedNum == result.duplicatedNum
                && lastGenerateID == result.getLastGenerateID()
                && Objects.equals( oid, result.oid )
                && modifiedNum == result.getModifiedNum();
    }

    @Override
    public int hashCode() {
        return Objects.hash( oid, insertNum, duplicatedNum, lastGenerateID, modifiedNum );
    }

    @Override
    public String toString() {
        return "InsertResult{" +
                "oid=" + oid +
                ", insertNum=" + insertNum +
                ", duplicatedNum=" + duplicatedNum +
                ", lastGenerateID=" + lastGenerateID +
                ", modifiedNum=" + modifiedNum +
                '}';
    }
}
