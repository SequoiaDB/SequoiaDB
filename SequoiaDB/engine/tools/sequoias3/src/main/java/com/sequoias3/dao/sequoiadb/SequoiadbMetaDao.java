package com.sequoias3.dao.sequoiadb;

import com.sequoiadb.base.*;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoias3.common.DBParamDefine;
import com.sequoias3.config.SequoiadbConfig;
import com.sequoias3.core.ObjectMeta;
import com.sequoias3.core.Region;
import com.sequoias3.dao.*;
import com.sequoias3.exception.S3Error;
import com.sequoias3.exception.S3ServerException;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Repository;

import java.util.List;

@Repository("MetaDao")
public class SequoiadbMetaDao implements MetaDao {
    private static final Logger logger = LoggerFactory.getLogger(SequoiadbMetaDao.class);

    @Autowired
    SdbDataSourceWrapper sdbDatasourceWrapper;

    @Autowired
    SequoiadbConfig config;

    @Autowired
    SequoiadbRegionSpaceDao sequoiadbRegionSpaceDao;

    @Autowired
    SdbBaseOperation sdbBaseOperation;

    @Override
    public void insertMeta(ConnectionDao connection, String csMetaName, String clMetaName,
                           ObjectMeta objectMeta, Boolean isHistory, Region region)
            throws S3ServerException {
        try {
            Sequoiadb sdb = ((SdbConnectionDao)connection).getConnection();
            insert(sdb, csMetaName, clMetaName, objectMeta, isHistory);
        }catch (BaseException e){
            if (e.getErrorCode() == SDBError.SDB_DMS_CS_NOTEXIST.getErrorCode()
                    || e.getErrorCode() == SDBError.SDB_DMS_NOTEXIST.getErrorCode()) {
                logger.info("no collection. cl = {}.{}", csMetaName, clMetaName);
                throw new S3ServerException(S3Error.REGION_COLLECTION_NOT_EXIST, "no meta collection");
            } else {
                logger.error("insert meta failed. error:"+e);
                throw e;
            }
        }catch (Exception e){
            throw e;
        }
    }

    private void insert(Sequoiadb sdb, String csMetaName, String clMetaName,
                       ObjectMeta objectMeta, Boolean isHistory)
            throws S3ServerException{
        try {
            CollectionSpace cs = sdb.getCollectionSpace(csMetaName);
            DBCollection cl = cs.getCollection(clMetaName);

            BSONObject insertData = convertMetaToBson(objectMeta);
            cl.insert(insertData);
        }catch (BaseException e){
            if (e.getErrorCode() == SDBError.SDB_IXM_DUP_KEY.getErrorCode()) {
                logger.error("duplicate key. csname:{}, clname:{}, key:{}, versionId:{}, isHistory:{}",
                        csMetaName, clMetaName, objectMeta.getKey(), objectMeta.getVersionId(), isHistory);
                if (!isHistory) {
                    throw new S3ServerException(S3Error.DAO_DUPLICATE_KEY,
                            "Duplicate key. csname:" + csMetaName + ", clname:" + clMetaName
                                    + ", key:" + objectMeta.getKey()
                                    + ", versionId:" + objectMeta.getVersionId());
                }
            }else{
                throw e;
            }
        }catch (Exception e){
            logger.error("Insert object meta failed");
            throw e;
        }
    }

    //query meta list by bucketid, prefix, startafter, versionid from cur meta and his meta
    @Override
    public QueryDbCursor queryMetaByBucket(String metaCsName, String metaClName, long bucketId,
                                           String prefix, String startAfter, Long specifiedVId,
                                           Boolean isIncludeDeleteMarker) throws S3ServerException{
        Sequoiadb sdb = null;
        DBCursor dbCursor = null;
        try {
            sdb = sdbDatasourceWrapper.getSequoiadb();
            CollectionSpace cs = sdb.getCollectionSpace(metaCsName);
            DBCollection cl = cs.getCollection(metaClName);

            BSONObject matcher = new BasicBSONObject();
            matcher.put(ObjectMeta.META_BUCKET_ID, bucketId);
            if (!isIncludeDeleteMarker) {
                matcher.put(ObjectMeta.META_DELETE_MARKER, false);
            }
            BSONObject keyMatcher = new BasicBSONObject();
            if (prefix != null && prefix.length() > 0){
                keyMatcher.put(DBParamDefine.NOT_SMALL, prefix);
                String prefixEnd = prefix.substring(0,prefix.length()-1) + (char)(prefix.charAt(prefix.length()-1)+1);
                keyMatcher.put(DBParamDefine.LESS_THAN, prefixEnd);
            }
            if (startAfter != null){
                if (specifiedVId != null) {
                    if(prefix == null || startAfter.compareTo(prefix) > 0) {
                        keyMatcher.put(DBParamDefine.NOT_SMALL, startAfter);
                    }
                }else{
                    keyMatcher.put(DBParamDefine.GREATER, startAfter);
                }
            }
            if (!keyMatcher.isEmpty()) {
                matcher.put(ObjectMeta.META_KEY_NAME, keyMatcher);
            }

            BSONObject selector = buildSelectForMeta();

            BSONObject orderBy = new BasicBSONObject();
            orderBy.put(ObjectMeta.META_KEY_NAME, 1);
            orderBy.put(ObjectMeta.META_VERSION_ID, -1);

            dbCursor = cl.query(matcher, selector, orderBy,null, 0);
            return new SdbQueryDbCursor(sdb, dbCursor);
        }catch (BaseException e){
            sdbBaseOperation.releaseDBCursor(dbCursor);
            sdbDatasourceWrapper.releaseSequoiadb(sdb);
            if (e.getErrorCode() == SDBError.SDB_DMS_CS_NOTEXIST.getErrorCode() ||
                    e.getErrorCode() == SDBError.SDB_DMS_NOTEXIST.getErrorCode()) {
                //no cs or cl ,return null
                return null;
            } else {
                logger.error("query metalist by bucket failed. error:",e);
                throw e;
            }
        } catch (Exception e){
            sdbBaseOperation.releaseDBCursor(dbCursor);
            sdbDatasourceWrapper.releaseSequoiadb(sdb);
            logger.error("query metalist by bucket failed.");
            throw e;
        }
    }

    //query one meta by object name and version id from cur meta or his meta
    @Override
    public ObjectMeta queryMetaByObjectName(String metaCsName, String metaClName,
                                            long bucketId, String objectName,
                                            Long versionId, Boolean noVersionFlag)
            throws S3ServerException {
        Sequoiadb sdb = null;
        try {
            sdb = sdbDatasourceWrapper.getSequoiadb();
            CollectionSpace cs = sdb.getCollectionSpace(metaCsName);
            DBCollection cl = cs.getCollection(metaClName);

            BSONObject matcher = new BasicBSONObject();
            matcher.put(ObjectMeta.META_BUCKET_ID, bucketId);
            matcher.put(ObjectMeta.META_KEY_NAME, objectName);
            if (versionId != null){
                matcher.put(ObjectMeta.META_VERSION_ID, versionId);
            }
            if (noVersionFlag != null){
                matcher.put(ObjectMeta.META_NO_VERSION_FLAG, noVersionFlag);
            }

            BSONObject queryResult = cl.queryOne(matcher, null, null, null, 0);
            return convertBsonToMeta(queryResult);
        }catch (BaseException e){
            if (e.getErrorCode() == SDBError.SDB_DMS_CS_NOTEXIST.getErrorCode() ||
                    e.getErrorCode() == SDBError.SDB_DMS_NOTEXIST.getErrorCode()) {
                //no cs or cl ,return null
                return null;
            } else {
                logger.error("query meta by name failed. error:"+e.getMessage());
                throw e;
            }
        } catch (Exception e){
            logger.error("query meta by name failed.");
            throw e;
        }finally {
            sdbDatasourceWrapper.releaseSequoiadb(sdb);
        }
    }

    //query one meta with bucketId from cur meta or his meta, for isempty bucket
    @Override
    public ObjectMeta queryMetaByBucketId(ConnectionDao connection, String metaCsName, String metaClName,
                                            long bucketId)
            throws S3ServerException {
        Sequoiadb sdb = null;
        try {
            if(connection != null){
                sdb = ((SdbConnectionDao)connection).getConnection();
            }else {
                sdb = sdbDatasourceWrapper.getSequoiadb();
            }
            CollectionSpace cs = sdb.getCollectionSpace(metaCsName);
            DBCollection cl = cs.getCollection(metaClName);

            BSONObject matcher = new BasicBSONObject();
            matcher.put(ObjectMeta.META_BUCKET_ID, bucketId);

            BSONObject hint = new BasicBSONObject();
            hint.put("", "");

            BSONObject queryResult = cl.queryOne(matcher, null, null, hint, 0);
            return convertBsonToMeta(queryResult);
        }catch (BaseException e){
            if (e.getErrorCode() == SDBError.SDB_DMS_CS_NOTEXIST.getErrorCode() ||
                    e.getErrorCode() == SDBError.SDB_DMS_NOTEXIST.getErrorCode()) {
                //no cs or cl ,return null
                return null;
            } else {
                logger.error("query meta by name failed. error:"+e.getMessage());
                throw e;
            }
        } catch (Exception e){
            logger.error("query meta by name failed.");
            throw e;
        }finally {
            if (connection == null) {
                sdbDatasourceWrapper.releaseSequoiadb(sdb);
            }
        }
    }

    //query metalist with bucketId from cur meta, for update delimiter
    @Override
    public QueryDbCursor queryMetaByBucketForUpdate(ConnectionDao connection,
                                                    String metaCsName, String metaClName,
                                                    long bucketId, String prefix,
                                                    String startAfter, int limitNum)
            throws S3ServerException {
        DBCursor dbCursor = null;
        try {
            Sequoiadb sdb =  ((SdbConnectionDao)connection).getConnection();
            CollectionSpace cs = sdb.getCollectionSpace(metaCsName);
            DBCollection cl = cs.getCollection(metaClName);

            BSONObject matcher = new BasicBSONObject();
            matcher.put(ObjectMeta.META_BUCKET_ID, bucketId);
            BSONObject keyMatcher = new BasicBSONObject();
            if (prefix != null && prefix.length() > 0){
                keyMatcher.put(DBParamDefine.NOT_SMALL, prefix);
                String prefixEnd = prefix.substring(0,prefix.length()-1) + (char)(prefix.charAt(prefix.length()-1)+1);
                keyMatcher.put(DBParamDefine.LESS_THAN, prefixEnd);
            }
            if (startAfter != null){
                keyMatcher.put(DBParamDefine.GREATER, startAfter);
            }
            if (!keyMatcher.isEmpty()) {
                matcher.put(ObjectMeta.META_KEY_NAME, keyMatcher);
            }

            BSONObject selector = new BasicBSONObject();
            selector.put(ObjectMeta.META_KEY_NAME, "");
            selector.put(ObjectMeta.META_VERSION_ID, "");
            selector.put(ObjectMeta.META_DELETE_MARKER, 0);

            BSONObject orderBy = new BasicBSONObject();
            orderBy.put(ObjectMeta.META_KEY_NAME, 1);
            orderBy.put(ObjectMeta.META_VERSION_ID, -1);

            BSONObject hint = new BasicBSONObject();
            hint.put("", "");

            dbCursor = cl.query(matcher, selector, orderBy, hint, 0, limitNum, DBQuery.FLG_QUERY_FOR_UPDATE);
            return new SdbQueryDbCursor(null, dbCursor);
        }catch (BaseException e){
            sdbBaseOperation.releaseDBCursor(dbCursor);
            if (e.getErrorCode() == SDBError.SDB_DMS_CS_NOTEXIST.getErrorCode() ||
                    e.getErrorCode() == SDBError.SDB_DMS_NOTEXIST.getErrorCode()) {
                //no cs or cl ,return null
                return null;
            } else {
                logger.error("query metalist by bucket failed. error:",e);
                throw e;
            }
        } catch (Exception e){
            sdbBaseOperation.releaseDBCursor(dbCursor);
            logger.error("query metalist by bucket failed.");
            throw e;
        }
    }

    @Override
    public QueryDbCursor queryMetaByBucketInKeys(String metaCsName, String metaClName,
                                               long bucketId, List<String> keys)
            throws S3ServerException {
        Sequoiadb sdb = null;
        DBCursor dbCursor = null;
        try {
            sdb = sdbDatasourceWrapper.getSequoiadb();
            CollectionSpace cs = sdb.getCollectionSpace(metaCsName);
            DBCollection cl = cs.getCollection(metaClName);

            BSONObject matcher = new BasicBSONObject();
            matcher.put(ObjectMeta.META_BUCKET_ID, bucketId);

            BSONObject nameMatcher = new BasicBSONObject();
            int size = keys.size();
            if (size > 0){
                nameMatcher.put(DBParamDefine.IN, keys);
            }

            if (!nameMatcher.isEmpty()) {
                matcher.put(ObjectMeta.META_KEY_NAME, nameMatcher);
            }

            BSONObject selector = buildSelectForMeta();

            BSONObject orderBy = new BasicBSONObject();
            orderBy.put(ObjectMeta.META_KEY_NAME, 1);
            orderBy.put(ObjectMeta.META_VERSION_ID, -1);

            dbCursor = cl.query(matcher, selector, orderBy,null, 0);
            return new SdbQueryDbCursor(sdb, dbCursor);
        }catch (BaseException e){
            sdbBaseOperation.releaseDBCursor(dbCursor);
            sdbDatasourceWrapper.releaseSequoiadb(sdb);
            if (e.getErrorCode() == SDBError.SDB_DMS_CS_NOTEXIST.getErrorCode() ||
                    e.getErrorCode() == SDBError.SDB_DMS_NOTEXIST.getErrorCode()) {
                //no cs or cl ,return null
                return null;
            } else {
                logger.error("query metalist by bucket parentId failed. error:",e);
                throw e;
            }
        } catch (Exception e){
            sdbBaseOperation.releaseDBCursor(dbCursor);
            sdbDatasourceWrapper.releaseSequoiadb(sdb);
            logger.error("query metalist by bucket failed.");
            throw e;
        }
    }

    @Override
    public QueryDbCursor queryMetaListByParentId(String metaCsName, String metaClName,
                                                 long bucketId, String parentIdName,
                                                 long parentId, String prefix, String startAfter,
                                                 Long versionIdMarker, Boolean isIncludeDeleteMarker)
            throws S3ServerException {
        Sequoiadb sdb = null;
        DBCursor dbCursor = null;
        try {
            sdb = sdbDatasourceWrapper.getSequoiadb();
            CollectionSpace cs = sdb.getCollectionSpace(metaCsName);
            DBCollection cl = cs.getCollection(metaClName);

            BSONObject matcher = new BasicBSONObject();
            matcher.put(ObjectMeta.META_BUCKET_ID, bucketId);
            if (!isIncludeDeleteMarker) {
                matcher.put(ObjectMeta.META_DELETE_MARKER, false);
            }
            matcher.put(parentIdName, parentId);
            BSONObject nameMatcher = new BasicBSONObject();
            if (prefix != null && prefix.length() > 0){
                nameMatcher.put(DBParamDefine.NOT_SMALL, prefix);
                String prefixEnd = prefix.substring(0,prefix.length()-1) + (char)(prefix.charAt(prefix.length()-1)+1);
                nameMatcher.put(DBParamDefine.LESS_THAN, prefixEnd);
            }
            if (startAfter != null){
                if (versionIdMarker == null) {
                    nameMatcher.put(DBParamDefine.GREATER, startAfter);
                }else {
                    if(prefix == null || startAfter.compareTo(prefix) > 0) {
                        nameMatcher.put(DBParamDefine.NOT_SMALL, startAfter);
                    }
                }
            }
            if (!nameMatcher.isEmpty()) {
                matcher.put(ObjectMeta.META_KEY_NAME, nameMatcher);
            }

            BSONObject selector = buildSelectForMeta();

            BSONObject orderBy = new BasicBSONObject();
            orderBy.put(ObjectMeta.META_KEY_NAME, 1);
            orderBy.put(ObjectMeta.META_VERSION_ID, -1);

            dbCursor = cl.query(matcher, selector, orderBy,null, 0);
            return new SdbQueryDbCursor(sdb, dbCursor);
        }catch (BaseException e){
            sdbBaseOperation.releaseDBCursor(dbCursor);
            sdbDatasourceWrapper.releaseSequoiadb(sdb);
            if (e.getErrorCode() == SDBError.SDB_DMS_CS_NOTEXIST.getErrorCode() ||
                    e.getErrorCode() == SDBError.SDB_DMS_NOTEXIST.getErrorCode()) {
                //no cs or cl ,return null
                return null;
            } else {
                logger.error("query metalist by bucket parentId failed. error:",e);
                throw e;
            }
        } catch (Exception e){
            sdbBaseOperation.releaseDBCursor(dbCursor);
            sdbDatasourceWrapper.releaseSequoiadb(sdb);
            logger.error("query metalist by bucket failed.");
            throw e;
        }
    }

    //query one other meta by parentId1 or parentId2
    @Override
    public Boolean queryOneOtherMetaByParentId(ConnectionDao connection, String metaCsName,
                                          String metaClName, long bucketId, String objectName,
                                          String parentIdName, long parentId)
            throws S3ServerException {
        Sequoiadb sdb = null;
        try {
            if (connection != null) {
                sdb = ((SdbConnectionDao) connection).getConnection();
            }else {
                sdb = sdbDatasourceWrapper.getSequoiadb();
            }
            CollectionSpace cs = sdb.getCollectionSpace(metaCsName);
            DBCollection cl = cs.getCollection(metaClName);

            BSONObject matcher = new BasicBSONObject();
            matcher.put(ObjectMeta.META_BUCKET_ID, bucketId);
            matcher.put(parentIdName, parentId);

            BSONObject keyMatcher = new BasicBSONObject();
            keyMatcher.put(DBParamDefine.NOT_EQUAL, objectName);
            matcher.put(ObjectMeta.META_KEY_NAME, keyMatcher);

            BSONObject hint = new BasicBSONObject();
            hint.put("", "");

            BSONObject result = cl.queryOne(matcher, null, null, hint, 0);
            if (result != null){
                return true;
            }else {
                return false;
            }
        }catch (BaseException e){
            if (e.getErrorCode() == SDBError.SDB_DMS_CS_NOTEXIST.getErrorCode() ||
                    e.getErrorCode() == SDBError.SDB_DMS_NOTEXIST.getErrorCode()) {
                //no cs or cl ,return null
                return false;
            } else {
                logger.error("query one meta by parentId failed. error:",e);
                throw e;
            }
        } catch (Exception e){
            logger.error("query one meta by parentId failed.");
            throw e;
        } finally {
            if (connection == null) {
                sdbDatasourceWrapper.releaseSequoiadb(sdb);
            }
        }
    }

    // query one meta and get U lock from cur meta or his meta
    @Override
    public ObjectMeta queryForUpdate(ConnectionDao connection, String metaCsName, String metaClName,
                                     long bucketId, String objectName, Long versionId, Boolean noVersionFlag)
            throws S3ServerException {
        try {
            Sequoiadb sdb = ((SdbConnectionDao)connection).getConnection();
            CollectionSpace cs = sdb.getCollectionSpace(metaCsName);
            DBCollection cl = cs.getCollection(metaClName);

            BSONObject matcher = new BasicBSONObject();
            matcher.put(ObjectMeta.META_BUCKET_ID, bucketId);
            matcher.put(ObjectMeta.META_KEY_NAME, objectName);
            if (versionId != null){
                matcher.put(ObjectMeta.META_VERSION_ID, versionId);
            }
            if (noVersionFlag != null){
                matcher.put(ObjectMeta.META_NO_VERSION_FLAG, noVersionFlag);
            }

            BSONObject order = new BasicBSONObject();
            order.put(ObjectMeta.META_VERSION_ID, -1);

            BSONObject hint = new BasicBSONObject();
            hint.put("", "");

            BSONObject queryResult = cl.queryOne(matcher, null, order, hint, DBQuery.FLG_QUERY_FOR_UPDATE);
            return convertBsonToMeta(queryResult);
        }catch (BaseException e){
            if (e.getErrorCode() == SDBError.SDB_DMS_CS_NOTEXIST.getErrorCode() ||
                    e.getErrorCode() == SDBError.SDB_DMS_NOTEXIST.getErrorCode()) {
                //no cs or cl ,return null
                return null;
            } else {
                logger.error("query meta by name failed. error:",e);
                throw new S3ServerException(S3Error.DAO_DB_ERROR, "query meta by name failed");
            }
        } catch (Exception e){
            logger.error("query meta by name failed.");
            throw new S3ServerException(S3Error.DAO_DB_ERROR, "query meta by name failed");
        }
    }

    // update meta in cur meta by bucketId and object name
    @Override
    public void updateMeta(ConnectionDao connection, String metaCsName, String metaClName, long bucketId,
                           String objectName, Long versionId, ObjectMeta objectMeta)
            throws S3ServerException {
        try {
            Sequoiadb sdb = ((SdbConnectionDao)connection).getConnection();
            CollectionSpace cs = sdb.getCollectionSpace(metaCsName);
            DBCollection cl = cs.getCollection(metaClName);

            BSONObject matcher = new BasicBSONObject();
            matcher.put(ObjectMeta.META_BUCKET_ID, bucketId);
            matcher.put(ObjectMeta.META_KEY_NAME, objectName);
            if (versionId != null){
                matcher.put(ObjectMeta.META_VERSION_ID, versionId);
            }

            BSONObject updateData = convertMetaToBson(objectMeta);
            BSONObject setUpdate = new BasicBSONObject();
            setUpdate.put(DBParamDefine.MODIFY_SET, updateData);

            BSONObject hint = new BasicBSONObject();
            hint.put("", "");

            cl.update(matcher, setUpdate, hint);
        } catch (Exception e){
            logger.error("update meta failed. error:"+e.getMessage());
            throw new S3ServerException(S3Error.DAO_DB_ERROR, "db error.", e);
        }
    }

    // update object parentId in cur meta, for update delimiter
    @Override
    public void updateMetaParentId(ConnectionDao connection, String metaCsName, String metaClName,
                                   long bucketId, String objectName, String parentIdName, long parentId)
            throws S3ServerException {
        try {
            Sequoiadb sdb = ((SdbConnectionDao)connection).getConnection();
            CollectionSpace cs = sdb.getCollectionSpace(metaCsName);
            DBCollection cl = cs.getCollection(metaClName);

            BSONObject matcher = new BasicBSONObject();
            matcher.put(ObjectMeta.META_BUCKET_ID, bucketId);
            matcher.put(ObjectMeta.META_KEY_NAME, objectName);

            BSONObject updateData = new BasicBSONObject();
            updateData.put(parentIdName, parentId);
            BSONObject setUpdate = new BasicBSONObject();
            setUpdate.put(DBParamDefine.MODIFY_SET, updateData);

            BSONObject hint = new BasicBSONObject();
            hint.put("", "");

            cl.update(matcher, setUpdate, hint);
        } catch (Exception e){
            logger.error("update meta failed. error:"+e.getMessage());
            throw new S3ServerException(S3Error.DAO_DB_ERROR, "db error.", e);
        }
    }

    // remove meta from cur meta or his meta by BucketId+Key or by BucketId+Key+VersionId
    @Override
    public void removeMeta(ConnectionDao connection, String metaCsName, String metaClName, long bucketId,
                           String objectName, Long versionId, Boolean noVersionFlag)
            throws S3ServerException {
        try {
            Sequoiadb sdb = ((SdbConnectionDao)connection).getConnection();
            CollectionSpace cs = sdb.getCollectionSpace(metaCsName);
            DBCollection cl = cs.getCollection(metaClName);

            BSONObject matcher = new BasicBSONObject();
            matcher.put(ObjectMeta.META_BUCKET_ID, bucketId);
            matcher.put(ObjectMeta.META_KEY_NAME, objectName);
            if (versionId != null){
                matcher.put(ObjectMeta.META_VERSION_ID, versionId);
            }
            if (noVersionFlag != null){
                matcher.put(ObjectMeta.META_NO_VERSION_FLAG, noVersionFlag);
            }

            BSONObject hint = new BasicBSONObject();
            hint.put("", "");

            cl.delete(matcher, hint);
        } catch (Exception e){
            logger.error("remove meta failed. error:"+e.getMessage());
            throw new S3ServerException(S3Error.DAO_DB_ERROR, "db error.", e);
        }
    }

    @Override
    public long getObjectNumber(String metaCsName, String metaClName, long bucketId)
            throws S3ServerException{
        Sequoiadb sdb = null;
        try {
            sdb = sdbDatasourceWrapper.getSequoiadb();
            CollectionSpace cs = sdb.getCollectionSpace(metaCsName);
            DBCollection cl = cs.getCollection(metaClName);

            BSONObject matcher = new BasicBSONObject();
            matcher.put(ObjectMeta.META_BUCKET_ID, bucketId);

            return cl.getCount(matcher);
        }catch (BaseException e){
            if (e.getErrorCode() == SDBError.SDB_DMS_CS_NOTEXIST.getErrorCode() ||
                    e.getErrorCode() == SDBError.SDB_DMS_NOTEXIST.getErrorCode()) {
                //no cs or cl ,return null
                return 0;
            } else {
                logger.error("query object number failed. error:"+e.getMessage());
                throw e;
            }
        } catch (Exception e){
            logger.error("query object number failed.");
            throw e;
        }finally {
            sdbDatasourceWrapper.releaseSequoiadb(sdb);
        }
    }

    private BSONObject buildSelectForMeta(){
        BSONObject selector = new BasicBSONObject();
        selector.put(ObjectMeta.META_KEY_NAME, "");
        selector.put(ObjectMeta.META_VERSION_ID, "");
        selector.put(ObjectMeta.META_LAST_MODIFIED, 0);
        selector.put(ObjectMeta.META_ETAG, "");
        selector.put(ObjectMeta.META_SIZE, "");
        selector.put(ObjectMeta.META_DELETE_MARKER, 0);
        selector.put(ObjectMeta.META_CS_NAME, 0);
        selector.put(ObjectMeta.META_CL_NAME, 0);
        selector.put(ObjectMeta.META_LOB_ID, 0);
        selector.put(ObjectMeta.META_NO_VERSION_FLAG, 0);
        return selector;
    }

    private ObjectMeta convertBsonToMeta(BSONObject bsonObject){
        if (null == bsonObject){
            return null;
        }
        ObjectMeta object = new ObjectMeta();
        object.setBucketId((long)bsonObject.get(ObjectMeta.META_BUCKET_ID));
        object.setKey(bsonObject.get(ObjectMeta.META_KEY_NAME).toString());
        object.setLobId((ObjectId)bsonObject.get(ObjectMeta.META_LOB_ID));
        object.setVersionId((Long)bsonObject.get(ObjectMeta.META_VERSION_ID));
        object.setNoVersionFlag((Boolean)bsonObject.get(ObjectMeta.META_NO_VERSION_FLAG));
        if (bsonObject.get(ObjectMeta.META_CS_NAME) != null){
            object.setCsName(bsonObject.get(ObjectMeta.META_CS_NAME).toString());
        }
        if (bsonObject.get(ObjectMeta.META_CL_NAME) != null) {
            object.setClName(bsonObject.get(ObjectMeta.META_CL_NAME).toString());
        }
        object.setLastModified((long)bsonObject.get(ObjectMeta.META_LAST_MODIFIED));
        if (bsonObject.get(ObjectMeta.META_ETAG) != null) {
            object.seteTag(bsonObject.get(ObjectMeta.META_ETAG).toString());
        }
        object.setSize((long) bsonObject.get(ObjectMeta.META_SIZE));
        object.setDeleteMarker((Boolean)bsonObject.get(ObjectMeta.META_DELETE_MARKER));
        if (bsonObject.get(ObjectMeta.META_EXPIRES) != null) {
            object.setExpires(bsonObject.get(ObjectMeta.META_EXPIRES).toString());
        }
        if (bsonObject.get(ObjectMeta.META_CONTENT_TYPE) != null) {
            object.setContentType(bsonObject.get(ObjectMeta.META_CONTENT_TYPE).toString());
        }
        if (bsonObject.get(ObjectMeta.META_CONTENT_ENCODING) != null) {
            object.setContentEncoding(bsonObject.get(ObjectMeta.META_CONTENT_ENCODING).toString());
        }
        if (bsonObject.get(ObjectMeta.META_CONTENT_DISPOSITION) != null) {
            object.setContentDisposition(bsonObject.get(ObjectMeta.META_CONTENT_DISPOSITION).toString());
        }
        if (bsonObject.get(ObjectMeta.META_CACHE_CONTROL) != null) {
            object.setCacheControl(bsonObject.get(ObjectMeta.META_CACHE_CONTROL).toString());
        }
        if (bsonObject.get(ObjectMeta.META_CONTENT_LANGUAGE) != null) {
            object.setContentLanguage(bsonObject.get(ObjectMeta.META_CONTENT_LANGUAGE).toString());
        }
        if (bsonObject.get(ObjectMeta.META_LIST) != null){
            BSONObject xMeta = (BSONObject)bsonObject.get(ObjectMeta.META_LIST);
            object.setMetaList(xMeta.toMap());
        }
        if (bsonObject.get(ObjectMeta.META_PARENTID1) != null){
            object.setParentId1((long)bsonObject.get(ObjectMeta.META_PARENTID1));
        }
        if (bsonObject.get(ObjectMeta.META_PARENTID2) != null){
            object.setParentId2((long)bsonObject.get(ObjectMeta.META_PARENTID2));
        }
        if (bsonObject.get(ObjectMeta.META_ACLID) != null){
            object.setAclId((long) bsonObject.get(ObjectMeta.META_ACLID));
        }
        return object;
    }

    private BSONObject convertMetaToBson(ObjectMeta meta){
        if (null == meta){
            return null;
        }

        BSONObject objectMeta = new BasicBSONObject();
        objectMeta.put(ObjectMeta.META_KEY_NAME, meta.getKey());
        objectMeta.put(ObjectMeta.META_NO_VERSION_FLAG, meta.getNoVersionFlag());
        objectMeta.put(ObjectMeta.META_BUCKET_ID, meta.getBucketId());
        objectMeta.put(ObjectMeta.META_CS_NAME, meta.getCsName());
        objectMeta.put(ObjectMeta.META_CL_NAME, meta.getClName());
        objectMeta.put(ObjectMeta.META_LOB_ID, meta.getLobId());
        objectMeta.put(ObjectMeta.META_VERSION_ID, meta.getVersionId());
        objectMeta.put(ObjectMeta.META_ETAG, meta.geteTag());
        objectMeta.put(ObjectMeta.META_LAST_MODIFIED, meta.getLastModified());
        objectMeta.put(ObjectMeta.META_SIZE, meta.getSize());
        objectMeta.put(ObjectMeta.META_CACHE_CONTROL, meta.getCacheControl());
        objectMeta.put(ObjectMeta.META_CONTENT_DISPOSITION, meta.getContentDisposition());
        objectMeta.put(ObjectMeta.META_CONTENT_ENCODING, meta.getContentEncoding());
        objectMeta.put(ObjectMeta.META_CONTENT_TYPE, meta.getContentType());
        objectMeta.put(ObjectMeta.META_DELETE_MARKER, meta.getDeleteMarker());
        objectMeta.put(ObjectMeta.META_EXPIRES, meta.getExpires());
        objectMeta.put(ObjectMeta.META_CONTENT_LANGUAGE, meta.getContentLanguage());
        if (meta.getMetaList() == null){
            objectMeta.put(ObjectMeta.META_LIST, null);
        } else {
            BSONObject xMetaList = new BasicBSONObject(meta.getMetaList());
            objectMeta.put(ObjectMeta.META_LIST, xMetaList);
        }
        objectMeta.put(ObjectMeta.META_PARENTID1, meta.getParentId1());
        objectMeta.put(ObjectMeta.META_PARENTID2, meta.getParentId2());
        objectMeta.put(ObjectMeta.META_ACLID, meta.getAclId());

        return objectMeta;
    }

    @Override
    public void releaseQueryDbCursor(QueryDbCursor queryDbCursor) {
        if (queryDbCursor != null) {
            SdbQueryDbCursor sdbQueryDbCursor = (SdbQueryDbCursor)queryDbCursor;
            sdbBaseOperation.releaseDBCursor(sdbQueryDbCursor.getCursor());
            sdbDatasourceWrapper.releaseSequoiadb(sdbQueryDbCursor.getSdb());
            sdbQueryDbCursor.setCursor(null);
            sdbQueryDbCursor.setSdb(null);
        }
    }
}
