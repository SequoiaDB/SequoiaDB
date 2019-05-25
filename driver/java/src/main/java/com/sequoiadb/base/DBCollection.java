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
import com.sequoiadb.message.request.*;
import com.sequoiadb.message.response.SdbReply;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;
import org.bson.util.JSON;

import java.util.*;

/**
 * Collection of SequoiaDB.
 */
public class DBCollection {
    private String name;
    private Sequoiadb sequoiadb;
    private CollectionSpace collectionSpace;
    private String csName;
    private String collectionFullName;
    private Set<String> mainKeys;
    private boolean ensureOID;

    /**
     * This flags represent that bulkInsert will continue when
     * duplicate key exist(the duplicate record will be ignored).
     */
    public static final int FLG_INSERT_CONTONDUP = 0x00000001;

    /**
     * The sharding key in update rule is not filtered,
     * when executing update or upsert.
     */
    public static final int FLG_UPDATE_KEEP_SHARDINGKEY = 0x00008000;

    /**
     * Get the name of current collection.
     *
     * @return The collection name
     */
    public String getName() {
        return name;
    }

    /**
     * Get the full name of specified collection in current collection space.
     *
     * @return The full name of specified collection
     */
    public String getFullName() {
        return collectionFullName;
    }

    /**
     * Get the full name of specified collection in current collection space.
     *
     * @return The full name of specified collection.
     */
    public String getCSName() {
        return csName;
    }

    /**
     * Get the Sequoiadb instance of current collection.
     *
     * @return Sequoiadb instance
     */
    public Sequoiadb getSequoiadb() {
        return sequoiadb;
    }

    /**
     * Get the CollectionSpace instance of current collection.
     *
     * @return CollectionSpace instance
     */
    public CollectionSpace getCollectionSpace() {
        return collectionSpace;
    }

    /**
     * Set the main keys used in save(). if no main keys are set, use the
     * default main key "_id".
     * Every time invokes this method, it will remove the main keys set in the last time.
     *
     * @param keys the main keys specified by user. the main key should exist in the
     *             object
     * @throws BaseException when keys is null
     */
    public void setMainKeys(String[] keys) throws BaseException {
        if (keys == null)
            throw new BaseException(SDBError.SDB_INVALIDARG, "keys is null");
        mainKeys.clear();

        if (keys.length == 0) {
            return;
        }

        for (String k : keys) {
            mainKeys.add(k);
        }
    }

    /**
     * @param sequoiadb Sequoiadb object
     * @param cs        CollectionSpace object
     * @param name      Collection name
     */
    DBCollection(Sequoiadb sequoiadb, CollectionSpace cs, String name) {
        this.name = name;
        this.sequoiadb = sequoiadb;
        this.collectionSpace = cs;
        this.csName = cs.getName();
        this.collectionFullName = csName + "." + name;
        this.mainKeys = new HashSet<String>();
        this.ensureOID = true;
    }

    /**
     * Insert a document into current collection, if the document
     * does not contain field "_id", it will be added.
     *
     * @param insertor The Bson object of insertor, can't be null
     * @return the value of the filed "_id"
     * @throws BaseException If error happens.
     */
    public Object insert(BSONObject insertor) throws BaseException {
        if (insertor == null) {
            throw new BaseException(SDBError.SDB_INVALIDARG);
        }

        Object retObj = insertor.get(SdbConstants.OID);
        if (retObj == null) {
            ObjectId objId = ObjectId.get();
            insertor.put(SdbConstants.OID, objId);
            retObj = objId;
        }

        InsertRequest request = new InsertRequest(collectionFullName, insertor);
        SdbReply response = sequoiadb.requestAndResponse(request);
        sequoiadb.throwIfError(response, insertor);
        sequoiadb.upsertCache(collectionFullName);
        return retObj;
    }

    /**
     * Insert a document into current collection, if the document
     * does not contain field "_id", it will be added.
     *
     * @param insertor The string of insertor
     * @return the value of the filed "_id"
     * @throws BaseException If error happens.
     */
    public Object insert(String insertor) throws BaseException {
        BSONObject in = null;
        if (insertor != null) {
            in = (BSONObject) JSON.parse(insertor);
        }
        return insert(in);
    }

    /**
     * Insert a bulk of bson objects into current collection.
     *
     * @param insertor The Bson object of insertor list, can't be null
     * @param flag     available value is FLG_INSERT_CONTONDUP or 0.
     *                 if flag = FLG_INSERT_CONTONDUP, bulkInsert will continue when Duplicate
     *                 key exist.(the duplicate record will be ignored);
     *                 if flag = 0, bulkInsert will interrupt when Duplicate key exist.
     * @throws BaseException If error happens.
     * @since 2.9
     */
    public void insert(List<BSONObject> insertor, int flag) throws BaseException {
        if (flag != 0 && flag != FLG_INSERT_CONTONDUP) {
            throw new BaseException(SDBError.SDB_INVALIDARG);
        }
        if (insertor == null || insertor.size() == 0) {
            throw new BaseException(SDBError.SDB_INVALIDARG);
        }

        InsertRequest request = new InsertRequest(collectionFullName, insertor, flag, ensureOID);
        SdbReply response = sequoiadb.requestAndResponse(request);
        sequoiadb.throwIfError(response);
        sequoiadb.upsertCache(collectionFullName);
    }

    /**
     * Insert a bulk of bson objects into current collection.
     *
     * @param insertor The Bson object of insertor list, can't be null.
     *                 insert will interrupt when Duplicate key exist.
     * @throws BaseException If error happens.
     * @since 2.9
     */
    public void insert(List<BSONObject> insertor) throws BaseException {
        insert(insertor, 0);
    }

    /**
     * Insert an object into current collection.
     * When flag is set to 0, it won't work to update the ShardingKey field, but the other fields take effect.
     *
     * @param type            The object of insertor, can't be null
     * @param ignoreNullValue true:if type's inner value is null, it will not save to collection;
     * @param flag            the update flag, default to be 0. Please see the definition
     *                        of follow flags for more detail.
     *                        <ul>
     *                        <li>DBCollection.FLG_UPDATE_KEEP_SHARDINGKEY
     *                        </ul>
     * @throws BaseException 1.when the type is not support, throw BaseException with the type "SDB_INVALIDARG"
     *                       2.when offer main keys by setMainKeys(), and try to update "_id" field,
     *                       it may get a BaseException with the type of "SDB_IXM_DUP_KEY"
     * @see com.sequoiadb.base.DBCollection#setMainKeys(String[])
     */
    public /*! @cond x*/ <T> /*! @endcond */ void save(T type, Boolean ignoreNullValue,
                                                       int flag) throws BaseException {
        BSONObject obj;
        try {
            obj = BasicBSONObject.typeToBson(type, ignoreNullValue);
        } catch (Exception e) {
            throw new BaseException(SDBError.SDB_INVALIDARG, type.toString(), e);
        }
        BSONObject matcher = new BasicBSONObject();
        BSONObject modifer = new BasicBSONObject();
        if (mainKeys.isEmpty()) {
            Object id = obj.get(SdbConstants.OID);
            if (id == null || (id instanceof ObjectId && ((ObjectId) id).isNew())) {
                if (id != null && id instanceof ObjectId)
                    ((ObjectId) id).notNew();
                insert(obj);
            } else {
                matcher.put(SdbConstants.OID, id);
                modifer.put("$set", obj);
                upsert(matcher, modifer, null, null, flag);
            }
        } else { // if user specify main keys, use these main keys
            Iterator<String> it = mainKeys.iterator();
            while (it.hasNext()) {
                String key = it.next();
                if (obj.containsField(key))
                    matcher.put(key, obj.get(key));
            }
            if (!matcher.isEmpty()) {
                modifer.put("$set", obj);
                upsert(matcher, modifer, null, null, flag);
            } else {
                insert(obj);
            }
        }
    }

    /**
     * Insert an object into current collection
     * when save include update shardingKey field, the shardingKey modify action is not take effect, but the other
     * field update is take effect.
     *
     * @param type            The object of insertor, can't be null
     * @param ignoreNullValue true:if type's inner value is null, it will not save to collection;
     *                        false:if type's inner value is null, it will save to collection too.
     * @throws BaseException 1.when the type is not support, throw BaseException with the type "SDB_INVALIDARG"
     *                       2.when offer main keys by setMainKeys(), and try to update "_id" field,
     *                       it may get a BaseException with the type of "SDB_IXM_DUP_KEY"
     * @see com.sequoiadb.base.DBCollection#setMainKeys(String[])
     */
    public /*! @cond x*/ <T> /*! @endcond */ void save(T type, Boolean ignoreNullValue) throws BaseException {
        save(type, ignoreNullValue, 0);
    }

    /**
     * Insert an object into current collection.
     * when save include update shardingKey field, the shardingKey modify action is not take effect, but the other
     * field update is take effect.
     *
     * @param type The object of insertor, can't be null
     * @throws BaseException 1.when the type is not support, throw BaseException with the type "SDB_INVALIDARG"
     *                       2.when offer main keys by setMainKeys(), and try to update "_id" field,
     *                       it may get a BaseException with the type of "SDB_IXM_DUP_KEY"
     * @see com.sequoiadb.base.DBCollection#setMainKeys(String[])
     */
    public /*! @cond x*/ <T> /*! @endcond */ void save(T type) throws BaseException {
        save(type, false);
    }

    /**
     * Insert an object into current collection.
     * When flag is set to 0, it won't work to update the ShardingKey field, but the other fields take effect.
     *
     * @param type            The List instance of insertor, can't be null or empty
     * @param ignoreNullValue true:if type's inner value is null, it will not save to collection;
     * @param flag            the update flag, default to be 0. Please see the definition
     *                        of follow flags for more detail.
     *                        <ul>
     *                        <li>DBCollection.FLG_UPDATE_KEEP_SHARDINGKEY
     *                        </ul>
     * @throws BaseException 1.while the input argument is null or the List instance is empty
     *                       2.while the type is not support, throw BaseException with the type "SDB_INVALIDARG"
     *                       3.while offer main keys by setMainKeys(), and try to update "_id" field,
     *                       it may get a BaseException with the type of "SDB_IXM_DUP_KEY" when the "_id" field you
     *                       want to update to had been existing in database
     * @see com.sequoiadb.base.DBCollection#setMainKeys(String[])
     */
    public /*! @cond x*/ <T> /*! @endcond */ void save(List<T> type, Boolean ignoreNullValue,
                                                       int flag) throws BaseException {
        if (type == null || type.size() == 0)
            throw new BaseException(SDBError.SDB_INVALIDARG, "type is empty or null");
        List<BSONObject> objs = new ArrayList<BSONObject>();
        try {
            Iterator<T> it = type.iterator();
            while (it != null && it.hasNext()) {
                objs.add(BasicBSONObject.typeToBson(it.next(), ignoreNullValue));
            }
        } catch (Exception e) {
            throw new BaseException(SDBError.SDB_INVALIDARG, type.toString(), e);
        }
        BSONObject matcher = new BasicBSONObject();
        BSONObject modifer = new BasicBSONObject();
        BSONObject obj = null;
        Iterator<BSONObject> ite = objs.iterator();
        if (mainKeys.isEmpty()) {
            while (ite != null && ite.hasNext()) {
                obj = ite.next();
                Object id = obj.get(SdbConstants.OID);
                if (id == null || (id instanceof ObjectId && ((ObjectId) id).isNew())) {
                    if (id != null && id instanceof ObjectId)
                        ((ObjectId) id).notNew();
                    insert(obj);
                } else {
                    matcher.put(SdbConstants.OID, id);
                    modifer.put("$set", obj);
                    upsert(matcher, modifer, null, null, flag);
                }
            }
        } else { // if user specify main keys, use these main keys
            while (ite != null && ite.hasNext()) {
                obj = ite.next();
                Iterator<String> i = mainKeys.iterator();
                while (i.hasNext()) {
                    String key = i.next();
                    if (obj.containsField(key))
                        matcher.put(key, obj.get(key));
                }
                if (!matcher.isEmpty()) {
                    modifer.put("$set", obj);
                    upsert(matcher, modifer, null, null, flag);
                } else {
                    insert(obj);
                }
            }
        }
    }

    /**
     * Insert an object into current collection.
     * when save include update shardingKey field, the shardingKey modify action is not take effect, but the other
     * field update is take effect.
     *
     * @param type            The List instance of insertor, can't be null or empty
     * @param ignoreNullValue true:if type's inner value is null, it will not save to collection;
     *                        false:if type's inner value is null, it will save to collection too.
     * @throws BaseException 1.while the input argument is null or the List instance is empty
     *                       2.while the type is not support, throw BaseException with the type "SDB_INVALIDARG"
     *                       3.while offer main keys by setMainKeys(), and try to update "_id" field,
     *                       it may get a BaseException with the type of "SDB_IXM_DUP_KEY" when the "_id" field you
     *                       want to update to had been existing in database
     * @see com.sequoiadb.base.DBCollection#setMainKeys(String[])
     */
    public /*! @cond x*/ <T> /*! @endcond */ void save(List<T> type, Boolean ignoreNullValue) throws BaseException {
        save(type, ignoreNullValue, 0);
    }

    /**
     * Insert an object into current collection.
     * when save include update shardingKey field, the shardingKey modify action is not take effect, but the other
     * field update is take effect.
     *
     * @param type The List instance of insertor, can't be null or empty
     * @throws BaseException 1.while the input argument is null or the List instance is empty
     *                       2.while the type is not support, throw BaseException with the type "SDB_INVALIDARG"
     *                       3.while offer main keys by setMainKeys(), and try to update "_id" field,
     *                       it may get a BaseException with the type of "SDB_IXM_DUP_KEY" when the "_id" field you
     *                       want to update to had been existing in database
     * @see com.sequoiadb.base.DBCollection#setMainKeys(String[])
     */
    public /*! @cond x*/ <T> /*! @endcond */ void save(List<T> type) throws BaseException {
        save(type, false);
    }

    /**
     * Set whether ensure OID of record when bulk insert records to SequoiaDB.
     *
     * @param flag whether ensure OID of record
     */
    public void ensureOID(boolean flag) {
        ensureOID = flag;
    }

    /**
     * @return True if ensure OID of record when bulk insert records to SequoiaDB and false if not.
     */
    public boolean isOIDEnsured() {
        return ensureOID;
    }

    /**
     * Insert a bulk of bson objects into current collection.
     *
     * @param insertor The Bson object of insertor list, can't be null
     * @param flag     available value is FLG_INSERT_CONTONDUP or 0.
     *                 if flag = FLG_INSERT_CONTONDUP, bulkInsert will continue when Duplicate
     *                 key exist.(the duplicate record will be ignored);
     *                 if flag = 0, bulkInsert will interrupt when Duplicate key exist.
     * @throws BaseException If error happens.
     * @deprecated use insert(List<BSONObject> insertor, int flag) instead
     */
    @Deprecated
    public void bulkInsert(List<BSONObject> insertor, int flag) throws BaseException {
        insert(insertor, flag);
    }

    /**
     * Delete the matching BSONObject of current collection.
     *
     * @param matcher The matching condition, delete all the documents if null
     * @throws BaseException If error happens.
     */
    public void delete(BSONObject matcher) throws BaseException {
        delete(matcher, null);
    }

    /**
     * Delete the matching of current collection.
     *
     * @param matcher The matching condition, delete all the documents if null
     * @throws BaseException If error happens.
     */
    public void delete(String matcher) throws BaseException {
        BSONObject ma = null;
        if (matcher != null) {
            ma = (BSONObject) JSON.parse(matcher);
        }
        delete(ma, null);
    }

    /**
     * Delete the matching bson's string of current collection.
     *
     * @param matcher The matching condition, delete all the documents if null
     * @param hint    Specified the index used to scan data. e.g. {"":"ageIndex"} means
     *                using index "ageIndex" to scan data(index scan);
     *                {"":null} means table scan. when hint is null,
     *                database automatically match the optimal index to scan data.
     * @throws BaseException If error happens.
     */
    public void delete(String matcher, String hint) throws BaseException {
        BSONObject ma = null;
        BSONObject hi = null;
        if (matcher != null) {
            ma = (BSONObject) JSON.parse(matcher);
        }
        if (hint != null) {
            hi = (BSONObject) JSON.parse(hint);
        }
        delete(ma, hi);
    }

    /**
     * Delete the matching BSONObject of current collection.
     *
     * @param matcher The matching condition, delete all the documents if null
     * @param hint    Specified the index used to scan data. e.g. {"":"ageIndex"} means
     *                using index "ageIndex" to scan data(index scan);
     *                {"":null} means table scan. when hint is null,
     *                database automatically match the optimal index to scan data.
     * @throws BaseException If error happens.
     */
    public void delete(BSONObject matcher, BSONObject hint)
            throws BaseException {
        DeleteRequest request = new DeleteRequest(collectionFullName, matcher, hint);
        SdbReply response = sequoiadb.requestAndResponse(request);
        if (response.getFlag() != 0) {
            String msg = "matcher = " + matcher + ", hint = " + hint;
            sequoiadb.throwIfError(response, msg);
        }
        sequoiadb.upsertCache(collectionFullName);
    }

    /**
     * Update the document of current collection.
     * It won't work to update the ShardingKey field, but the other fields take effect.
     *
     * @param query DBQuery with matching condition, updating rule and hint
     * @throws BaseException If error happens.
     */
    public void update(DBQuery query) throws BaseException {
        _update(query.getFlag(), query.getMatcher(), query.getModifier(), query.getHint());
    }

    /**
     * Update the BSONObject of current collection.
     * It won't work to update the ShardingKey field, but the other fields take effect.
     *
     * @param matcher  The matching condition, update all the documents if null
     * @param modifier The updating rule, can't be null
     * @param hint     Specified the index used to scan data. e.g. {"":"ageIndex"} means
     *                 using index "ageIndex" to scan data(index scan);
     *                 {"":null} means table scan. when hint is null,
     *                 database automatically match the optimal index to scan data.
     * @throws BaseException If error happens.
     */
    public void update(BSONObject matcher, BSONObject modifier, BSONObject hint)
            throws BaseException {
        _update(0, matcher, modifier, hint);
    }

    /**
     * Update the BSONObject of current collection.
     * When flag is set to 0, it won't work to update the ShardingKey field, but the other fields take effect.
     *
     * @param matcher  The matching condition, update all the documents if null
     * @param modifier The updating rule, can't be null
     * @param hint     Specified the index used to scan data. e.g. {"":"ageIndex"} means
     *                 using index "ageIndex" to scan data(index scan);
     *                 {"":null} means table scan. when hint is null,
     *                 database automatically match the optimal index to scan data.
     * @param flag     the update flag, default to be 0. Please see the definition
     *                 of follow flags for more detail.
     *                 <ul>
     *                 <li>DBCollection.FLG_UPDATE_KEEP_SHARDINGKEY
     *                 </ul>
     * @throws BaseException If error happens.
     */
    public void update(BSONObject matcher, BSONObject modifier, BSONObject hint,
                       int flag) throws BaseException {
        _update(flag, matcher, modifier, hint);
    }

    /**
     * Update the BSONObject of current collection.
     * It won't work to update the ShardingKey field, but the other fields take effect.
     *
     * @param matcher  The matching condition, update all the documents if null
     * @param modifier The updating rule, can't be null or empty
     * @param hint     Specified the index used to scan data. e.g. {"":"ageIndex"} means
     *                 using index "ageIndex" to scan data(index scan);
     *                 {"":null} means table scan. when hint is null,
     *                 database automatically match the optimal index to scan data.
     * @throws BaseException If error happens.
     */
    public void update(String matcher, String modifier, String hint)
            throws BaseException {
        BSONObject ma = null;
        BSONObject mo = null;
        BSONObject hi = null;
        if (matcher != null) {
            ma = (BSONObject) JSON.parse(matcher);
        }
        if (modifier != null) {
            mo = (BSONObject) JSON.parse(modifier);
        }
        if (hint != null) {
            hi = (BSONObject) JSON.parse(hint);
        }
        _update(0, ma, mo, hi);
    }

    /**
     * Update the BSONObject of current collection.
     * When flag is set to 0, it won't work to update the ShardingKey field, but the other fields take effect.
     *
     * @param matcher  The matching condition, update all the documents if null
     * @param modifier The updating rule, can't be null or empty
     * @param hint     Specified the index used to scan data. e.g. {"":"ageIndex"} means
     *                 using index "ageIndex" to scan data(index scan);
     *                 {"":null} means table scan. when hint is null,
     *                 database automatically match the optimal index to scan data.
     * @param flag     the update flag, default to be 0. Please see the definition
     *                 of follow flags for more detail.
     *                 <ul>
     *                 <li>DBCollection.FLG_UPDATE_KEEP_SHARDINGKEY
     *                 </ul>
     * @throws BaseException If error happens.
     */
    public void update(String matcher, String modifier, String hint, int flag)
            throws BaseException {
        BSONObject ma = null;
        BSONObject mo = null;
        BSONObject hi = null;
        if (matcher != null) {
            ma = (BSONObject) JSON.parse(matcher);
        }
        if (modifier != null) {
            mo = (BSONObject) JSON.parse(modifier);
        }
        if (hint != null) {
            hi = (BSONObject) JSON.parse(hint);
        }
        _update(flag, ma, mo, hi);
    }

    /**
     * Update the BSONObject of current collection, insert if no matching.
     * It won't work to update the ShardingKey field, but the other fields take effect.
     *
     * @param matcher  The matching condition, update all the documents
     *                 if null(that's to say, we match all the documents)
     * @param modifier The updating rule, can't be null
     * @param hint     Specified the index used to scan data. e.g. {"":"ageIndex"} means
     *                 using index "ageIndex" to scan data(index scan);
     *                 {"":null} means table scan. when hint is null,
     *                 database automatically match the optimal index to scan data.
     * @throws BaseException If error happens.
     */
    public void upsert(BSONObject matcher, BSONObject modifier, BSONObject hint)
            throws BaseException {
        _update(SdbConstants.FLG_UPDATE_UPSERT, matcher, modifier, hint);
    }

    /**
     * Update the BSONObject of current collection, insert if no matching.
     * It won't work to update the ShardingKey field, but the other fields take effect.
     *
     * @param matcher     The matching condition, update all the documents
     *                    if null(that's to say, we match all the documents)
     * @param modifier    The updating rule, can't be null
     * @param hint        Specified the index used to scan data. e.g. {"":"ageIndex"} means
     *                    using index "ageIndex" to scan data(index scan);
     *                    {"":null} means table scan. when hint is null,
     *                    database automatically match the optimal index to scan data.
     * @param setOnInsert The setOnInsert assigns the specified values to the fileds when insert
     * @throws BaseException If error happens.
     */
    public void upsert(BSONObject matcher, BSONObject modifier, BSONObject hint,
                       BSONObject setOnInsert) throws BaseException {
        upsert(matcher, modifier, hint, setOnInsert, 0);
    }

    /**
     * Update the BSONObject of current collection, insert if no matching.
     * When flag is set to 0, it won't work to update the ShardingKey field, but the other fields take effect.
     *
     * @param matcher     The matching condition, update all the documents
     *                    if null(that's to say, we match all the documents)
     * @param modifier    The updating rule, can't be null
     * @param hint        Specified the index used to scan data. e.g. {"":"ageIndex"} means
     *                    using index "ageIndex" to scan data(index scan);
     *                    {"":null} means table scan. when hint is null,
     *                    database automatically match the optimal index to scan data.
     * @param setOnInsert The setOnInsert assigns the specified values to the fileds when insert
     * @param flag        the upsert flag, default to be 0. Please see the definition
     *                    of follow flags for more detail.
     *                    <ul>
     *                    <li>DBCollection.FLG_UPDATE_KEEP_SHARDINGKEY
     *                    </ul>
     * @throws BaseException If error happens.
     */
    public void upsert(BSONObject matcher, BSONObject modifier, BSONObject hint,
                       BSONObject setOnInsert, int flag) throws BaseException {
        BSONObject newHint;
        if (setOnInsert != null) {
            newHint = new BasicBSONObject();
            if (hint != null) {
                newHint.putAll(hint);
            }
            newHint.put(SdbConstants.FIELD_NAME_SET_ON_INSERT, setOnInsert);
        } else {
            newHint = hint;
        }
        flag |= SdbConstants.FLG_UPDATE_UPSERT;
        _update(flag, matcher, modifier, newHint);
    }

    /**
     * Explain query of current collection.
     *
     * @param matcher    the matching rule, return all the documents if null
     * @param selector   the selective rule, return the whole document if null
     * @param orderBy    the ordered rule, never sort if null
     * @param hint       Specified the index used to scan data. e.g. {"":"ageIndex"} means
     *                   using index "ageIndex" to scan data(index scan);
     *                   {"":null} means table scan. when hint is null,
     *                   database automatically match the optimal index to scan data.
     * @param skipRows   skip the first numToSkip documents, never skip if this parameter is 0
     * @param returnRows return the specified amount of documents,
     *                   when returnRows is 0, return nothing,
     *                   when returnRows is -1, return all the documents
     * @param flag       the query flag, default to be 0. Please see the definition
     *                   of follow flags for more detail. Usage:
     *                   e.g. set ( DBQuery.FLG_QUERY_FORCE_HINT | DBQuery.FLG_QUERY_WITH_RETURNDATA ) to param flag
     *                   <ul>
     *                   <li>DBQuery.FLG_QUERY_STRINGOUT
     *                   <li>DBQuery.FLG_QUERY_FORCE_HINT
     *                   <li>DBQuery.FLG_QUERY_PARALLED
     *                   <li>DBQuery.FLG_QUERY_WITH_RETURNDATA
     *                   </ul>
     * @param options    The rules of query explain, the options are as below:
     *                   <ul>
     *                   <li>Run     : Whether execute query explain or not, true for executing query explain then get
     *                   the data and time information; false for not executing query explain but get the
     *                   query explain information only. e.g. {Run:true}
     *                   </ul>
     * @return a DBCursor instance of the result
     * @throws BaseException If error happens.
     */
    public DBCursor explain(BSONObject matcher, BSONObject selector,
                            BSONObject orderBy, BSONObject hint, long skipRows, long returnRows,
                            int flag, BSONObject options) throws BaseException {

        flag |= DBQuery.FLG_QUERY_EXPLAIN;
        BSONObject innerHint = new BasicBSONObject();
        if (null != hint) {
            innerHint.put(SdbConstants.FIELD_NAME_HINT, hint);
        }

        if (null != options) {
            innerHint.put(SdbConstants.FIELD_NAME_OPTIONS, options);
        }

        return query(matcher, selector, orderBy, innerHint, skipRows,
                returnRows, flag);
    }

    /**
     * Get all documents of current collection.
     *
     * @return a DBCursor instance of the result
     * @throws BaseException If error happens.
     */
    public DBCursor query() throws BaseException {
        return query("", "", "", "", 0, -1);
    }

    /**
     * Get the matching documents in current collection.
     *
     * @param matcher the matching rule, return all the documents if null
     * @return a DBCursor instance of the result or null if no any matched document
     * @throws BaseException If error happens.
     * @see com.sequoiadb.base.DBQuery
     */
    public DBCursor query(DBQuery matcher) throws BaseException {
        if (matcher == null) {
            return query();
        }
        return query(matcher.getMatcher(), matcher.getSelector(),
                matcher.getOrderBy(), matcher.getHint(), matcher.getSkipRowsCount(),
                matcher.getReturnRowsCount(), matcher.getFlag());
    }

    /**
     * Get the matching documents in current collection.
     *
     * @param matcher  the matching rule, return all the documents if null
     * @param selector the selective rule, return the whole document if null
     * @param orderBy  the ordered rule, never sort if null
     * @param hint     Specified the index used to scan data. e.g. {"":"ageIndex"} means
     *                 using index "ageIndex" to scan data(index scan);
     *                 {"":null} means table scan. when hint is null,
     *                 database automatically match the optimal index to scan data.
     * @return a DBCursor instance of the result or null if no any matched document
     * @throws BaseException If error happens.
     */
    public DBCursor query(BSONObject matcher, BSONObject selector,
                          BSONObject orderBy, BSONObject hint) throws BaseException {
        return query(matcher, selector, orderBy, hint, 0, -1, 0);
    }

    /**
     * Get the matching documents in current collection.
     *
     * @param matcher  the matching rule, return all the documents if null
     * @param selector the selective rule, return the whole document if null
     * @param orderBy  the ordered rule, never sort if null
     * @param hint     Specified the index used to scan data. e.g. {"":"ageIndex"} means
     *                 using index "ageIndex" to scan data(index scan);
     *                 {"":null} means table scan. when hint is null,
     *                 database automatically match the optimal index to scan data.
     * @param flag     the query flag, default to be 0. Please see the definition
     *                 of follow flags for more detail. Usage:
     *                 e.g. set ( DBQuery.FLG_QUERY_FORCE_HINT | DBQuery.FLG_QUERY_WITH_RETURNDATA ) to param flag
     *                 <ul>
     *                 <li>DBQuery.FLG_QUERY_STRINGOUT
     *                 <li>DBQuery.FLG_QUERY_FORCE_HINT
     *                 <li>DBQuery.FLG_QUERY_PARALLED
     *                 <li>DBQuery.FLG_QUERY_WITH_RETURNDATA
     *                 </ul>
     * @return a DBCursor instance of the result or null if no any matched document
     * @throws BaseException If error happens.
     */
    public DBCursor query(BSONObject matcher, BSONObject selector,
                          BSONObject orderBy, BSONObject hint, int flag) throws BaseException {
        return query(matcher, selector, orderBy, hint, 0, -1, flag);
    }

    /**
     * Get the matching documents in current collection.
     *
     * @param matcher  the matching rule, return all the documents if null
     * @param selector the selective rule, return the whole document if null
     * @param orderBy  the ordered rule, never sort if null
     * @param hint     Specified the index used to scan data. e.g. {"":"ageIndex"} means
     *                 using index "ageIndex" to scan data(index scan);
     *                 {"":null} means table scan. when hint is null,
     *                 database automatically match the optimal index to scan data.
     * @return a DBCursor instance of the result or null if no any matched document
     * @throws BaseException If error happens.
     */
    public DBCursor query(String matcher, String selector, String orderBy,
                          String hint) throws BaseException {
        return query(matcher, selector, orderBy, hint, 0);
    }

    /**
     * Get the matching documents in current collection.
     *
     * @param matcher  the matching rule, return all the documents if null
     * @param selector the selective rule, return the whole document if null
     * @param orderBy  the ordered rule, never sort if null
     * @param hint     Specified the index used to scan data. e.g. {"":"ageIndex"} means
     *                 using index "ageIndex" to scan data(index scan);
     *                 {"":null} means table scan. when hint is null,
     *                 database automatically match the optimal index to scan data.
     * @param flag     the query flag, default to be 0. Please see the definition
     *                 of follow flags for more detail. Usage:
     *                 e.g. set ( DBQuery.FLG_QUERY_FORCE_HINT | DBQuery.FLG_QUERY_WITH_RETURNDATA ) to param flag
     *                 <ul>
     *                 <li>DBQuery.FLG_QUERY_STRINGOUT
     *                 <li>DBQuery.FLG_QUERY_FORCE_HINT
     *                 <li>DBQuery.FLG_QUERY_PARALLED
     *                 <li>DBQuery.FLG_QUERY_WITH_RETURNDATA
     *                 </ul>
     * @return a DBCursor instance of the result or null if no any matched document
     * @throws BaseException If error happens.
     */
    public DBCursor query(String matcher, String selector, String orderBy,
                          String hint, int flag) throws BaseException {
        BSONObject ma = null;
        BSONObject se = null;
        BSONObject or = null;
        BSONObject hi = null;
        if (matcher != null) {
            ma = (BSONObject) JSON.parse(matcher);
        }
        if (selector != null) {
            se = (BSONObject) JSON.parse(selector);
        }
        if (orderBy != null && !orderBy.equals("")) {
            or = (BSONObject) JSON.parse(orderBy);
        }
        if (hint != null) {
            hi = (BSONObject) JSON.parse(hint);
        }
        return query(ma, se, or, hi, 0, -1, flag);
    }

    /**
     * Get the matching documents in current collection.
     *
     * @param matcher    the matching rule, return all the documents if null
     * @param selector   the selective rule, return the whole document if null
     * @param orderBy    the ordered rule, never sort if null
     * @param hint       Specified the index used to scan data. e.g. {"":"ageIndex"} means
     *                   using index "ageIndex" to scan data(index scan);
     *                   {"":null} means table scan. when hint is null,
     *                   database automatically match the optimal index to scan data.
     * @param skipRows   skip the first numToSkip documents, never skip if this parameter is 0
     * @param returnRows return the specified amount of documents,
     *                   when returnRows is 0, return nothing,
     *                   when returnRows is -1, return all the documents
     * @return a DBCursor instance of the result or null if no any matched document
     * @throws BaseException If error happens.
     */
    public DBCursor query(String matcher, String selector, String orderBy,
                          String hint, long skipRows, long returnRows) throws BaseException {
        BSONObject ma = null;
        BSONObject se = null;
        BSONObject or = null;
        BSONObject hi = null;
        if (matcher != null) {
            ma = (BSONObject) JSON.parse(matcher);
        }
        if (selector != null) {
            se = (BSONObject) JSON.parse(selector);
        }
        if (orderBy != null) {
            or = (BSONObject) JSON.parse(orderBy);
        }
        if (hint != null) {
            hi = (BSONObject) JSON.parse(hint);
        }
        return query(ma, se, or, hi, skipRows, returnRows, 0);
    }

    /**
     * Get the matching documents in current collection.
     *
     * @param matcher    the matching rule, return all the documents if null
     * @param selector   the selective rule, return the whole document if null
     * @param orderBy    the ordered rule, never sort if null
     * @param hint       Specified the index used to scan data. e.g. {"":"ageIndex"} means
     *                   using index "ageIndex" to scan data(index scan);
     *                   {"":null} means table scan. when hint is null,
     *                   database automatically match the optimal index to scan data.
     * @param skipRows   skip the first numToSkip documents, never skip if this parameter is 0
     * @param returnRows return the specified amount of documents,
     *                   when returnRows is 0, return nothing,
     *                   when returnRows is -1, return all the documents
     * @return a DBCursor instance of the result or null if no any matched document
     * @throws BaseException If error happens.
     */
    public DBCursor query(BSONObject matcher, BSONObject selector,
                          BSONObject orderBy, BSONObject hint, long skipRows, long returnRows) throws BaseException {
        return query(matcher, selector, orderBy, hint, skipRows, returnRows, 0);
    }

    /**
     * Get the matching documents in current collection.
     *
     * @param matcher    the matching rule, return all the documents if null
     * @param selector   the selective rule, return the whole document if null
     * @param orderBy    the ordered rule, never sort if null
     * @param hint       Specified the index used to scan data. e.g. {"":"ageIndex"} means
     *                   using index "ageIndex" to scan data(index scan);
     *                   {"":null} means table scan. when hint is null,
     *                   database automatically match the optimal index to scan data.
     * @param skipRows   skip the first numToSkip documents, never skip if this parameter is 0
     * @param returnRows return the specified amount of documents,
     *                   when returnRows is 0, return nothing,
     *                   when returnRows is -1, return all the documents
     * @param flags      the query flags, default to be 0. Please see the definition
     *                   of follow flags for more detail. Usage:
     *                   e.g. set ( DBQuery.FLG_QUERY_FORCE_HINT | DBQuery.FLG_QUERY_WITH_RETURNDATA ) to param flag
     *                   <ul>
     *                   <li>DBQuery.FLG_QUERY_STRINGOUT
     *                   <li>DBQuery.FLG_QUERY_FORCE_HINT
     *                   <li>DBQuery.FLG_QUERY_PARALLED
     *                   <li>DBQuery.FLG_QUERY_WITH_RETURNDATA
     *                   </ul>
     * @return a DBCursor instance of the result or null if no any matched document
     * @throws BaseException If error happens.
     */
    public DBCursor query(BSONObject matcher, BSONObject selector,
                          BSONObject orderBy, BSONObject hint,
                          long skipRows, long returnRows,
                          int flags) throws BaseException {
        int newFlags = DBQuery.regulateFlags(flags);

        if (returnRows < 0) {
            returnRows = -1;
        }
        if (returnRows == 1) {
            newFlags |= DBQuery.FLG_QUERY_WITH_RETURNDATA;
        }

        QueryRequest request = new QueryRequest(collectionFullName,
                matcher, selector, orderBy, hint,
                skipRows, returnRows, newFlags);
        SdbReply response = sequoiadb.requestAndResponse(request);

        int flag = response.getFlag();
        if (flag != 0) {
            if (flag == SDBError.SDB_DMS_EOC.getErrorCode()) {
                return null;
            } else {
                String msg = "matcher = " + matcher +
                        ", selector = " + selector +
                        ", orderBy = " + orderBy +
                        ", hint = " + hint +
                        ", skipRows = " + skipRows +
                        ", returnRows = " + returnRows;
                sequoiadb.throwIfError(response, msg);
            }
        }

        sequoiadb.upsertCache(collectionFullName);

        DBCursor cursor = new DBCursor(response, sequoiadb);
        return cursor;
    }

    /**
     * Get one matched document from current collection.
     *
     * @param matcher  the matching rule, return all the documents if null
     * @param selector the selective rule, return the whole document if null
     * @param orderBy  the ordered rule, never sort if null
     * @param hint     Specified the index used to scan data. e.g. {"":"ageIndex"} means
     *                 using index "ageIndex" to scan data(index scan);
     *                 {"":null} means table scan. when hint is null,
     *                 database automatically match the optimal index to scan data.
     * @param flag     the query flag, default to be 0. Please see the definition
     *                 of follow flags for more detail. Usage:
     *                 e.g. set ( DBQuery.FLG_QUERY_FORCE_HINT | DBQuery.FLG_QUERY_WITH_RETURNDATA ) to param flag
     *                 <ul>
     *                 <li>DBQuery.FLG_QUERY_STRINGOUT
     *                 <li>DBQuery.FLG_QUERY_FORCE_HINT
     *                 <li>DBQuery.FLG_QUERY_PARALLED
     *                 <li>DBQuery.FLG_QUERY_WITH_RETURNDATA
     *                 </ul>
     * @return the matched document or null if no such document
     * @throws BaseException If error happens.
     */
    public BSONObject queryOne(BSONObject matcher, BSONObject selector,
                               BSONObject orderBy, BSONObject hint,
                               int flag) throws BaseException {
        flag = flag | DBQuery.FLG_QUERY_WITH_RETURNDATA;
        DBCursor cursor = query(matcher, selector, orderBy, hint, 0, 1, flag);
        return cursor.getNext();
    }

    /**
     * Get one document from current collection.
     *
     * @return the document or null if no any document in current collection
     * @throws BaseException If error happens.
     */
    public BSONObject queryOne() throws BaseException {
        return queryOne(null, null, null, null, 0);
    }

    /**
     * Get all the indexes of current collection
     *
     * @return DBCursor of indexes
     * @throws BaseException If error happens.
     */
    public DBCursor getIndexes() throws BaseException {
        BSONObject obj = new BasicBSONObject();
        obj.put(SdbConstants.FIELD_COLLECTION, collectionFullName);

        AdminRequest request = new AdminRequest(AdminCommand.GET_INDEXES, null, obj);
        SdbReply response = sequoiadb.requestAndResponse(request);

        int flags = response.getFlag();
        if (flags != 0) {
            if (flags == SDBError.SDB_DMS_EOC.getErrorCode()) {
                return null;
            } else {
                sequoiadb.throwIfError(response);
            }
        }

        sequoiadb.upsertCache(collectionFullName);

        DBCursor cursor = new DBCursor(response, sequoiadb);
        return cursor;
    }

    private DBCursor _queryAndModify(BSONObject matcher, BSONObject selector,
                                     BSONObject orderBy, BSONObject hint, BSONObject update,
                                     long skipRows, long returnRows, int flag,
                                     boolean isUpdate, boolean returnNew)
            throws BaseException {
        BSONObject modify = new BasicBSONObject();

        if (isUpdate) {
            if (update == null || update.isEmpty()) {
                throw new BaseException(SDBError.SDB_INVALIDARG, "update can't be empty");
            }

            modify.put(SdbConstants.FIELD_NAME_OP,
                    SdbConstants.FIELD_OP_VALUE_UPDATE);
            modify.put(SdbConstants.FIELD_NAME_OP_UPDATE, update);
            modify.put(SdbConstants.FIELD_NAME_RETURNNEW, returnNew);
        } else {
            modify.put(SdbConstants.FIELD_NAME_OP,
                    SdbConstants.FIELD_OP_VALUE_REMOVE);
            modify.put(SdbConstants.FIELD_NAME_OP_REMOVE, true);
        }

        BSONObject newHint = new BasicBSONObject();
        if (hint != null) {
            newHint.putAll(hint);
        }
        newHint.put(SdbConstants.FIELD_NAME_MODIFY, modify);

        flag |= DBQuery.FLG_QUERY_MODIFY;
        return query(matcher, selector, orderBy, newHint,
                skipRows, returnRows, flag);
    }

    /**
     * Get the matching documents in current collection and update.
     * In order to make the update take effect, user must travel
     * the DBCursor returned by this function.
     *
     * @param matcher    the matching rule, return all the documents if null
     * @param selector   the selective rule, return the whole document if null
     * @param orderBy    the ordered rule, never sort if null
     * @param update     the update rule, can't be null
     * @param hint       Specified the index used to scan data. e.g. {"":"ageIndex"} means
     *                   using index "ageIndex" to scan data(index scan);
     *                   {"":null} means table scan. when hint is null,
     *                   database automatically match the optimal index to scan data.
     * @param skipRows   skip the first numToSkip documents, never skip if this parameter is 0
     * @param returnRows return the specified amount of documents,
     *                   when returnRows is 0, return nothing,
     *                   when returnRows is -1, return all the documents
     * @param flag       the query flags, default to be 0. Please see the definition
     *                   of follow flags for more detail. Usage:
     *                   e.g. set ( DBQuery.FLG_QUERY_FORCE_HINT | DBQuery.FLG_QUERY_WITH_RETURNDATA ) to param flag
     *                   <ul>
     *                   <li>DBQuery.FLG_QUERY_STRINGOUT
     *                   <li>DBQuery.FLG_QUERY_FORCE_HINT
     *                   <li>DBQuery.FLG_QUERY_PARALLED
     *                   <li>DBQuery.FLG_QUERY_WITH_RETURNDATA
     *                   <li>DBQuery.FLG_QUERY_KEEP_SHARDINGKEY_IN_UPDATE
     *                   </ul>
     * @param returnNew  When true, returns the updated document rather than the original
     * @return a DBCursor instance of the result or null if no any matched document
     * @throws BaseException If error happens.
     */
    public DBCursor queryAndUpdate(BSONObject matcher, BSONObject selector,
                                   BSONObject orderBy, BSONObject hint, BSONObject update,
                                   long skipRows, long returnRows, int flag, boolean returnNew)
            throws BaseException {
        return _queryAndModify(matcher, selector, orderBy, hint, update,
                skipRows, returnRows, flag, true, returnNew);
    }

    /**
     * Get the matching documents in current collection and remove.
     * In order to make the remove take effect, user must travel
     * the DBCursor returned by this function.
     *
     * @param matcher    the matching rule, return all the documents if null
     * @param selector   the selective rule, return the whole document if null
     * @param orderBy    the ordered rule, never sort if null
     * @param hint       Specified the index used to scan data. e.g. {"":"ageIndex"} means
     *                   using index "ageIndex" to scan data(index scan);
     *                   {"":null} means table scan. when hint is null,
     *                   database automatically match the optimal index to scan data.
     * @param skipRows   skip the first numToSkip documents, never skip if this parameter is 0
     * @param returnRows return the specified amount of documents,
     *                   when returnRows is 0, return nothing,
     *                   when returnRows is -1, return all the documents
     * @param flag       the query flag, default to be 0. Please see the definition
     *                   of follow flags for more detail. Usage:
     *                   e.g. set ( DBQuery.FLG_QUERY_FORCE_HINT | DBQuery.FLG_QUERY_WITH_RETURNDATA ) to param flag
     *                   <ul>
     *                   <li>DBQuery.FLG_QUERY_STRINGOUT
     *                   <li>DBQuery.FLG_QUERY_FORCE_HINT
     *                   <li>DBQuery.FLG_QUERY_PARALLED
     *                   <li>DBQuery.FLG_QUERY_WITH_RETURNDATA
     *                   </ul>
     * @return a DBCursor instance of the result or null if no any matched document
     * @throws BaseException If error happens.
     */
    public DBCursor queryAndRemove(BSONObject matcher, BSONObject selector,
                                   BSONObject orderBy, BSONObject hint,
                                   long skipRows, long returnRows, int flag)
            throws BaseException {
        return _queryAndModify(matcher, selector, orderBy, hint, null,
                skipRows, returnRows, flag, false, false);
    }

    /**
     * Get all of or one of the indexes in current collection.
     *
     * @param name The index name, returns all of the indexes if this parameter
     *             is null
     * @return DBCursor of indexes
     * @throws BaseException If error happens.
     */
    public DBCursor getIndex(String name) throws BaseException {
        if (name == null) {
            return getIndexes();
        }

        BSONObject condition = new BasicBSONObject();
        condition.put(SdbConstants.IXM_INDEXDEF + "."
                + SdbConstants.IXM_NAME, name);

        BSONObject obj = new BasicBSONObject();
        obj.put(SdbConstants.FIELD_COLLECTION, collectionFullName);

        AdminRequest request = new AdminRequest(AdminCommand.GET_INDEXES, condition, obj);
        SdbReply response = sequoiadb.requestAndResponse(request);

        int flags = response.getFlag();
        if (flags != 0) {
            if (flags == SDBError.SDB_DMS_EOC.getErrorCode()) {
                return null;
            } else {
                sequoiadb.throwIfError(response);
            }
        }
        sequoiadb.upsertCache(collectionFullName);
        return new DBCursor(response, sequoiadb);
    }

    /**
     * Create a index with name and key.
     *
     * @param name           The index name
     * @param key            The index keys in JSON format, like: "{\"a\":1, \"b\":-1}"
     * @param isUnique       Whether the index elements are unique or not
     * @param enforced       Whether the index is enforced unique This element is
     *                       meaningful when isUnique is set to true
     * @param sortBufferSize The size(MB) of sort buffer used when creating index,
     *                       zero means don't use sort buffer
     * @throws BaseException If error happens.
     */
    public void createIndex(String name, BSONObject key, boolean isUnique,
                            boolean enforced, int sortBufferSize) throws BaseException {
        if (sortBufferSize < 0) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "sortBufferSize less than 0");
        }

        BSONObject obj = new BasicBSONObject();
        obj.put(SdbConstants.IXM_KEY, key);
        obj.put(SdbConstants.IXM_NAME, name);
        obj.put(SdbConstants.IXM_UNIQUE, isUnique);
        obj.put(SdbConstants.IXM_ENFORCED, enforced);

        BSONObject createObj = new BasicBSONObject();
        createObj.put(SdbConstants.FIELD_COLLECTION, collectionFullName);
        createObj.put(SdbConstants.FIELD_INDEX, obj);

        BSONObject hint = new BasicBSONObject();
        hint.put(SdbConstants.IXM_FIELD_NAME_SORT_BUFFER_SIZE,
                sortBufferSize);

        AdminRequest request = new AdminRequest(AdminCommand.CREATE_INDEX, createObj, hint);
        SdbReply response = sequoiadb.requestAndResponse(request);

        if (response.getFlag() != 0) {
            String msg = "name = " + name +
                    ", key = " + key +
                    ", isUnique = " + isUnique;
            sequoiadb.throwIfError(response, msg);
        }

        sequoiadb.upsertCache(collectionFullName);
    }

    /**
     * Create a index with name and key.
     *
     * @param name           The index name
     * @param key            The index keys in JSON format, like: "{\"a\":1, \"b\":-1}"
     * @param isUnique       Whether the index elements are unique or not
     * @param enforced       Whether the index is enforced unique This element is
     *                       meaningful when isUnique is set to true
     * @param sortBufferSize The size(MB) of sort buffer used when creating index,
     *                       zero means don't use sort buffer
     * @throws BaseException If error happens.
     */
    public void createIndex(String name, String key, boolean isUnique,
                            boolean enforced, int sortBufferSize) throws BaseException {
        BSONObject k = null;
        if (key != null) {
            k = (BSONObject) JSON.parse(key);
        }
        createIndex(name, k, isUnique, enforced, sortBufferSize);
    }

    /**
     * Create a index with name and key
     *
     * @param name     The index name
     * @param key      The index keys in JSON format, like: "{\"a\":1, \"b\":-1}"
     * @param isUnique Whether the index elements are unique or not
     * @param enforced Whether the index is enforced unique This element is
     *                 meaningful when isUnique is set to true
     * @throws BaseException If error happens.
     */
    public void createIndex(String name, BSONObject key, boolean isUnique,
                            boolean enforced) throws BaseException {
        createIndex(name, key, isUnique, enforced,
                SdbConstants.IXM_SORT_BUFFER_DEFAULT_SIZE);
    }

    /**
     * Create a index with name and key.
     *
     * @param name     The index name
     * @param key      The index keys in JSON format, like: "{\"a\":1, \"b\":-1}"
     * @param isUnique Whether the index elements are unique or not
     * @param enforced Whether the index is enforced unique This element is
     *                 meaningful when isUnique is set to true
     * @throws BaseException If error happens.
     */
    public void createIndex(String name, String key, boolean isUnique,
                            boolean enforced) throws BaseException {
        BSONObject k = null;
        if (key != null) {
            k = (BSONObject) JSON.parse(key);
        }
        createIndex(name, k, isUnique, enforced,
                SdbConstants.IXM_SORT_BUFFER_DEFAULT_SIZE);
    }

    /**
     * Create the id index.
     *
     * @param options can be empty or specify option. e.g. {SortBufferSize:64}
     * @throws BaseException If error happens.
     */
    public void createIdIndex(BSONObject options) throws BaseException {
        BSONObject tmp = new BasicBSONObject();
        tmp.put(SdbConstants.FIELD_NAME_NAME,
                SdbConstants.SDB_ALTER_CRT_ID_INDEX);
        if (options == null || options.isEmpty()) {
            tmp.put(SdbConstants.FIELD_NAME_ARGS, null);
        } else {
            tmp.put(SdbConstants.FIELD_NAME_ARGS, options);
        }


        BSONObject innerOptions = new BasicBSONObject();
        innerOptions.put(SdbConstants.FIELD_NAME_ALTER, tmp);
        alterCollection(innerOptions);
    }

    /**
     * Drop the id index.
     *
     * @throws BaseException If error happens.
     */
    public void dropIdIndex() throws BaseException {
        BSONObject tmp = new BasicBSONObject();
        tmp.put(SdbConstants.FIELD_NAME_NAME,
                SdbConstants.SDB_ALTER_DROP_ID_INDEX);
        tmp.put(SdbConstants.FIELD_NAME_ARGS, null);

        BSONObject options = new BasicBSONObject();
        options.put(SdbConstants.FIELD_NAME_ALTER, tmp);
        alterCollection(options);
    }

    /**
     * Remove the named index of current collection.
     *
     * @param name The index name
     * @throws BaseException If error happens.
     */
    public void dropIndex(String name) throws BaseException {
        BSONObject index = new BasicBSONObject();
        index.put("", name);

        BSONObject dropObj = new BasicBSONObject();
        dropObj.put(SdbConstants.FIELD_COLLECTION, collectionFullName);
        dropObj.put(SdbConstants.FIELD_INDEX, index);

        AdminRequest request = new AdminRequest(AdminCommand.DROP_INDEX, dropObj);
        SdbReply response = sequoiadb.requestAndResponse(request);
        sequoiadb.throwIfError(response, name);
        sequoiadb.upsertCache(collectionFullName);
    }

    /**
     * Get the amount of documents in current collection.
     *
     * @return the amount of matching documents
     * @throws BaseException If error happens.
     */
    public long getCount() throws BaseException {
        return getCount("");
    }

    /**
     * Get the amount of matching documents in current collection.
     *
     * @param matcher the matching rule
     * @return the amount of matching documents
     * @throws BaseException If error happens.
     */
    public long getCount(String matcher) throws BaseException {
        BSONObject con = null;
        if (matcher != null) {
            con = (BSONObject) JSON.parse(matcher);
        }
        return getCount(con);
    }

    /**
     * Get the amount of matching documents in current collection.
     *
     * @param matcher The matching rule, when condition is null, the return amount contains all the records.
     * @return the amount of matching documents
     * @throws BaseException If error happens.
     */
    public long getCount(BSONObject matcher) throws BaseException {
        return getCount(matcher, null);
    }

    /**
     * Get the count of matching BSONObject in current collection.
     *
     * @param matcher The matching rule, when condition is null, the return amount contains all the records.
     * @param hint    Specified the index used to scan data. e.g. {"":"ageIndex"} means
     *                using index "ageIndex" to scan data(index scan);
     *                {"":null} means table scan. when hint is null,
     *                database automatically match the optimal index to scan data.
     * @return The count of matching BSONObjects
     * @throws BaseException If error happens.
     */
    public long getCount(BSONObject matcher, BSONObject hint) throws BaseException {
        BSONObject newHint = new BasicBSONObject();
        newHint.put(SdbConstants.FIELD_COLLECTION, collectionFullName);
        if (null != hint) {
            newHint.put(SdbConstants.FIELD_NAME_HINT, hint);
        }

        AdminRequest request = new AdminRequest(AdminCommand.GET_COUNT, matcher, newHint);
        SdbReply response = sequoiadb.requestAndResponse(request);

        if (response.getFlag() != 0) {
            String msg = "condition = " + matcher +
                    ", hint = " + hint;
            sequoiadb.throwIfError(response, msg);
        }

        sequoiadb.upsertCache(collectionFullName);

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
     * Split the specified collection from source group to target group by range.
     *
     * @param sourceGroupName   the source group name
     * @param destGroupName     the destination group name
     * @param splitCondition    the split condition
     * @param splitEndCondition the split end condition or null, only usable when "ShardingType" is "range".
     *                          eg:If we create a collection with the option {ShardingKey:{"age":1},ShardingType:"range"},
     *                          we can fill {age:30} as the splitCondition, and fill {age:60} as the splitEndCondition. when split,
     *                          the target group will get the records whose age's hash value are in [30,60). If splitEndCondition is null,
     *                          they are in [30,max).
     * @throws BaseException If error happens.
     */
    public void split(String sourceGroupName, String destGroupName,
                      BSONObject splitCondition, BSONObject splitEndCondition) throws BaseException {
        if ((null == sourceGroupName || sourceGroupName.equals("")) ||
                (null == destGroupName || destGroupName.equals("")) ||
                null == splitCondition) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "null parameter");
        }

        BSONObject obj = new BasicBSONObject();
        obj.put(SdbConstants.FIELD_NAME_NAME, collectionFullName);
        obj.put(SdbConstants.FIELD_NAME_SOURCE, sourceGroupName);
        obj.put(SdbConstants.FIELD_NAME_TARGET, destGroupName);
        obj.put(SdbConstants.FIELD_NAME_SPLITQUERY, splitCondition);
        if (null != splitEndCondition) {
            obj.put(SdbConstants.FIELD_NAME_SPLITENDQUERY, splitEndCondition);
        }

        AdminRequest request = new AdminRequest(AdminCommand.SPLIT, obj);
        SdbReply response = sequoiadb.requestAndResponse(request);

        if (response.getFlag() != 0) {
            String msg = "sourceGroupName = " + sourceGroupName +
                    ", destGroupName = " + destGroupName +
                    ", splitCondition = " + splitCondition +
                    ", splitEndCondition = " + splitEndCondition;
            sequoiadb.throwIfError(response, msg);
        }

        sequoiadb.upsertCache(collectionFullName);
    }

    /**
     * Split the specified collection from source group to target group by percent.
     *
     * @param sourceGroupName the source group name
     * @param destGroupName   the destination group name
     * @param percent         the split percent, Range:(0,100]
     * @throws BaseException If error happens.
     */
    public void split(String sourceGroupName, String destGroupName,
                      double percent) throws BaseException {
        if ((null == sourceGroupName || sourceGroupName.equals("")) ||
                (null == destGroupName || destGroupName.equals("")) ||
                (percent <= 0.0 || percent > 100.0)) {
            throw new BaseException(SDBError.SDB_INVALIDARG);
        }

        BSONObject obj = new BasicBSONObject();
        obj.put(SdbConstants.FIELD_NAME_NAME, collectionFullName);
        obj.put(SdbConstants.FIELD_NAME_SOURCE, sourceGroupName);
        obj.put(SdbConstants.FIELD_NAME_TARGET, destGroupName);
        obj.put(SdbConstants.FIELD_NAME_SPLITPERCENT, percent);

        AdminRequest request = new AdminRequest(AdminCommand.SPLIT, obj);
        SdbReply response = sequoiadb.requestAndResponse(request);

        if (response.getFlag() != 0) {
            String msg = "sourceGroupName = " + sourceGroupName +
                    ", destGroupName = " + destGroupName +
                    ", percent = " + percent;
            sequoiadb.throwIfError(response, msg);
        }

        sequoiadb.upsertCache(collectionFullName);
    }

    /**
     * Split the specified collection from source group to target group by range asynchronously.
     *
     * @param sourceGroupName   the source group name
     * @param destGroupName     the destination group name
     * @param splitCondition    the split condition
     * @param splitEndCondition the split end condition or null, only usable when "ShardingType" is "range".
     *                          eg:If we create a collection with the option {ShardingKey:{"age":1},ShardingType:"range"},
     *                          we can fill {age:30} as the splitCondition, and fill {age:60} as the splitEndCondition. when split,
     *                          the target group will get the records whose age's hash values are in [30,60). If splitEndCondition is null,
     *                          they are in [30,max).
     * @return return the task id, we can use the return id to manage the sharding which is run background.
     * @throws BaseException If error happens.
     * @see Sequoiadb#listTasks(BSONObject, BSONObject, BSONObject, BSONObject)
     * @see Sequoiadb#cancelTask(long, boolean)
     */
    public long splitAsync(String sourceGroupName,
                           String destGroupName,
                           BSONObject splitCondition,
                           BSONObject splitEndCondition) throws BaseException {
        if ((null == sourceGroupName || sourceGroupName.equals("")) ||
                (null == destGroupName || destGroupName.equals("")) ||
                null == splitCondition) {
            throw new BaseException(SDBError.SDB_INVALIDARG);
        }

        BSONObject obj = new BasicBSONObject();
        obj.put(SdbConstants.FIELD_NAME_NAME, collectionFullName);
        obj.put(SdbConstants.FIELD_NAME_ASYNC, true);
        obj.put(SdbConstants.FIELD_NAME_SOURCE, sourceGroupName);
        obj.put(SdbConstants.FIELD_NAME_TARGET, destGroupName);
        obj.put(SdbConstants.FIELD_NAME_SPLITQUERY, splitCondition);
        if (null != splitEndCondition) {
            obj.put(SdbConstants.FIELD_NAME_SPLITENDQUERY, splitEndCondition);
        }

        AdminRequest request = new AdminRequest(AdminCommand.SPLIT, obj);
        SdbReply response = sequoiadb.requestAndResponse(request);

        if (response.getFlag() != 0) {
            String msg = "sourceGroupName = " + sourceGroupName +
                    ", destGroupName = " + destGroupName +
                    ", splitCondition = " + splitCondition +
                    ", splitEndCondition = " + splitEndCondition;
            sequoiadb.throwIfError(response, msg);
        }
        BSONObject result;
        DBCursor cursor = new DBCursor(response, sequoiadb);
        try {
            if (!cursor.hasNext()) {
                throw new BaseException(SDBError.SDB_CAT_TASK_NOTFOUND);
            }
            result = cursor.getNext();
        } finally {
            cursor.close();
        }
        boolean flag = result.containsField(SdbConstants.FIELD_NAME_TASKID);
        if (!flag) {
            throw new BaseException(SDBError.SDB_CAT_TASK_NOTFOUND);
        }

        sequoiadb.upsertCache(collectionFullName);
        long taskid = (Long) result.get(SdbConstants.FIELD_NAME_TASKID);
        return taskid;
    }

    /**
     * Split the specified collection from source group to target group by percent asynchronously.
     *
     * @param sourceGroupName the source group name
     * @param destGroupName   the destination group name
     * @param percent         the split percent, Range:(0,100]
     * @return return the task id, we can use the return id to manage the sharding which is run background.
     * @throws BaseException If error happens.
     */
    public long splitAsync(String sourceGroupName, String destGroupName,
                           double percent) throws BaseException {
        if ((null == sourceGroupName || sourceGroupName.equals("")) ||
                (null == destGroupName || destGroupName.equals("")) ||
                (percent <= 0.0 || percent > 100.0)) {
            throw new BaseException(SDBError.SDB_INVALIDARG);
        }

        BSONObject obj = new BasicBSONObject();
        obj.put(SdbConstants.FIELD_NAME_NAME, collectionFullName);
        obj.put(SdbConstants.FIELD_NAME_ASYNC, true);
        obj.put(SdbConstants.FIELD_NAME_SOURCE, sourceGroupName);
        obj.put(SdbConstants.FIELD_NAME_TARGET, destGroupName);
        obj.put(SdbConstants.FIELD_NAME_SPLITPERCENT, percent);

        AdminRequest request = new AdminRequest(AdminCommand.SPLIT, obj);
        SdbReply response = sequoiadb.requestAndResponse(request);

        if (response.getFlag() != 0) {
            String msg = "sourceGroupName = " + sourceGroupName +
                    ", destGroupName = " + destGroupName +
                    ", percent = " + percent;
            sequoiadb.throwIfError(response, msg);
        }
        BSONObject result;
        DBCursor cursor = new DBCursor(response, sequoiadb);
        try {
            if (!cursor.hasNext()) {
                throw new BaseException(SDBError.SDB_CAT_TASK_NOTFOUND);
            }
            result = cursor.getNext();
        } finally {
            cursor.close();
        }
        boolean flag = result.containsField(SdbConstants.FIELD_NAME_TASKID);
        if (!flag) {
            throw new BaseException(SDBError.SDB_CAT_TASK_NOTFOUND);
        }

        sequoiadb.upsertCache(collectionFullName);

        long taskid = (Long) result.get(SdbConstants.FIELD_NAME_TASKID);
        return taskid;
    }

    /**
     * Execute aggregate operation in current collection.
     *
     * @param objs The Bson object of rule list, can't be null
     * @throws BaseException If error happens.
     */
    public DBCursor aggregate(List<BSONObject> objs)
            throws BaseException {
        if (objs == null || objs.size() == 0) {
            throw new BaseException(SDBError.SDB_INVALIDARG);
        }

        AggregateRequest request = new AggregateRequest(collectionFullName, objs);
        SdbReply response = sequoiadb.requestAndResponse(request);

        int flags = response.getFlag();
        if (flags != 0) {
            if (flags == SDBError.SDB_DMS_EOC.getErrorCode()) {
                return null;
            } else {
                sequoiadb.throwIfError(response, objs);
            }
        }

        sequoiadb.upsertCache(collectionFullName);

        DBCursor cursor = new DBCursor(response, sequoiadb);
        return cursor;
    }

    /**
     * Get index blocks' or data blocks' information for concurrent query.
     *
     * @param matcher    the matching rule, return all the meta information if null
     * @param orderBy    the ordered rule, never sort if null
     * @param hint       Specified the index used to scan data. e.g. {"":"ageIndex"} means
     *                   using index "ageIndex" to scan data(index scan);
     *                   {"":null} means table scan. when hint is null,
     *                   database automatically match the optimal index to scan data.
     * @param skipRows   The rows to be skipped
     * @param returnRows return the specified amount of documents,
     *                   when returnRows is 0, return nothing,
     *                   when returnRows is -1, return all the documents
     * @param flag       The flag to use which form for record data
     *                   0: bson stream
     *                   1: binary data stream, form: col1|col2|col3
     * @return DBCursor of data
     * @throws BaseException If error happens.
     */
    public DBCursor getQueryMeta(BSONObject matcher, BSONObject orderBy,
                                 BSONObject hint, long skipRows,
                                 long returnRows, int flag) throws BaseException {
        BSONObject newHint = new BasicBSONObject();
        newHint.put("Collection", this.collectionFullName);
        if (null == hint || hint.isEmpty()) {
            BSONObject empty = new BasicBSONObject();
            newHint.put("Hint", empty);
        } else {
            newHint.put("Hint", hint);
        }

        QueryRequest request = new QueryRequest(AdminCommand.GET_QUERYMETA, matcher, null, orderBy, newHint,
                skipRows, returnRows, flag);
        SdbReply response = sequoiadb.requestAndResponse(request);

        int flags = response.getFlag();
        if (flags != 0) {
            if (flags == SDBError.SDB_DMS_EOC.getErrorCode()) {
                return null;
            } else {
                String msg = "query = " + matcher +
                        ", hint = " + hint +
                        ", orderBy = " + orderBy +
                        ", skipRows = " + skipRows +
                        ", returnRows = " + returnRows;
                sequoiadb.throwIfError(response, msg);
            }
        }

        sequoiadb.upsertCache(collectionFullName);

        DBCursor cursor = new DBCursor(response, sequoiadb);
        return cursor;
    }

    /**
     * Attach the specified collection.
     *
     * @param subClFullName The full name of the sub-collection
     * @param options       The low boundary and up boundary
     *                      eg: {"LowBound":{a:1},"UpBound":{a:100}}
     * @throws BaseException If error happens.
     */
    public void attachCollection(String subClFullName, BSONObject options) throws BaseException {
        if (null == subClFullName || subClFullName.equals("") ||
                null == options ||
                null == collectionFullName || collectionFullName.equals("")) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "sub collection name or options is empty or null");
        }

        BSONObject newOptions = new BasicBSONObject();
        newOptions.put(SdbConstants.FIELD_NAME_NAME, collectionFullName);
        newOptions.put(SdbConstants.FIELD_NAME_SUBCLNAME, subClFullName);
        newOptions.putAll(options);

        AdminRequest request = new AdminRequest(AdminCommand.ATTACH_CL, newOptions);
        SdbReply response = sequoiadb.requestAndResponse(request);

        if (response.getFlag() != 0) {
            String msg = "subCollectionName = " + subClFullName +
                    ", options = " + options;
            sequoiadb.throwIfError(response, msg);
        }

        sequoiadb.upsertCache(collectionFullName);
    }

    /**
     * Detach the specified collection.
     *
     * @param subClFullName The full name of the sub-collection
     * @throws BaseException If error happens.
     */
    public void detachCollection(String subClFullName) throws BaseException {
        if (null == subClFullName || subClFullName.equals("") ||
                null == collectionFullName || collectionFullName.equals("")) {
            throw new BaseException(SDBError.SDB_INVALIDARG, subClFullName);
        }

        BSONObject options = new BasicBSONObject();
        options.put(SdbConstants.FIELD_NAME_NAME, collectionFullName);
        options.put(SdbConstants.FIELD_NAME_SUBCLNAME, subClFullName);

        AdminRequest request = new AdminRequest(AdminCommand.DETACH_CL, options);
        SdbReply response = sequoiadb.requestAndResponse(request);
        sequoiadb.throwIfError(response, subClFullName);
        sequoiadb.upsertCache(collectionFullName);
    }

    /**
     * Alter the attributes of current collection.
     * Can't alter attributes about split in partition collection; After altering a collection to
     * be a partition collection, need to split this collection manually.
     *
     * @param options The options for altering current collection are as below:
     *                <ul>
     *                <li>ReplSize     : Assign how many replica nodes need to be synchronized when a write request(insert, update, etc) is executed
     *                <li>ShardingKey   : Assign the sharding key
     *                <li>ShardingType        : Assign the sharding type
     *                <li>Partition        : When the ShardingType is "hash", need to assign Partition, it's the bucket number for hash, the range is [2^3,2^20].
     *                e.g. {RepliSize:0, ShardingKey:{a:1}, ShardingType:"hash", Partition:1024}
     *                </ul>
     * @throws BaseException If error happens.
     */
    public void alterCollection(BSONObject options) throws BaseException {
        if (null == options) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "options is null");
        }

        BSONObject newObj = new BasicBSONObject();
        if (!options.containsField(SdbConstants.FIELD_NAME_ALTER)) {
            newObj.put(SdbConstants.FIELD_NAME_NAME, collectionFullName);
            newObj.put(SdbConstants.FIELD_NAME_OPTIONS, options);
        } else {
            Object tmpAlter = options.get(SdbConstants.FIELD_NAME_ALTER);
            if (tmpAlter instanceof BasicBSONObject) {
                newObj.put(SdbConstants.FIELD_NAME_ALTER, tmpAlter);
            } else {
                throw new BaseException(SDBError.SDB_INVALIDARG, options.toString());
            }
            newObj.put(SdbConstants.FIELD_NAME_ALTER_TYPE,
                    SdbConstants.SDB_ALTER_CL);
            newObj.put(SdbConstants.FIELD_NAME_VERSION,
                    SdbConstants.SDB_ALTER_VERSION);
            newObj.put(SdbConstants.FIELD_NAME_NAME, collectionFullName);

            if (options.containsField(SdbConstants.FIELD_NAME_OPTIONS)) {
                Object tmpOptions = options.get(SdbConstants.FIELD_NAME_OPTIONS);
                if (tmpOptions instanceof BasicBSONObject) {
                    newObj.put(SdbConstants.FIELD_NAME_OPTIONS, tmpOptions);
                } else {
                    throw new BaseException(SDBError.SDB_INVALIDARG, options.toString());
                }
            }

        }

        AdminRequest request = new AdminRequest(AdminCommand.ALTER_COLLECTION, newObj);
        SdbReply response = sequoiadb.requestAndResponse(request);
        sequoiadb.throwIfError(response, options);
        sequoiadb.upsertCache(collectionFullName);
    }

    private void _update(int flag, BSONObject matcher, BSONObject modifier,
                         BSONObject hint) throws BaseException {
        UpdateRequest request = new UpdateRequest(collectionFullName, matcher, modifier, hint, flag);
        SdbReply response = sequoiadb.requestAndResponse(request);

        if (response.getFlag() != 0) {
            String msg = "matcher = " + matcher +
                    ", modifier = " + modifier +
                    ", hint = " + hint;
            sequoiadb.throwIfError(response, msg);
        }

        sequoiadb.upsertCache(collectionFullName);
    }

    /**
     * Get all of the lobs in current collection.
     *
     * @return DBCursor of lobs
     * @throws BaseException If error happens.
     */
    public DBCursor listLobs() throws BaseException {
        BSONObject obj = new BasicBSONObject();
        obj.put(SdbConstants.FIELD_COLLECTION, collectionFullName);

        AdminRequest request = new AdminRequest(AdminCommand.LIST_LOBS, obj);
        SdbReply response = sequoiadb.requestAndResponse(request);

        int flag = response.getFlag();
        if (flag != 0) {
            if (flag == SDBError.SDB_DMS_EOC.getErrorCode()) {
                return null;
            } else {
                sequoiadb.throwIfError(response);
            }
        }

        sequoiadb.upsertCache(collectionFullName);

        DBCursor cursor = new DBCursor(response, sequoiadb);
        return cursor;
    }

    /**
     * Create a lob.
     *
     * @return DBLob object
     * @throws BaseException If error happens..
     */
    public DBLob createLob() throws BaseException {
        return createLob(null);
    }

    /**
     * Create a lob with a given id.
     *
     * @param id the lob's id. if id is null, it will be generated in
     *           this function
     * @return DBLob object
     * @throws BaseException If error happens..
     */
    public DBLob createLob(ObjectId id) throws BaseException {
        DBLobImpl lob = new DBLobImpl(this);
        lob.open(id, DBLobImpl.SDB_LOB_CREATEONLY);
        sequoiadb.upsertCache(collectionFullName);
        return lob;
    }

    /**
     * Open an existing lob with id.
     *
     * @param id   the lob's id.
     * @param mode open mode:
     *             DBLob.SDB_LOB_READ for reading,
     *             DBLob.SDB_LOB_WRITE for writing.
     * @return DBLob object
     * @throws BaseException If error happens..
     */
    public DBLob openLob(ObjectId id, int mode) throws BaseException {
        if (mode != DBLob.SDB_LOB_READ && mode != DBLob.SDB_LOB_WRITE) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "mode is unsupported: " + mode);
        }

        DBLobImpl lob = new DBLobImpl(this);
        lob.open(id, mode);
        sequoiadb.upsertCache(collectionFullName);
        return lob;
    }

    /**
     * Open an existing lob with id.
     *
     * @param id the lob's id.
     * @return DBLob object
     * @throws BaseException If error happens.
     */
    public DBLob openLob(ObjectId id) throws BaseException {
        return openLob(id, DBLob.SDB_LOB_READ);
    }

    /**
     * Remove an existing lob.
     *
     * @param lobId the lob's id.
     * @throws BaseException If error happens..
     */
    public void removeLob(ObjectId lobId) throws BaseException {
        BSONObject removeObj = new BasicBSONObject();
        removeObj.put(SdbConstants.FIELD_COLLECTION, collectionFullName);
        removeObj.put(DBLobImpl.FIELD_NAME_LOB_OID, lobId);

        LobRemoveRequest request = new LobRemoveRequest(removeObj);
        SdbReply response = sequoiadb.requestAndResponse(request);
        sequoiadb.throwIfError(response, removeObj);
        sequoiadb.upsertCache(collectionFullName);
    }

    /**
     * Truncate an existing lob.
     *
     * @param lobId  the lob's id.
     * @param length the truncate length
     * @throws BaseException If error happens.
     */
    public void truncateLob(ObjectId lobId, long length) throws BaseException {
        if (length < 0) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "Invalid length");
        }

        BSONObject truncateObj = new BasicBSONObject();
        truncateObj.put(SdbConstants.FIELD_COLLECTION, collectionFullName);
        truncateObj.put(DBLobImpl.FIELD_NAME_LOB_OID, lobId);
        truncateObj.put(DBLobImpl.FIELD_NAME_LOB_LENGTH, length);

        LobTruncateRequest request = new LobTruncateRequest(truncateObj);
        SdbReply response = sequoiadb.requestAndResponse(request);
        sequoiadb.throwIfError(response, truncateObj);
        sequoiadb.upsertCache(collectionFullName);
    }

    /**
     * Truncate the collection.
     *
     * @throws BaseException If error happens.
     */
    public void truncate() throws BaseException {
        BSONObject options = new BasicBSONObject();
        options.put(SdbConstants.FIELD_COLLECTION, collectionFullName);

        AdminRequest request = new AdminRequest(AdminCommand.TRUNCATE, options);
        SdbReply response = sequoiadb.requestAndResponse(request);
        sequoiadb.throwIfError(response, options);
        sequoiadb.upsertCache(collectionFullName);
    }

    /**
     * Pop records from the collection.
     *
     * @param options the pop option for the operation, including a record LogicalID,
     *                and an optional Direction: 1 for forward pop and -1 for backward
     *                pop
     * @throws BaseException If error happens.
     */
    public void pop(BSONObject options) throws BaseException {
        if (null == options) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "options is null");
        }

        BSONObject newObj = new BasicBSONObject();
        newObj.put(SdbConstants.FIELD_COLLECTION, collectionFullName);
        Object lidObj = options.get(SdbConstants.FIELD_NAME_LOGICALID);
        if (lidObj instanceof Integer || lidObj instanceof Long) {
            newObj.put(SdbConstants.FIELD_NAME_LOGICALID, lidObj);
        } else {
            throw new BaseException(SDBError.SDB_INVALIDARG, options.toString());
        }
        Object directObj = options.get(SdbConstants.FIELD_NAME_DIRECTION);
        if (null == directObj) {
            newObj.put(SdbConstants.FIELD_NAME_DIRECTION, 1);
        } else if (directObj instanceof Integer) {
            newObj.put(SdbConstants.FIELD_NAME_DIRECTION, directObj);
        } else {
            throw new BaseException(SDBError.SDB_INVALIDARG, options.toString());
        }

        AdminRequest request = new AdminRequest(AdminCommand.POP, newObj);
        SdbReply response = sequoiadb.requestAndResponse(request);
        sequoiadb.throwIfError(response, newObj);
        sequoiadb.upsertCache(collectionFullName);
    }
}
