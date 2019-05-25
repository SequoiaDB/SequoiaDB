/*
 * Copyright 2017 SequoiaDB Inc.
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

import java.util.ArrayList;
import java.util.List;

/**
 * Collection space of SequoiaDB.
 */
public class CollectionSpace {
    private String name;
    private Sequoiadb sequoiadb;

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
    public DBCollection getCollection(String collectionName)
            throws BaseException {
        String collectionFullName = name + "." + collectionName;
        if (sequoiadb.fetchCache(collectionFullName)) {
            return new DBCollection(sequoiadb, this, collectionName);
        }

        if (isCollectionExist(collectionName)) {
            return new DBCollection(sequoiadb, this, collectionName);
        } else {
            throw new BaseException(SDBError.SDB_DMS_NOTEXIST, collectionName);
        }
    }

    /**
     * Verify the existence of collection in current collection space.
     *
     * @param collectionName The collection name
     * @return True if collection existed or False if not existed
     * @throws BaseException If error happens.
     */
    public boolean isCollectionExist(String collectionName) throws BaseException {
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
        List<String> collectionNames = new ArrayList<String>();
        List<String> colNames = sequoiadb.getCollectionNames();
        if ((colNames != null) && (colNames.size() != 0)) {
            for (String col : colNames) {
                if (col.startsWith(name + ".")) {
                    collectionNames.add(col);
                }
            }
        }
        return collectionNames;
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
     * @param options        The options for creating collection, including
     *                       "ShardingKey", "ReplSize", "IsMainCL" and "Compressed" informations,
     *                       no options, if null
     * @return the newly created object of collection
     * @throws BaseException Tf error happens.
     */
    public DBCollection createCollection(String collectionName,
                                         BSONObject options) {
        if (isCollectionExist(collectionName)) {
            throw new BaseException(SDBError.SDB_DMS_EXIST, collectionName);
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
        if (!isCollectionExist(collectionName)) {
            throw new BaseException(SDBError.SDB_DMS_NOTEXIST, collectionName);
        }

        String collectionFullName = name + "." + collectionName;

        BSONObject obj = new BasicBSONObject();
        obj.put(SdbConstants.FIELD_NAME_NAME, collectionFullName);

        AdminRequest request = new AdminRequest(AdminCommand.DROP_CL, obj);
        SdbReply response = sequoiadb.requestAndResponse(request);
        sequoiadb.throwIfError(response, collectionName);
        sequoiadb.removeCache(collectionFullName);
    }
}
