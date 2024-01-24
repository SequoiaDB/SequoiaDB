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

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.ConfigOptions;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.flink.common.constant.SDBConstant;
import com.sequoiadb.flink.common.exception.SDBException;
import com.sequoiadb.flink.common.util.SDBInfoUtil;
import com.sequoiadb.flink.config.SDBSinkOptions;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;

public class SDBSinkClient implements SDBClient {

    private static final BSONObject INDEX_OPTIONS = new BasicBSONObject();

    static {
        INDEX_OPTIONS.put(SDBConstant.INDEX_UNIQUE, true);
        INDEX_OPTIONS.put(SDBConstant.INDEX_NOT_NULL, true);
    }

    private final List<String> hosts;
    private final String collectionSpace;
    private final String collection;

    private final String username;
    private final String password;
    private final SDBSinkOptions sdboptions;

    private Sequoiadb sdb;
    private CollectionSpace cs;
    private DBCollection cl;

    private static final Logger LOG = LoggerFactory.getLogger(SDBSinkClient.class);
    private final String PRIMARY_KEY = "PRIMARY";

    private SDBSinkClient(
            List<String> hosts,
            String collectionSpace,
            String collection,
            String username,
            String password,
            SDBSinkOptions sdboptions) {

        this.hosts = hosts;
        this.collectionSpace = collectionSpace;
        this.collection = collection;
        this.username = username;
        this.password = password;
        this.sdboptions = sdboptions;

    }

    private SDBSinkClient(
            List<String> hosts,
            String collectionSpace,
            String collection,
            String username,
            String password
    ) {
        this.hosts = hosts;
        this.collectionSpace = collectionSpace;
        this.collection = collection;
        this.username = username;
        this.password = password;
        this.sdboptions = null;
    }

    /*
     * return a SDBClient built with sdbOptions and hosts
     * @param sdbOptions            Sinkoptions from user input
     * @return SDBClient
     */
    public static SDBClient createClient(SDBSinkOptions sdbOptions) {
        LOG.info("create SDBSinkClient");
        return new SDBSinkClient(
                sdbOptions.getHosts(),
                sdbOptions.getCollectionSpace(),
                sdbOptions.getCollection(),
                sdbOptions.getUsername(),
                sdbOptions.getPassword(),
                sdbOptions);

    }

    /*
     * return a SDBClient built with sdbOptions and hosts
     * @param sdbOptions            Sinkoptions from user input
     * @param hosts                 list of SDB hosts
     * @return SDBClient
     */
    public static SDBClient createClientWithHost(SDBSinkOptions sdbOptions, List<String> hosts) {
        LOG.info("create SDBSinkClient with hosts");
        return new SDBSinkClient(
                hosts,
                sdbOptions.getCollectionSpace(),
                sdbOptions.getCollection(),
                sdbOptions.getUsername(),
                sdbOptions.getPassword(),
                sdbOptions);
    }

    /*
     * return if collection has uniqueindex
     * by request index info from SDB
     * @param hosts                 SDB hosts
     * @param collectionSpace       collection space name
     * @param collection            collection name
     * @param username              SDB username
     * @param password              SDB password
     * @return boolean
     */
    public static List<HashSet<String>> checkUniqueIndex(
            List<String> hosts, String collectionSpace, String collection, String username, String password) {
        ConfigOptions options = new ConfigOptions();
        Sequoiadb db = new Sequoiadb(hosts, username, password, options);
        Boolean unique = false;
        List<HashSet<String>> unique_indexes = new ArrayList<HashSet<String>>();
        LOG.info("check idempotent");
        try {
            DBCursor indexes = db.getCollectionSpace(collectionSpace).getCollection(collection).getIndexes();
            while (indexes.hasNext()) {
                BSONObject index = (BSONObject) indexes.getNext().get(SDBConstant.INDEX_DEF);
                unique = (Boolean) index.get(SDBConstant.UNIQUE);
                if (unique) {
                    BSONObject keys = (BSONObject) index.get(SDBConstant.KEY);
                    unique_indexes.add(new HashSet<String>(keys.toMap().keySet()));
                }
            }
            db.getCollectionSpace(collectionSpace).getCollection(collection).createIdIndex(null);
        } catch (BaseException e) {
            throw e;
        } finally {
            db.close();
        }

        return unique_indexes;
    }

    /*
     * return a SDB Client.
     * SDB client is not init until it is used.
     * To avoid lock level upgrade caused slow down,
     * TransMaxLockNum is configured.
     * @return Sequoiadb
     */
    @Override
    public Sequoiadb getClient() {
        if (sdb == null) {
            ConfigOptions options = new ConfigOptions();
            options.setSocketKeepAlive(true);
            sdb = new Sequoiadb(hosts, username, password, options);

            // set up source info in session attr, ignore failure and just
            // print warning log when throws exception.
            SDBInfoUtil.setupSourceSessionAttrIgnoreFailures(sdb, sdboptions.getSourceInfo());

            try {
                /* Because it is a new feature in 3.4.5
                 * It will fail when set it in older version SDB
                 * added it in try catch to stop flink applcation crashing
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

    /*
     * return a SDB collectionspace.
     * collectionspace will not be checked until this funciton is called
     * if there is not a collection space exist with cs name
     * it will create a new one with user options
     * @return CollectionSpace
     */
    @Override
    public CollectionSpace getCS() {
        if (cs == null) {
            try {
                cs = getClient().getCollectionSpace(collectionSpace);
            } catch (BaseException e) {
                throw new SDBException("cannot get collection space from Sequoiadb.", e);
            }
        }
        return cs;
    }

    /*
     * return a SDB collection.
     * collection will not be checked until here
     * if there is not a collection exist with cl name
     * it will create a new one with user options
     * @return DBCollection
     */
    @Override
    public DBCollection getCL() {
        if (cl == null) {
            try {
                cl = getCS().getCollection(collection);
            } catch (BaseException e) {
                throw new SDBException("cannot get collection from Sequoiadb.", e);
            }
        }
        return cl;
    }

    @Override
    public void close() {
        if (sdb != null) {
            sdb.close();
        }
        sdb = null;
        cs = null;
        cl = null;
    }

    /**
     * return a SDB collection space.
     * A new collection space will be created with options
     *
     * @param collectionSpaceStr name of collection space
     * @return collectionSpace
     */
    private CollectionSpace ensureCollectionSpaceWithOptions(String collectionSpaceStr) {
        BSONObject options = new BasicBSONObject();
        options.put(SDBConstant.PAGE_SIZE, sdboptions.getPageSize());
        String domain = sdboptions.getDomain();
        if (domain != null) {
            options.put(SDBConstant.DOMAIN, domain);
        }

        CollectionSpace collectionSpace = null;
        try {
            collectionSpace = getClient()
                    .createCollectionSpace(collectionSpaceStr, options);
        } catch (BaseException ex) {
            if (ex.getErrorCode() == SDBError.SDB_DMS_CS_EXIST.getErrorCode()) {
                collectionSpace = getClient().getCollectionSpace(collectionSpaceStr);
            } else {
                throw ex;
            }
        }

        return collectionSpace;
    }

    /**
     * create collection with options if collection isn't exist.
     * <p>
     * A new collection will be created with options
     *
     * @param collection name of collection
     * @param primaryKey primary specified in flink sql table
     * @return DBCollection
     * @throws BaseException throw exception when collection is already exist.
     */
    private DBCollection ensureCollectionWithOptions(String collection, HashSet<String> primaryKey) {
        BSONObject options = new BasicBSONObject();
        String shardingKey = sdboptions.getShardingKey();

        BSONObject pkBson = new BasicBSONObject();
        if (primaryKey != null) {
            for (String key : primaryKey) {
                pkBson.put(key, 1);
            }
        }

        if (sdboptions.getAutoPartition()) {
            if (shardingKey != null) {
                BSONObject skBson = (BSONObject) JSON.parse(shardingKey);
                //verity pk contain all fields in sharding key
                if (!pkBson.isEmpty() && !SDBInfoUtil.containValidation(pkBson, skBson)) {
                    throw new SDBException(String.format("The primary key must include all fields in sharding key, " +
                            "primary key:%s, sharding key:%s,please drop flink mapping table and select appropriate " +
                            "primary key to create a new table", pkBson, skBson));
                }

                options.put(SDBConstant.SHARDING_KEY, skBson);
                options.put(SDBConstant.SHARDING_TYPE, sdboptions.getShardingType());
                options.put(SDBConstant.AUTO_SPLIT, true);
            } else if (!pkBson.isEmpty()) {
                // if user don't specify sharding key, using primary key as sharding key.
                options.put(SDBConstant.SHARDING_KEY, pkBson);
                options.put(SDBConstant.SHARDING_TYPE, sdboptions.getShardingType());
                options.put(SDBConstant.AUTO_SPLIT, true);
            }
        } else {
            if (shardingKey != null) {
                throw new SDBException(String.format("Incompatible parameters passed in: autopartition is false " +
                        "while shardingkey(%s) is specified. ", shardingKey));
            }
        }
        options.put(SDBConstant.REPL_SIZE, sdboptions.getReplSize());
        options.put(SDBConstant.COMPRESSION_TYPE, sdboptions.getCompressionType());


        String Group = sdboptions.getGroup();
        if (Group != null) {
            options.put(SDBConstant.GROUP, Group);
        }

        DBCollection cl = null;
        try {
            cl = getCS().createCollection(collection, options);
        } catch (BaseException ex) {
            if (ex.getErrorCode() == SDBError.SDB_DMS_EXIST.getErrorCode()) {
                cl = getCS().getCollection(collection);
            } else {
                throw ex;
            }
        }

        try {
            if (cl != null && !pkBson.isEmpty()) {
                cl.createIndex(PRIMARY_KEY, pkBson, INDEX_OPTIONS);
            }
        } catch (BaseException ex) {
            if (ex.getErrorCode() == SDBError.SDB_IXM_EXIST.getErrorCode()) {
                // ignore when primary key is already exist
            } else {
                throw ex;
            }
        }

        return cl;
    }

    @Override
    public String toString() {
        return "SDBSinkClient [collection=" + collection + ", collectionSpace=" + collectionSpace
                + ", hosts=" + hosts + ", password=" + password
                + ", sdboptions=" + sdboptions + ", username="
                + username + "]";
    }

}
