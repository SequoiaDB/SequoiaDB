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
import com.sequoiadb.message.request.AdminRequest;
import com.sequoiadb.message.response.SdbReply;
import com.sequoiadb.util.Helper;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;

/**
 * Data source of SequoiaDB.
 */
public class DataSource {
    private String name;
    private Sequoiadb sequoiadb;

    DataSource(Sequoiadb sequoiadb, String name) {
        this.name = name;
        this.sequoiadb = sequoiadb;
    }

    /**
     * Get the data source name.
     *
     * @return The name of current data source
     */
    public String getName() {
        return this.name;
    }

    /**
     * Modify the attributes of the current data source.
     *
     * @param option The options for setting data source attributes, as follows:
     *                <ul>
     *                <li>Name(String) : New name of current data source
     *                <li>Address(String) : The list of coord addresses for the target sequoiadb cluster,
     *                spearated by ','
     *                <li>User(String) : User name of data source
     *                <li>Password(String) : Data source password corresponding to User
     *                <ul>
     *                  <li>AccessMode(String) : Configure access permissions for the data source, default is "ALL",
     *                  the values are as follows:
     *                  <ul>
     *                    <li>"READ" : Allow read-only operation
     *                    <li>"WRITE" : Allow write-only operation
     *                    <li>"ALL" or "READ|WRITE" : Allow all operations
     *                    <li>"NONE" : Neither read nor write operation is allowed
     *                  </ul>
     *                  <li>ErrorFilterMask(String) : Configure error filtering for data operations on data sources,
     *                  default is "NONE", the values are as follows:
     *                  <ul>
     *                    <li>"READ" : Filter data read errors
     *                    <li>"WRITE" : Filter data write errors
     *                    <li>"ALL" or "READ|WRITE" : Filter both data read and write errors
     *                    <li>"NONE" : Do not filter any errors
     *                  </ul>
     *                  <li>ErrorControlLevel(String) : Configure the error control level when performing unsupported data
     *                  operations(such as DDL) on the mapping collection or collection space, default is "low",
     *                  the values are as follows:
     *                  <ul>
     *                    <li>"high" : Report an error and output an error message
     *                    <li>"low" : Ignore unsupported data operations and do not execute on data source
     *                  </ul>
     *                  <li>TransPropagateMode(String) : Configure the transaction propagation mode on data source,
     *                  default is "never", the values are as follows:
     *                  <ul>
     *                    <li>"never": Transaction operation is forbidden. Report an error and output and error message.
     *                    <li>"notsupport": Transaction operation is not supported on data source. The operation
     *                    will be converted to non-transactional and send to data source.
     *                  </ul>
     *                  <li>InheritSessionAttr(Bool): Configure whether the session between the coordination node and the
     *                  data source inherits properties of the local session, default is true. The supported attributes
                        include PreferedInstance, PreferedInstanceMode, PreferedStrict, PreferedPeriod and Timeout.
     *                </ul>
     * @throws BaseException If error happens.
     */
    public void alterDataSource(BSONObject option) throws BaseException {
        if (option == null || option.isEmpty()) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "option can not be null or empty");
        }

        String newDSName = null;
        BSONObject obj = new BasicBSONObject();
        for (String key: option.keySet()) {
            // save the new data source name
            if (key.equals(SdbConstants.FIELD_NAME_NAME)){
                newDSName = (String)option.get(key);
            }
            // password need to be encrypted
            if (key.equals(SdbConstants.FIELD_NAME_PASSWD)){
                String pwd = Helper.md5((String)option.get(key));
                obj.put(key, pwd);
            }else {
                obj.put(key, option.get(key));
            }
        }

        BSONObject matcher = new BasicBSONObject();
        matcher.put(SdbConstants.FIELD_NAME_NAME, name);
        matcher.put(SdbConstants.FIELD_NAME_OPTIONS, obj);

        AdminRequest request = new AdminRequest(AdminCommand.ALTER_DATASOURCE, matcher);
        SdbReply response = this.sequoiadb.requestAndResponse(request);
        this.sequoiadb.throwIfError(response);

        // update the data source name
        if (newDSName != null){
            this.name = newDSName;
        }
    }
}