/*
 * Copyright 2022 SequoiaDB Inc.
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

package com.sequoiadb.flink.common.client;


import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.ConfigOptions;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.flink.common.constant.SDBConstant;
import com.sequoiadb.flink.common.exception.SDBException;
import com.sequoiadb.flink.common.util.SDBInfoUtil;
import com.sequoiadb.flink.config.SDBSinkOptions;
import org.apache.flink.util.Preconditions;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;


public class SDBCollectionProvider implements SDBClientProvider {

    private static final Logger LOG = LoggerFactory.getLogger(SDBCollectionProvider.class);

    private static final BSONObject INDEX_OPTIONS = new BasicBSONObject();

    static {
        INDEX_OPTIONS.put(SDBConstant.INDEX_UNIQUE, true);
        INDEX_OPTIONS.put(SDBConstant.INDEX_NOT_NULL, true);
    }

    private static final String PRIMARY_KEY_NAME = "PRIMARY";

    // sequoiadb connection config
    private final List<String> hosts;
    private final String collectionSpaceStr;
    private final String collectionStr;
    private final String username;
    private final String password;

    private final SDBSinkOptions sinkOptions;

    // sequoiadb handler
    private transient Sequoiadb sdb;
    private transient CollectionSpace collectionSpace;
    private transient DBCollection collection;

    public SDBCollectionProvider(
            List<String> hosts,
            String collectionSpaceStr,
            String collectionStr,
            String username,
            String password,
            SDBSinkOptions sinkOptions) {
        Preconditions.checkNotNull(hosts);
        Preconditions.checkNotNull(collectionSpaceStr);
        Preconditions.checkNotNull(collectionStr);
        Preconditions.checkNotNull(username);
        Preconditions.checkNotNull(password);

        this.hosts = hosts;
        this.collectionSpaceStr = collectionSpaceStr;
        this.collectionStr = collectionStr;
        this.username = username;
        this.password = password;
        this.sinkOptions = sinkOptions;
    }

    /*
     * return a SDB Client.
     * SDB client is not init until it is used.
     * To avoid lock level upgrade caused slow down,
     * TransMaxLockNum is configured.
     * @return Sequoiadb
     */

    private Sequoiadb getClient() {
        if (sdb == null) {
            ConfigOptions options = new ConfigOptions();
            options.setSocketKeepAlive(true);
            sdb = new Sequoiadb(hosts, username, password, options);

            // set up source info in session attr, ignore failure and just
            // print warning log when throws exception.
            if (sinkOptions != null) {
                SDBInfoUtil.setupSourceSessionAttrIgnoreFailures(sdb, sinkOptions.getSourceInfo());
            }

            try {
                /* Because it is a new feature in 3.4.5
                 * It will fail when set it in older version SDB
                 * and the error is caught.
                 * but the error doesn't affect the normal usage of Sequoiadb session
                 */
                BasicBSONObject sessionOptions = new BasicBSONObject();
                sessionOptions.put(SDBConstant.TRANS_MAX_LOCK_NUM, -1);
                sdb.setSessionAttr(sessionOptions);
            } catch (BaseException e) {
                if (e.getErrorCode() != SDBError.SDB_INVALIDARG.getErrorCode()) {
                    throw e;
                }
            }
        }
        return sdb;
    }

    /**
     * setup source info by setter
     */
    public void setupSourceInfo(String sourceInfo) {
        SDBInfoUtil.setupSourceSessionAttrIgnoreFailures(getClient(), sourceInfo);
    }

    @Override
    public CollectionSpace getCollectionSpace() {
        if (collectionSpace == null) {
            try {
                collectionSpace = getClient().getCollectionSpace(collectionSpaceStr);
            } catch (BaseException ex) {
                throw new SDBException("cannot get collection space from Sequoiadb.", ex);
            }
        }
        return collectionSpace;
    }


    @Override
    public DBCollection getCollection() {
        if (collection == null) {
            try {
                collection = getCollectionSpace().getCollection(collectionStr);
            } catch (BaseException ex) {
                throw new SDBException("cannot get collection from Sequoiadb.", ex);
            }
        }
        return collection;
    }


    /**
     * obtain all index information of Sequoiadb table created by flink
     *
     * @return List<BSONObject>: a list of indexDef in Bson format
     */

    public List<BSONObject> getIndexDefList() {
        List<BSONObject> indexList = new ArrayList<>();

        try (DBCursor indexIterator = getCollection().getIndexes()) {
            // get index bsonObject iteator from Sequoiadb
            while (indexIterator.hasNext()) {
                BSONObject indexBsonObj = indexIterator.getNext();
                //get indexdef corresponding a bsonObject from index bsonObject
                BSONObject indexDef = (BSONObject) indexBsonObj.get(SDBConstant.INDEX_DEF);
                indexList.add(indexDef);
            }
        } catch (BaseException e) {
            throw new SDBException("cannot get collection index from Sequoiadb");
        }

        return indexList;
    }


    /**
     * return a list of string type index column names
     * this is to help me determine whether the join field of Lookup Join uses
     * the index field.
     *
     * @return List<String>
     */

    public List<String> getIndexColumnNames() {
        List<String> indexNameList = new ArrayList<>();
        List<BSONObject> indexDefList = getIndexDefList();

        for (BSONObject index : indexDefList) {
            BSONObject key = (BSONObject) index.get(SDBConstant.KEY);
            Iterator<String> indexIterator = key.keySet().iterator();
            while (indexIterator.hasNext()) {
                indexNameList.add(indexIterator.next());
            }
        }

        return indexNameList;
    }

    /**
     * check if collectionSpace and collection in user definition of flink table exists
     * create if it doesn't exist,including the index.
     *
     * @param sinkOptions include user definition
     */
    public static void ensureCollectionSpaceWithCollection(SDBSinkOptions sinkOptions) {

        Sequoiadb sdb = new Sequoiadb(
                sinkOptions.getHosts(),
                sinkOptions.getUsername(),
                sinkOptions.getPassword(),
                new ConfigOptions()
        );

        boolean isCreatedCS = false;

        BSONObject optionsCS = new BasicBSONObject();
        optionsCS.put(SDBConstant.PAGE_SIZE, sinkOptions.getPageSize());

        String domain = sinkOptions.getDomain();
        if (domain != null) {
            optionsCS.put(SDBConstant.DOMAIN, domain);
        }

        CollectionSpace collectionSpace = null;
        try {
            collectionSpace = sdb.createCollectionSpace(sinkOptions.getCollectionSpace(), optionsCS);
            isCreatedCS = true;
        } catch (BaseException ex) {
            if (ex.getErrorCode() == SDBError.SDB_DMS_CS_EXIST.getErrorCode()) {
                collectionSpace = sdb.getCollectionSpace(sinkOptions.getCollectionSpace());
            } else {
                throw ex;
            }
        }

        BSONObject optionsCL = new BasicBSONObject();
        String shardingKey = sinkOptions.getShardingKey();
        String[] primaryKey = sinkOptions.getUpsertKey();

        // using primary key which defines in flink table to build pk bson,
        // pk bson is like {id: 1, name: 1}
        BSONObject pkBson = new BasicBSONObject();
        if (primaryKey != null && primaryKey.length != 0) {
            for (String upsertKey : primaryKey) {
                pkBson.put(upsertKey, 1);
            }
        }

        if (sinkOptions.getAutoPartition()) {
            if (shardingKey != null) {
                BSONObject skBson = (BSONObject) JSON.parse(shardingKey);
                //verity pk contain all fields in sharding key
                if (!pkBson.isEmpty() && !SDBInfoUtil.containValidation(pkBson, skBson)) {
                    throw new SDBException(String.format("The primary key must include all fields in sharding key, " +
                            "primary key:%s, sharding key:%s,please drop flink mapping table and select appropriate " +
                            "primary key to create a new table", pkBson, skBson));
                }

                optionsCL.put(SDBConstant.SHARDING_KEY, skBson);
                optionsCL.put(SDBConstant.SHARDING_TYPE, sinkOptions.getShardingType());
                optionsCL.put(SDBConstant.AUTO_SPLIT, true);
            } else if (!pkBson.isEmpty()) {
                // if user don't specify sharding key, using primary key as sharding key.
                optionsCL.put(SDBConstant.SHARDING_KEY, pkBson);
                optionsCL.put(SDBConstant.SHARDING_TYPE, sinkOptions.getShardingType());
                optionsCL.put(SDBConstant.AUTO_SPLIT, true);
            }
        } else {
            if (shardingKey != null) {
                throw new SDBException(String.format("Incompatible parameters passed in: autopartition is false " +
                        "while shardingkey(%s) is specified. ", shardingKey));
            }
        }
        optionsCL.put(SDBConstant.REPL_SIZE, sinkOptions.getReplSize());
        optionsCL.put(SDBConstant.COMPRESSION_TYPE, sinkOptions.getCompressionType());

        String Group = sinkOptions.getGroup();
        if (Group != null) {
            optionsCL.put(SDBConstant.GROUP, Group);
        }

        DBCollection collection = null;
        try {
            collection = collectionSpace.createCollection(sinkOptions.getCollection(), optionsCL);
        } catch (BaseException ex) {
            if (ex.getErrorCode() == SDBError.SDB_DMS_EXIST.getErrorCode()) {
                collection = collectionSpace.getCollection(sinkOptions.getCollection());
                return;
            } else {
                try{
                    // delete collectionSpace when it failed to create collection
                    if(isCreatedCS){
                        sdb.dropCollectionSpace(sinkOptions.getCollectionSpace());
                    }
                } catch (BaseException e) {
                    throw new SDBException("cleanup collectionSpace or collection failed.",e);
                }
                throw ex;
            }
        }

        try {
            if (collection != null && !pkBson.isEmpty()) {
                collection.createIndex(PRIMARY_KEY_NAME, pkBson, INDEX_OPTIONS);
            }
        } catch (BaseException ex) {
            if (ex.getErrorCode() == SDBError.SDB_IXM_EXIST.getErrorCode()) {
                // ignore when primary key is already exist
            } else {
                // directly delete collection
                // there isn't the collection that exists in the first place
                try{
                    collectionSpace.dropCollection(sinkOptions.getCollection());
                    if(isCreatedCS){
                        sdb.dropCollectionSpace(sinkOptions.getCollectionSpace());
                    }
                } catch (BaseException e) {
                    throw new SDBException("cleanup collectionSpace or collection failed.",e);
                }
                throw ex;
            }
        } finally {
            sdb.close();
        }
    }

    @Override
    public void close() throws IOException {
        try {
            if (sdb != null) {
                sdb.close();
            }
        } catch (Exception ex) {
            throw new SDBException("close sequoiadb connection failed.",
                    ex);
        } finally {
            sdb = null;
            collectionSpace = null;
            collection = null;
        }
    }

}
