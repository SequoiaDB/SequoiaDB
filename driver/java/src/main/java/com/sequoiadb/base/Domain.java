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
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;

/**
 * Domain of SequoiaDB.
 */
public class Domain {
    private String name;
    private Sequoiadb sequoiadb;

    /**
     * @return The name of current domain
     */
    public String getName() {
        return name;
    }

    /**
     * @return Sequoiadb The Sequoiadb instance of current domain belongs to.
     */
    public Sequoiadb getSequoiadb() {
        return sequoiadb;
    }

    /**
     * @param sequoiadb Sequoiadb connection instance
     * @param name      the name for the created domain
     */
    Domain(Sequoiadb sequoiadb, String name) {
        this.name = name;
        this.sequoiadb = sequoiadb;
    }

    private void alterInternal(String taskName, BSONObject options, boolean allowNullArgs) throws BaseException {
        if (null == options && !allowNullArgs) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "options is null");
        }
        BSONObject argumentObj = new BasicBSONObject();
        argumentObj.put(SdbConstants.FIELD_NAME_NAME, taskName);
        argumentObj.put(SdbConstants.FIELD_NAME_ARGS, options);
        BSONObject alterObject = new BasicBSONObject();
        alterObject.put(SdbConstants.FIELD_NAME_ALTER, argumentObj);
        alterDomain(alterObject);
    }

    /**
     * Alter current domain.
     *
     * @param options the options user wants to alter:
     *                <ul>
     *                <li>Groups:    The list of replica groups' names which the domain is going to contain.
     *                eg: { "Groups": [ "group1", "group2", "group3" ] }, it means that domain
     *                changes to contain "group1", "group2" and "group3".
     *                We can add or remove groups in current domain. However, if a group has data
     *                in it, remove it out of domain will be failing.
     *                <li>AutoSplit: Alter current domain to have the ability of automatically split or not.
     *                If this option is set to be true, while creating collection(ShardingType is "hash") in this domain,
     *                the data of this collection will be split(hash split) into all the groups in this domain automatically.
     *                However, it won't automatically split data into those groups which were add into this domain later.
     *                eg: { "Groups": [ "group1", "group2", "group3" ], "AutoSplit: true" }
     *                </ul>
     * @throws BaseException If error happens.
     */
    public void alterDomain(BSONObject options) throws BaseException {
        if (null == options) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "options is null");
        }

        BSONObject newObj = new BasicBSONObject();
        if (!options.containsField(SdbConstants.FIELD_NAME_ALTER)) {
            newObj.put(SdbConstants.FIELD_NAME_NAME, this.name);
            newObj.put(SdbConstants.FIELD_NAME_OPTIONS, options);
        } else {
            Object tmpAlter = options.get(SdbConstants.FIELD_NAME_ALTER);
            if (tmpAlter instanceof BasicBSONObject ||
                    tmpAlter instanceof BasicBSONList) {
                newObj.put(SdbConstants.FIELD_NAME_ALTER, tmpAlter);
            } else {
                throw new BaseException(SDBError.SDB_INVALIDARG, options.toString());
            }
            newObj.put(SdbConstants.FIELD_NAME_ALTER_TYPE, SdbConstants.SDB_ALTER_DOMAIN);
            newObj.put(SdbConstants.FIELD_NAME_VERSION, SdbConstants.SDB_ALTER_VERSION);
            newObj.put(SdbConstants.FIELD_NAME_NAME, name);

            if (options.containsField(SdbConstants.FIELD_NAME_OPTIONS)) {
                Object tmpOptions = options.get(SdbConstants.FIELD_NAME_OPTIONS);
                if (tmpOptions instanceof BasicBSONObject) {
                    newObj.put(SdbConstants.FIELD_NAME_OPTIONS, tmpOptions);
                } else {
                    throw new BaseException(SDBError.SDB_INVALIDARG, options.toString());
                }
            }
        }

        AdminRequest request = new AdminRequest(AdminCommand.ALTER_DOMAIN, newObj);
        SdbReply response = sequoiadb.requestAndResponse(request);
        sequoiadb.throwIfError(response);
    }

    /**
     * Alter current domain to add groups
     *
     * @param options the options user wants to alter:
     *                <ul>
     *                <li>Groups:    The list of replica groups' names which the domain is going to add.
     *                </ul>
     * @throws BaseException If error happens.
     */
    public void addGroups(BSONObject options) throws BaseException {
        alterInternal(SdbConstants.SDB_ALTER_ADD_GROUPS, options, false);
    }

    /**
     * Alter current domain to set groups
     *
     * @param options the options user wants to alter:
     *                <ul>
     *                <li>Groups:    The list of replica groups' names which the domain is going to contain.
     *                </ul>
     * @throws BaseException If error happens.
     */
    public void setGroups(BSONObject options) throws BaseException {
        alterInternal(SdbConstants.SDB_ALTER_SET_GROUPS, options, false);
    }

    /**
     * Alter current domain to remove groups
     *
     * @param options the options user wants to alter:
     *                <ul>
     *                <li>Groups:    The list of replica groups' names which the domain is going to remove.
     *                If a group has data in it, remove it out of domain will be failing.
     *                </ul>
     * @throws BaseException If error happens.
     */
    public void removeGroups(BSONObject options) throws BaseException {
        alterInternal(SdbConstants.SDB_ALTER_REMOVE_GROUPS, options, false);
    }

    /**
     * Alter current domain to set attributes.
     *
     * @param options the options user wants to alter:
     *                <ul>
     *                <li>Groups:    The list of replica groups' names which the domain is going to contain.
     *                eg: { "Groups": [ "group1", "group2", "group3" ] }, it means that domain
     *                changes to contain "group1", "group2" and "group3".
     *                We can add or remove groups in current domain. However, if a group has data
     *                in it, remove it out of domain will be failing.
     *                <li>AutoSplit: Alter current domain to have the ability of automatically split or not.
     *                If this option is set to be true, while creating collection(ShardingType is "hash") in this domain,
     *                the data of this collection will be split(hash split) into all the groups in this domain automatically.
     *                However, it won't automatically split data into those groups which were add into this domain later.
     *                eg: { "Groups": [ "group1", "group2", "group3" ], "AutoSplit: true" }
     *                </ul>
     * @throws BaseException If error happens.
     */
    public void setAttributes(BSONObject options) throws BaseException {
        alterInternal(SdbConstants.SDB_ALTER_SET_ATTRIBUTES, options, false);
    }

    /**
     * Alter replica groups in domain to set location.
     *
     * @param hostName   The host name in data group, if the host name in replica group that match
     *                   the specified host name, it will set location info.
     *                   However, if a group which don't match the host name, do nothing.
     * @param location   The location in data group, if the parameter is "",
     *                   it means to remove the domain location
     * @throws BaseException If error happens.
     * @since 3.6.1
     */
    public void setLocation(String hostName, String location) throws BaseException {
        if (hostName == null) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "The host name is null");
        }

        if (location == null) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "The location name is null");
        }

        BSONObject options = new BasicBSONObject();
        options.put(SdbConstants.FIELD_NAME_HOST, hostName);
        options.put(SdbConstants.NODE_LOCATION, location);
        alterInternal(SdbConstants.SDB_ALTER_SET_LOCATION, options, false);
    }

    /**
     * List all the collection spaces in current domain.
     *
     * @return the cursor of result
     * @throws BaseException If error happens.
     */
    public DBCursor listCSInDomain() throws BaseException {
        return listCSCL(Sequoiadb.SDB_LIST_CS_IN_DOMAIN);
    }

    /**
     * List all the collections in current domain.
     *
     * @return the cursor of result
     * @throws BaseException If error happens.
     */
    public DBCursor listCLInDomain() throws BaseException {
        return listCSCL(Sequoiadb.SDB_LIST_CL_IN_DOMAIN);
    }

    private DBCursor listCSCL(int type) throws BaseException {
        // append argument
        BSONObject matcher = new BasicBSONObject();
        BSONObject selector = new BasicBSONObject();
        matcher.put(SdbConstants.FIELD_NAME_DOMAIN, this.name);
        selector.put(SdbConstants.FIELD_NAME_NAME, null);
        // get cs or cl in current domain
        return this.sequoiadb.getList(type, matcher, selector, null);
    }

}
