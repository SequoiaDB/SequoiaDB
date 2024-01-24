/*
 * Copyright 2018 SequoiaDB Inc.
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

package com.sequoiadb.base;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.message.ResultSet;
import com.sequoiadb.message.request.AdminRequest;
import com.sequoiadb.message.request.SequenceFetchRequest;
import com.sequoiadb.message.response.SdbReply;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;

/**
 * Sequence of SequoiaDB
 */
public class DBSequence {

    private String name;
    private Sequoiadb sdb;

    DBSequence(String name, Sequoiadb sdb){
        this.name = name;
        this.sdb = sdb;
    }

    /**
     * @return The name of current sequence
     */
    public String getName(){
        return this.name;
    }

    /**
     * Fetch a bulk of continuous values.
     *
     * @param fetchNum The number of values to be fetched
     * @return A bson object that contains the following fields:
     *           <ul>
     *             <li>NextValue(long) : The next value and also the first returned value.
     *             <li>ReturnNum(int)  : The number of values returned.
     *             <li>Increment(int)  : Increment of values.
     *           </ul>
     */
    public BSONObject fetch(int fetchNum){
        if (fetchNum < 1){
            throw new BaseException(SDBError.SDB_INVALIDARG, "fetchNum cannot be less than 1");
        }
        BSONObject matcher = new BasicBSONObject();
        matcher.put(SdbConstants.FIELD_NAME_NAME, this.name);
        matcher.put(SdbConstants.FIELD_NAME_FETCH_NUM, fetchNum);

        SequenceFetchRequest request = new SequenceFetchRequest(matcher);
        SdbReply response = sdb.requestAndResponse(request);
        sdb.throwIfError(response);
        ResultSet resultSet = response.getResultSet();
        BSONObject result = null;
        if (resultSet != null){
            result = resultSet.getNext();
        }
        return result;
    }

    /**
     * Get the current value of sequence.
     *
     * @return The current value
     */
    public long getCurrentValue(){
        BSONObject matcher = new BasicBSONObject(SdbConstants.FIELD_NAME_NAME, this.name);
        AdminRequest request = new AdminRequest(AdminCommand.GET_SEQ_CURR_VAL,matcher);
        SdbReply response = sdb.requestAndResponse(request);
        sdb.throwIfError(response);
        ResultSet resultSet = response.getResultSet();
        if (resultSet != null){
            BSONObject obj = resultSet.getNext();
            if (obj != null){
                Object currentValue = obj.get(SdbConstants.FIELD_NAME_CURRENT_VALUE);
                if (currentValue != null){
                    return (long)currentValue;
                }
            }
        }
        throw new BaseException(SDBError.SDB_NET_BROKEN_MSG, "Failed to get "+ SdbConstants.FIELD_NAME_CURRENT_VALUE +
                               " from response message");
    }

    /**
     * Get the next value of sequence.
     *
     * @return The next value
     */
    public long getNextValue(){
        BSONObject obj = fetch(1);
        String targetKey = "NextValue";
        if (obj != null){
            Object value = obj.get(targetKey);
            if (value != null){
                return (long)value;
            }
        }
        throw new BaseException(SDBError.SDB_NET_BROKEN_MSG,"Failed to get " + targetKey + " from response message");
    }

    /**
     * Restart sequence from the given value.
     *
     * @param startValue The start value.
     */
    public void restart(long startValue){
        BSONObject obj = new BasicBSONObject(SdbConstants.FIELD_NAME_START_VALUE, startValue);
        _alterInternal(SdbConstants.SEQ_OPT_RESTART, obj);
    }

    /**
     * Alter the sequence with the specified options.
     *
     * @param options The options specified by user, details as bellow:
     *                <ul>
     *                  <li>CurrentValue(long) : The current value of sequence
     *                  <li>StartValue(long)   : The start value of sequence
     *                  <li>MinValue(long)     : The minimum value of sequence
     *                  <li>MaxValue(long)     : The maxmun value of sequence
     *                  <li>Increment(int)     : The increment value of sequence
     *                  <li>CacheSize(int)     : The cache size of sequence
     *                  <li>AcquireSize(int)   : The acquire size of sequence
     *                  <li>Cycled(boolean)    : The cycled flag of sequence
     *                </ul>
     */
    public void setAttributes(BSONObject options){
        _alterInternal(SdbConstants.SEQ_OPT_SETATTR, options);
    }

    /**
     * Set the current value to sequence.
     *
     * @param value The expected current value
     */
    public void setCurrentValue(long value){
        BSONObject obj = new BasicBSONObject(SdbConstants.FIELD_NAME_EXPECT_VALUE, value);
        _alterInternal(SdbConstants.SEQ_OPT_SET_CURR_VALUE, obj);
    }

    private void _alterInternal(String actionName, BSONObject options){
        if (options != null && options.get(SdbConstants.FIELD_NAME_NAME) != null){
            throw new BaseException(SDBError.SDB_INVALIDARG);
        }
        BSONObject obj = new BasicBSONObject();
        BSONObject subObj = new BasicBSONObject();

        subObj.put(SdbConstants.FIELD_NAME_NAME, this.name);
        subObj.putAll(options);

        obj.put(SdbConstants.FIELD_NAME_ACTION, actionName);
        obj.put(SdbConstants.FIELD_NAME_OPTIONS, subObj);

        AdminRequest request = new AdminRequest(AdminCommand.ALTER_SEQUENCE, obj);
        SdbReply response = sdb.requestAndResponse(request);
        sdb.throwIfError(response);
    }

}
