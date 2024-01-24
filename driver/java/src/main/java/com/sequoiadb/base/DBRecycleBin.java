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
import org.bson.BasicBSONObject;
import org.bson.BSONObject;

/**
 * Recycle bin of SequoiaDB.
 */
public class DBRecycleBin {
    private Sequoiadb sequoiadb;

    /**
     * @param sequoiadb Sequoiadb instance
     */
    DBRecycleBin(Sequoiadb sequoiadb) {
        this.sequoiadb = sequoiadb;
    }

    /**
     * Get the list of recycle items in recycle bin items.
     *
     * @param query      The matching rule, match all the documents if null.
     * @param selector   The selective rule, return the whole document if null.
     * @param orderBy    The ordered rule, never sort if null.
     * @return The list of recycle items by cursor.
     * @throws BaseException If error happens.
     */
    public DBCursor list(BSONObject query,
                         BSONObject selector,
                         BSONObject orderBy)
    {
        return sequoiadb.getList(Sequoiadb.SDB_LIST_RECYCLEBIN,
                                 query, selector, orderBy);
    }

    /**
     * Get the list of recycle items in recycle bin items.
     *
     * @param query      The matching rule, match all the documents if null.
     * @param selector   The selective rule, return the whole document if null.
     * @param orderBy    The ordered rule, never sort if null.
     * @param hint       The options provided for specific list type. Reserved.
     * @param skipRows   skip the first numToSkip documents, never skip if this parameter is 0
     * @param returnRows return the specified amount of documents, when returnRows is 0, return nothing,
     *                   when returnRows is -1, return all the documents.
     * @return The list of recycle items by cursor.
     * @throws BaseException If error happens.
     */
    public DBCursor list(BSONObject query,
                         BSONObject selector,
                         BSONObject orderBy,
                         BSONObject hint,
                         long skipRows,
                         long returnRows)
    {
        return sequoiadb.getList(Sequoiadb.SDB_LIST_RECYCLEBIN,
                                 query, selector, orderBy,
                                 hint, skipRows, returnRows);
    }

    /**
     * Get the snapshot of recycle items.
     *
     * @param query      The matching rule, match all the documents if null.
     * @param selector   The selective rule, return the whole document if null.
     * @param orderBy    The ordered rule, never sort if null.
     * @return The snapshot of recycle items by cursor.
     * @throws BaseException If error happens.
     */
    public DBCursor snapshot(BSONObject query,
                             BSONObject selector,
                             BSONObject orderBy)
    {
        return sequoiadb.getSnapshot(Sequoiadb.SDB_LIST_RECYCLEBIN,
                                     query, selector, orderBy);
    }

    /**
     * Get the snapshot of recycle items.
     *
     * @param query      The matching rule, match all the documents if null.
     * @param selector   The selective rule, return the whole document if null.
     * @param orderBy    The ordered rule, never sort if null.
     * @param hint       The hint rule, the options provided for specific snapshot type format:{
     *                   '$Options': { <options> } }
     * @param skipRows   skip the first numToSkip documents, never skip if this parameter is 0
     * @param returnRows return the specified amount of documents, when returnRows is 0, return nothing,
     *                   when returnRows is -1, return all the documents.
     * @return The snapshot of recycle items by cursor.
     * @throws BaseException If error happens.
     */
    public DBCursor snapshot(BSONObject query,
                             BSONObject selector,
                             BSONObject orderBy,
                             BSONObject hint,
                             long skipRows,
                             long returnRows)
    {
        return sequoiadb.getSnapshot(Sequoiadb.SDB_LIST_RECYCLEBIN,
                                     query, selector, orderBy,
                                     hint, skipRows, returnRows);
    }

    /**
     * Alter options of the recycle bin.
     *
     * @param options   The options of recycle bin to be changed.
     *                  <ul>
     *                  <li>Enable : Indicates whether to enable the recycle bin.
     *                  <li>ExpireTime : Indicates the expired time of items in the recycle bin.
     *                  <li>MaxItemNum : Indicates the maximum number of items allowed in the recycle bin.
     *                  <li>MaxVersionNum : Indicates the maximum number of versions of the same item allowed in the recycle bin.
     *                  <li>AutoDrop : Indicates whether to drop old items automatically when number of items is up to the limit of MaxItemNum or MaxVersionNum.
     *                  </ul>
     * @throws BaseException If error happens.
     */
    public void setAttributes(BSONObject options) throws BaseException {
        if (options == null) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "options is null");
        }
        BSONObject argumentObj = new BasicBSONObject();
        argumentObj.put(SdbConstants.FIELD_NAME_ACTION,
                        SdbConstants.SDB_ALTER_SET_ATTRIBUTES);
        argumentObj.put(SdbConstants.FIELD_NAME_OPTIONS, options );
        AdminRequest request = new AdminRequest(AdminCommand.ALTER_RECYCLEBIN,
                                                argumentObj);
        SdbReply response = sequoiadb.requestAndResponse(request);
        sequoiadb.throwIfError(response, argumentObj);
    }

    /**
     * Alter options of the recycle bin.
     *
     * @param options   The options of recycle bin to be changed.
     *                  <ul>
     *                  <li>Enable : Indicates whether to enable the recycle bin.
     *                  <li>ExpireTime : Indicates the expired time of items in the recycle bin.
     *                  <li>MaxItemNum : Indicates the maximum number of items allowed in the recycle bin.
     *                  <li>MaxVersionNum : Indicates the maximum number of versions of the same item allowed in the recycle bin.
     *                  <li>AutoDrop : Indicates whether to drop old items automatically when number of items is up to the limit of MaxItemNum or MaxVersionNum.
     *                  </ul>
     * @throws BaseException If error happens.
     */
    public void alter(BSONObject options) throws BaseException {
        setAttributes(options);
    }

    /**
     * Enable the recycle bin.
     *
     * @throws BaseException If error happens.
     */
    public void enable() throws BaseException {
        BSONObject argumentObj = new BasicBSONObject();
        argumentObj.put(SdbConstants.FIELD_NAME_ACTION,
                        SdbConstants.SDB_ALTER_ENABLE_RECYCLEBIN);
        AdminRequest request = new AdminRequest(AdminCommand.ALTER_RECYCLEBIN,
                                                argumentObj);
        SdbReply response = sequoiadb.requestAndResponse(request);
        sequoiadb.throwIfError(response, argumentObj);
    }

    /**
     * Disable the recycle bin.
     *
     * @throws BaseException If error happens.
     */
    public void disable() throws BaseException {
        BSONObject argumentObj = new BasicBSONObject();
        argumentObj.put(SdbConstants.FIELD_NAME_ACTION,
                        SdbConstants.SDB_ALTER_DISABLE_RECYCLEBIN);
        AdminRequest request = new AdminRequest(AdminCommand.ALTER_RECYCLEBIN,
                                                argumentObj);
        SdbReply response = sequoiadb.requestAndResponse(request);
        sequoiadb.throwIfError(response, argumentObj);
    }

    /**
     * Get the detail of the recycle bin.
     *
     * @return The detail of recycle bin.
     * @throws BaseException If error happens.
     */
    public BSONObject getDetail() throws BaseException {
       BSONObject retObj;
       int flag = DBQuery.FLG_QUERY_WITH_RETURNDATA;
       flag |= DBQuery.FLG_QUERY_CLOSE_EOF_CTX;
       AdminRequest request = new AdminRequest(AdminCommand.GET_RECYCLEBIN_DETAIL, null, null,
               null, null, 0, -1, flag);
       SdbReply response = sequoiadb.requestAndResponse(request);
       sequoiadb.throwIfError(response);
       DBCursor cursor = new DBCursor(response, sequoiadb);
       try {
           if (cursor.hasNext()) {
               retObj = cursor.getNext();
               return retObj;
           } else {
               throw new BaseException(SDBError.SDB_SYS,
                                       "No detail for recycle bin");
           }
       } finally {
           cursor.close();
       }
    }

    /**
     * Get the count of matching recycle items in recycle bin.
     *
     * @param matcher   condition The matching rule, return the count of all documents if this parameter is empty.
     * @return The count of matching recycle items.
     * @throws BaseException If error happens.
     */
    public long getCount(BSONObject matcher) throws BaseException {
        int flag = DBQuery.FLG_QUERY_WITH_RETURNDATA;
        flag |= DBQuery.FLG_QUERY_CLOSE_EOF_CTX;
        AdminRequest request = new AdminRequest(AdminCommand.GET_RECYCLEBIN_COUNT, matcher, null,
                null, null, 0, -1, flag);
        SdbReply response = sequoiadb.requestAndResponse(request);
        sequoiadb.throwIfError(response, matcher);

        DBCursor cursor = new DBCursor(response, sequoiadb);
        BSONObject object;
        try {
            object = cursor.getNext();
        } finally {
            cursor.close();
        }
        return (Long) object.get(SdbConstants.FIELD_TOTAL);
    }

    /**
     * Return item from recycle bin.
     *
     * @param name    The name of item to be returned
     * @param options The options are as below:
     *                  <ul>
     *                  <li>Enforced : Whether to drop  the conflicting collection or collection space,
     *                  default is false.
     *                  </ul>
     * @return The return item information
     * @throws BaseException If error happens.
     */
    public BSONObject returnItem(String name, BSONObject options) throws BaseException {
        BSONObject retObj;
        if (name == null || name.isEmpty()) {
            throw new BaseException(
                            SDBError.SDB_INVALIDARG,
                            "name of recycle item can not be null or empty");
        }
        BSONObject rebuildOptions = new BasicBSONObject();
        rebuildOptions.put(SdbConstants.FIELD_NAME_RECYCLE_NAME, name);
        if (options != null) {
            rebuildOptions.putAll(options);
        }
        int flag = DBQuery.FLG_QUERY_WITH_RETURNDATA;
        flag |= DBQuery.FLG_QUERY_CLOSE_EOF_CTX;
        AdminRequest request = new AdminRequest(AdminCommand.RETURN_RECYCLEBIN_ITEM, rebuildOptions,
                null, null, null, 0, -1, flag);
        SdbReply response = sequoiadb.requestAndResponse(request);
        sequoiadb.throwIfError(response, rebuildOptions);
        DBCursor cursor = new DBCursor(response, sequoiadb);
        try {
            if (cursor.hasNext()) {
                retObj = cursor.getNext();
                return retObj;
            } else {
                throw new BaseException(SDBError.SDB_SYS,
                                        "No return result for recycle item");
            }
        } finally {
            cursor.close();
        }
    }

    /**
     * Return item to specified name from recycle bin.
     *
     * @param name         The name of item to be returned.
     * @param returnName   The name of collection or collection space to be returned to.
     * @param options      Reserved argument.
     * @return The return item information
     * @throws BaseException If error happens.
     */
    public BSONObject returnItemToName(String name,
                                       String returnName,
                                       BSONObject options) throws BaseException {
        BSONObject retObj;
        if (name == null || name.isEmpty()) {
            throw new BaseException(
                            SDBError.SDB_INVALIDARG,
                            "name of recycle item can not be null or empty");
        }
        if (returnName == null || returnName.isEmpty()) {
            throw new BaseException(
                            SDBError.SDB_INVALIDARG,
                            "return name of recycle item can not be null or empty");
        }
        BSONObject rebuildOptions = new BasicBSONObject();
        rebuildOptions.put(SdbConstants.FIELD_NAME_RECYCLE_NAME, name);
        rebuildOptions.put(SdbConstants.FIELD_NAME_RETURN_NAME, returnName);
        if (options != null) {
            rebuildOptions.putAll(options);
        }
        int flag = DBQuery.FLG_QUERY_WITH_RETURNDATA;
        flag |= DBQuery.FLG_QUERY_CLOSE_EOF_CTX;
        AdminRequest request = new AdminRequest(AdminCommand.RETURN_RECYCLEBIN_ITEM_TO_NAME,
                rebuildOptions, null, null, null, 0, -1, flag);
        SdbReply response = sequoiadb.requestAndResponse(request);
        sequoiadb.throwIfError(response, rebuildOptions);
        DBCursor cursor = new DBCursor(response, sequoiadb);
        try {
            if (cursor.hasNext()) {
                retObj = cursor.getNext();
                return retObj;
            } else {
                throw new BaseException(SDBError.SDB_SYS,
                                        "No return result for recycle item");
            }
        } finally {
            cursor.close();
        }
    }

    /**
     * Drop item from recycle bin permanently.
     *
     * @param name    The name of item to be cleared.
     * @param options The options are as below:
     *                  <ul>
     *                  <li>Async     : Asynchronous drop or not, default is false.
     *                  <li>Recursive : Whether to drop the recycle bin item of the collection type
     *                  associated with the collection space when dropping the recycle bin item of
     *                  the collection space type. The default is false.
     *                  </ul>
     * @throws BaseException If error happens.
     */
    public void dropItem(String name,
                         BSONObject options) throws BaseException {
        if (name == null || name.isEmpty()) {
            throw new BaseException(
                            SDBError.SDB_INVALIDARG,
                            "name of recycle item can not be null or empty");
        }
        BSONObject rebuildOptions = new BasicBSONObject();
        rebuildOptions.put(SdbConstants.FIELD_NAME_RECYCLE_NAME, name);
        if (options != null) {
            rebuildOptions.putAll(options);
        }
        AdminRequest request =
                        new AdminRequest(AdminCommand.DROP_RECYCLEBIN_ITEM,
                                         rebuildOptions);
        SdbReply response = sequoiadb.requestAndResponse(request);
        sequoiadb.throwIfError(response, rebuildOptions);
    }

    /**
     * Drop all items from recycle bin permanently.
     *
     * @param options The options are as below:
     *                  <ul>
     *                  <li>Async : Asynchronous drop or not, default is false.
     *                  </ul>
     * @throws BaseException If error happens.
     */
    public void dropAll(BSONObject options) throws BaseException {
        AdminRequest request =
                        new AdminRequest(AdminCommand.DROP_RECYCLEBIN_ALL,
                                         options);
        SdbReply response = sequoiadb.requestAndResponse(request);
        sequoiadb.throwIfError(response, options);
    }
}
