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
 * The result of delete operation.
 *
 * @since 3.4.5/5.0.3
 */
public class DeleteResult extends BaseResult {
    private final long deletedNum;

    public DeleteResult( BSONObject obj ) {
        this.deletedNum = ( long ) Helper.getValue( obj, "DeletedNum",
                Helper.INIT_LONG );
    }

    /**
     * Gets the number of records deleted.
     *
     * @return The number of records deleted
     */
    public long getDeletedNum() {
        return deletedNum;
    }

    @Override
    public boolean equals( Object o ) {
        if ( this == o )
            return true;
        if ( o == null || getClass() != o.getClass() )
            return false;
        DeleteResult that = ( DeleteResult ) o;
        return Objects.equals( deletedNum, that.deletedNum );
    }

    @Override
    public int hashCode() {
        return Objects.hash( deletedNum );
    }

    @Override
    public String toString() {
        return "DeleteResult{" + "deletedNum=" + deletedNum + '}';
    }
}
