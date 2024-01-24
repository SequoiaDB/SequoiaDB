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

import java.util.ArrayList;
import java.util.List;

/**
 * Collection space of SequoiaDB.
 */
public class CollectionSpace {
    private String name;
    private Sequoiadb sequoiadb;

    private static final String FIELD_NAME_DOMAIN = "Domain";

    /**
     * Return the name of current collection space.
     *
     * @return The collection space name
     */
    public String getName() {
        return name;
    }

    /**
     * Get the Sequoiadb instance of current collection space belongs to.
     *
     * @return Sequoiadb object
     */
    public Sequoiadb getSequoiadb() {
        return sequoiadb;
    }

    /**
     * @param sequoiadb Sequoiadb instance
     * @param name      Collection space name
     */
    CollectionSpace(Sequoiadb sequoiadb, String name) {
        this.name = name;
        this.sequoiadb = sequoiadb;
    }

    /**
     * Get the named collection.
     *
     * @param collectionName The collection name
     * @return the object of the specified collection, or an exception when the collection does not exist.
     * @throws BaseException If error happens.
     */
    public DBCollection getCollection(String collectionName) throws BaseException {
        if (collectionName == null || collectionName.equals("")) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "collectionName can't be null or empty");
        }
        // get cl from cache
        String collectionFullName = name + "." + collectionName;
        if (sequoiadb.fetchCache(collectionFullName)) {
            return new DBCollection(sequoiadb, this, collectionName);
        }
        // create cl and then update cache
        BSONObject obj = new BasicBSONObject();
        obj.put(SdbConstants.FIELD_NAME_NAME, collectionFullName);
        AdminRequest request = new AdminRequest(AdminCommand.TEST_CL, obj);
        SdbReply response = sequoiadb.requestAndResponse(request);
        sequoiadb.throwIfError(response);
        sequoiadb.upsertCache(collectionFullName);
        return new DBCollection(sequoiadb, this, collectionName);
    }

    /**
     * Verify the existence of collection in current collection space.
     *
     * @param collectionName The collection name
     * @return True if collection existed or False if not existed
     * @throws BaseException If error happens.
     */
    public boolean isCollectionExist(String collectionName) throws BaseException {
        if (collectionName == null || collectionName.equals("")) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "collectionName can't be null or empty");
        }
        String collectionFullName = name + "." + collectionName;

        BSONObject obj = new BasicBSONObject();
        obj.put(SdbConstants.FIELD_NAME_NAME, collectionFullName);

        AdminRequest request = new AdminRequest(AdminCommand.TEST_CL, obj);
        SdbReply response = sequoiadb.requestAndResponse(request);

        int flag = response.getFlag();
        if (flag == 0) {
            sequoiadb.upsertCache(collectionFullName);
            return true;
        } else if (flag == SDBError.SDB_DMS_NOTEXIST.getErrorCode()) {
            sequoiadb.removeCache(collectionFullName);
            return false;
        } else {
            sequoiadb.throwIfError(response);
            return false; // make compiler happy
        }
    }

    /**
     * Get all the collection names of current collection space.
     *
     * @return A list of collection names
     * @throws BaseException If error happens.
     */
    public List<String> getCollectionNames() throws BaseException {
        ArrayList<String> colList = new ArrayList<>();

        BSONObject condition = new BasicBSONObject();
        BSONObject subObj = new BasicBSONObject();

        subObj.put("$gt", name + ".");
        subObj.put("$lt", name + "/");

        condition.put(SdbConstants.FIELD_NAME_NAME, subObj);

        try(DBCursor cursor = sequoiadb.getList(Sequoiadb.SDB_LIST_COLLECTIONS, condition, null, null)) {
            while (cursor.hasNext()) {
                colList.add(cursor.getNext().get("Name").toString());
            }
        }
        return colList;
    }

    /**
     * Create the named collection in current collection space.
     *
     * @param collectionName The collection name
     * @return the newly created object of collection
     * @throws BaseException If error happens.
     */
    public DBCollection createCollection(String collectionName)
            throws BaseException {
        return createCollection(collectionName, null);
    }

    /**
     * Create collection by options.
     *
     * @param collectionName The collection name
     * @param options The options are as following:
     *               <ul>
     *               <li>ShardingKey : Assign the sharding key, foramt: { ShardingKey: { <key name>: <1/-1>} }, 
     *               1 indicates positive order, -1 indicates reverse order. e.g. { ShardingKey: { age: 1 } }
     *               <li>ShardingType : Assign the sharding type, default is "hash"
     *               <li>Partition : The number of partition, it is valid when ShardingType is "hash",
     *               the range is [2^3, 2^20], default is 4096
     *               <li>ReplSize : Assign how many replica nodes need to be synchronized when a write
     *               request (insert, update, etc) is executed, default is 1
     *               <li>Compressed : Whether to enable data compression, default is true
     *               <li>CompressionType : The compression type of data, could be "snappy" or "lzw", default is "lzw"
     *               <li>AutoSplit : Whether to enable the automatic partitioning, it is valid when ShardingType
     *               is "hash", defalut is false
     *               <li>Group: Assign the data group to which it belongs, default: The collection will
     *               be created in any data group of the domain that the collection belongs to
     *               <li>AutoIndexId : Whether to build "$id" index, default is true
     *               <li>EnsureShardingIndex : Whether to build sharding index, default is true
     *               <li>StrictDataMode : Whether to enable strict date mode in numeric operations, default is false
     *               <li>AutoIncrement : Assign attributes of an autoincrement field or batch autoincrement fields
     *               e.g. { AutoIncrement : { Field : "a", MaxValue : 2000 } },
     *               { AutoIncrement : [ { Field : "a", MaxValue : 2000}, { Field : "a", MaxValue : 4000 } ] }
     *               <li>LobShardingKeyFormat : Assign the format of lob sharding key, could be "YYYYMMDD", "YYYYMM" or "YYYY".
     *               It is valid when the collection is main collection
     *               <li>IsMainCL : Main collection or not, default is false, which means it is not main collection
     *               <li>DataSource : The name of the date soure used
     *               <li>Mapping : The name of the collection to be mapped
     * @return the newly created object of collection.
     * @throws BaseException If error happens.
     */
    public DBCollection createCollection(String collectionName, BSONObject options) {
        if (collectionName == null || collectionName.equals("")) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "collectionName can't be null or empty");
        }
        String collectionFullName = name + "." + collectionName;
        BSONObject obj = new BasicBSONObject();
        obj.put(SdbConstants.FIELD_NAME_NAME, collectionFullName);
        if (options != null) {
            obj.putAll(options);
        }

        AdminRequest request = new AdminRequest(AdminCommand.CREATE_CL, obj);
        SdbReply response = sequoiadb.requestAndResponse(request);
        String msg = "collection = " + collectionFullName + ", options = " + options;
        sequoiadb.throwIfError(response, msg);
        sequoiadb.upsertCache(collectionFullName);
        return new DBCollection(sequoiadb, this, collectionName);
    }

    /**
     * Drop current collection space.
     *
     * @throws BaseException If error happens.
     * @see com.sequoiadb.base.Sequoiadb#dropCollectionSpace(String)
     * @deprecated Use Sequoiadb.dropCollectionSpace() instead.
     */
    public void drop() throws BaseException {
        sequoiadb.dropCollectionSpace(this.name);
    }

    /**
     * Remove the named collection of current collection space.
     *
     * @param collectionName The collection name
     * @throws BaseException If error happens.
     */
    public void dropCollection(String collectionName) throws BaseException {
        dropCollection(collectionName, null);
    }

    /**
     * Remove the named collection of current collection space.
     *
     * @param collectionName The collection name
     * @param options The options for drop specified collection
     *                <ul>
     *                <li>SkipRecycleBin : Indicates whether to skip recycle bin, default is false.
     *                </ul>
     * @throws BaseException If error happens.
     */
    public void dropCollection(String collectionName, BSONObject options)
            throws BaseException {
        if (collectionName == null || collectionName.equals("")) {
            throw new BaseException(SDBError.SDB_INVALIDARG,
                                    "collectionName can't be null or empty");
        }
        String collectionFullName = name + "." + collectionName;
        BSONObject rebuildOptions = new BasicBSONObject();
        rebuildOptions.put(SdbConstants.FIELD_NAME_NAME, collectionFullName);
        if (options != null) {
            rebuildOptions.putAll(options);
        }
        AdminRequest request = new AdminRequest(AdminCommand.DROP_CL,
                                                rebuildOptions);
        SdbReply response = sequoiadb.requestAndResponse(request);
        sequoiadb.throwIfError(response, collectionName);
        sequoiadb.removeCache(collectionFullName);
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
        alterCollectionSpace(alterObject);
    }

    /**
     * Alter the current collection space.
     *
     * @param options The options of the collection space:
     *                <ul>
     *                <li>Domain : the domain of collection space.
     *                <li>PageSize : page size of collection space.
     *                <li>LobPageSize : LOB page size of collection space.
     *                </ul>
     * @throws BaseException If error happens.
     */
    public void alterCollectionSpace(BSONObject options) throws BaseException {
        if (null == options) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "options is null");
        }

        BSONObject newObj = new BasicBSONObject();
        if (!options.containsField(SdbConstants.FIELD_NAME_ALTER)) {
            newObj.put(SdbConstants.FIELD_NAME_NAME, name);
            newObj.put(SdbConstants.FIELD_NAME_OPTIONS, options);
        } else {
            Object tmpAlter = options.get(SdbConstants.FIELD_NAME_ALTER);
            if (tmpAlter instanceof BasicBSONObject ||
                    tmpAlter instanceof BasicBSONList) {
                newObj.put(SdbConstants.FIELD_NAME_ALTER, tmpAlter);
            } else {
                throw new BaseException(SDBError.SDB_INVALIDARG, options.toString());
            }
            newObj.put(SdbConstants.FIELD_NAME_ALTER_TYPE, SdbConstants.SDB_ALTER_CS);
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

        AdminRequest request = new AdminRequest(AdminCommand.ALTER_CS, newObj);
        SdbReply response = sequoiadb.requestAndResponse(request);
        sequoiadb.throwIfError(response);
    }

    /**
     * Alter the current collection space to set domain
     *
     * @param options The options of the collection space:
     *                <ul>
     *                <li>Domain : the domain of collection space.
     *                </ul>
     * @throws BaseException If error happens.
     */
    public void setDomain(BSONObject options) throws BaseException {
        alterInternal(SdbConstants.SDB_ALTER_SET_DOMAIN, options, false);
    }

    /**
     * Alter the current collection space to remove domain
     *
     * @throws BaseException If error happens.
     */
    public void removeDomain() throws BaseException {
        alterInternal(SdbConstants.SDB_ALTER_REMOVE_DOMAIN, null, true);
    }

    /**
     * Get the Domain name of the current collection space. Returns an empty string if the current collection space
     * has no owning domain
     * @return the Domain name.
     * @throws BaseException If error happens.
     */
    public String getDomainName(){
        String result = null;
        String cmd = "select Domain from $LIST_CS where Name = '" + this.name + "'";
        DBCursor cursor = sequoiadb.exec(cmd);
        try
        {
            if (cursor.hasNext()) {
                BSONObject obj = cursor.getNext();
                result = (String) obj.get(FIELD_NAME_DOMAIN);
            }
            return result == null ? "" : result;
        }
        finally
        {
            cursor.close();
        }
    }

    /**
     * Alter the current collection space to enable capped
     *
     * @throws BaseException If error happens.
     */
    public void enableCapped() throws BaseException {
        alterInternal(SdbConstants.SDB_ALTER_ENABLE_CAPPED, null, true);
    }

    /**
     * Alter the current collection space to disable capped
     *
     * @throws BaseException If error happens.
     */
    public void disableCapped() throws BaseException {
        alterInternal(SdbConstants.SDB_ALTER_DISABLE_CAPPED, null, true);
    }

    /**
     * Alter the current collection space to set attributes.
     *
     * @param options The options of the collection space:
     *                <ul>
     *                <li>Domain : the domain of collection space.
     *                <li>PageSize : page size of collection space.
     *                <li>LobPageSize : LOB page size of collection space.
     *                </ul>
     * @throws BaseException If error happens.
     */
    public void setAttributes(BSONObject options) throws BaseException {
        alterInternal(SdbConstants.SDB_ALTER_SET_ATTRIBUTES, options, false);
    }

    /**
     * @param oldName The old collection name
     * @param newName The new collection name
     * @throws BaseException If error happens.
     */
    public void renameCollection(String oldName, String newName) throws BaseException {
        if (oldName == null || oldName.length() == 0) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "The old name of collection is null or empty");
        }
        if (newName == null || newName.length() == 0) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "The new name of collection is null or empty");
        }

        BSONObject matcher = new BasicBSONObject();
        matcher.put(SdbConstants.FIELD_NAME_CELLECTIONSPACE, name);
        matcher.put(SdbConstants.FIELD_NAME_OLDNAME, oldName);
        matcher.put(SdbConstants.FIELD_NAME_NEWNAME, newName);

        AdminRequest request = new AdminRequest(AdminCommand.RENAME_CL, matcher);
        SdbReply response = sequoiadb.requestAndResponse(request);
        sequoiadb.throwIfError(response);
        sequoiadb.removeCache(name + "." + oldName);
        sequoiadb.upsertCache(name + "." + newName);
    }
}
