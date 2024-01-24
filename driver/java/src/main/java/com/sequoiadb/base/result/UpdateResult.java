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

import java.util.Objects;

/**
 * The result of update operation.
 *
 * @since 3.4.5/5.0.3
 */
public class UpdateResult extends BaseResult {
    private final long updatedNum;
    private final long insertNum;
    private final long modifiedNum;

    public UpdateResult( BSONObject obj ) {
        this.updatedNum = ( long ) Helper.getValue( obj, "UpdatedNum",
                Helper.INIT_LONG );
        this.insertNum = ( long ) Helper.getValue( obj, "InsertedNum",
                Helper.INIT_LONG );
        this.modifiedNum = ( long ) Helper.getValue( obj, "ModifiedNum",
                Helper.INIT_LONG );
    }

    /**
     * Gets the number of records updated.
     *
     * @return The number of records updated
     */
    public long getUpdatedNum() {
        return updatedNum;
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
     * Gets the number of records successfully updated with data changes.
     *
     * @return The number of records successfully updated with data changes
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
        UpdateResult that = ( UpdateResult ) o;
        return updatedNum == that.updatedNum && insertNum == that.insertNum
                && modifiedNum == that.modifiedNum;
    }

    @Override
    public int hashCode() {
        return Objects.hash( updatedNum, insertNum, modifiedNum );
    }

    @Override
    public String toString() {
        return "UpdateResult{" + "updatedNum=" + updatedNum + ", insertNum="
                + insertNum + ", modifiedNum=" + modifiedNum + '}';
    }
}
