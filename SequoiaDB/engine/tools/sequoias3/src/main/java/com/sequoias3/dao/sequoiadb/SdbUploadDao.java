package com.sequoias3.dao.sequoiadb;

import com.sequoiadb.base.*;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoias3.common.DBParamDefine;
import com.sequoias3.config.SequoiadbConfig;
import com.sequoias3.core.UploadMeta;
import com.sequoias3.dao.*;
import com.sequoias3.exception.S3Error;
import com.sequoias3.exception.S3ServerException;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Repository;

@Repository("UploadDao")
public class SdbUploadDao implements UploadDao {
    private static final Logger logger = LoggerFactory.getLogger(SdbUploadDao.class);

    @Autowired
    SdbDataSourceWrapper sdbDatasourceWrapper;

    @Autowired
    SequoiadbConfig config;

    @Autowired
    SdbBaseOperation sdbBaseOperation;

    @Override
    public void insertUploadMeta(ConnectionDao connection, long bucketId, String objectName, Long uploadId,
                                 UploadMeta meta) throws S3ServerException {
        Sequoiadb sdb = null;
        try {
            if (connection != null) {
                sdb = ((SdbConnectionDao) connection).getConnection();
            }else {
                sdb = sdbDatasourceWrapper.getSequoiadb();
            }
            CollectionSpace cs = sdb.getCollectionSpace(config.getMetaCsName());
            DBCollection cl = cs.getCollection(DaoCollectionDefine.UPLOAD_LIST);

            BSONObject uploadMeta = convertMetaToBson(meta);

            cl.insert(uploadMeta);
        }catch (BaseException e){
            if (e.getErrorType() == SDBError.SDB_IXM_DUP_KEY.name()) {
                throw new S3ServerException(S3Error.DAO_DUPLICATE_KEY, "Duplicate key.");
            } else {
                throw e;
            }
        } catch (Exception e) {
            logger.error("insert upload failed. uploadId:{}", uploadId);
            throw e;
        } finally {
            if (connection == null){
                sdbDatasourceWrapper.releaseSequoiadb(sdb);
            }
        }
    }

    @Override
    public void updateUploadMeta(ConnectionDao connection, long bucketId, String objectName,
                                 Long uploadId, UploadMeta update)
            throws S3ServerException {
        Sequoiadb sdb = null;
        try {
            if (connection != null) {
                sdb = ((SdbConnectionDao) connection).getConnection();
            }else {
                sdb = sdbDatasourceWrapper.getSequoiadb();
            }

            CollectionSpace cs = sdb.getCollectionSpace(config.getMetaCsName());
            DBCollection cl = cs.getCollection(DaoCollectionDefine.UPLOAD_LIST);

            BSONObject matcher = new BasicBSONObject();
            matcher.put(UploadMeta.META_BUCKET_ID, bucketId);
            matcher.put(UploadMeta.META_KEY_NAME, objectName);
            matcher.put(UploadMeta.META_UPLOAD_ID, uploadId);

            BSONObject updateMeta = convertMetaToBson(update);
            BSONObject setUpdate = new BasicBSONObject();
            setUpdate.put(DBParamDefine.MODIFY_SET, updateMeta);

            BSONObject hint = new BasicBSONObject();
            hint.put("", "");

            cl.update(matcher, setUpdate, hint);
        } catch (Exception e){
            logger.error("update upload failed. uploadId:{}", uploadId);
            throw e;
        } finally {
            if (connection == null){
                sdbDatasourceWrapper.releaseSequoiadb(sdb);
            }
        }
    }

    @Override
    public UploadMeta queryUploadByUploadId(ConnectionDao connection, Long bucketId,
                                        String objectName, long uploadId, Boolean forUpdate)
            throws S3ServerException {
        Sequoiadb sdb = null;
        try {
            if (connection != null) {
                sdb = ((SdbConnectionDao) connection).getConnection();
            } else {
                sdb = sdbDatasourceWrapper.getSequoiadb();
            }

            CollectionSpace cs = sdb.getCollectionSpace(config.getMetaCsName());
            DBCollection cl = cs.getCollection(DaoCollectionDefine.UPLOAD_LIST);

            BSONObject matcher = new BasicBSONObject();
            if (bucketId != null) {
                matcher.put(UploadMeta.META_BUCKET_ID, bucketId);
            }
            if (objectName != null) {
                matcher.put(UploadMeta.META_KEY_NAME, objectName);
            }
            matcher.put(UploadMeta.META_UPLOAD_ID, uploadId);

            Integer flag = 0;
            if (forUpdate){
                flag = DBQuery.FLG_QUERY_FOR_UPDATE;
            }

            BSONObject hint = new BasicBSONObject();
            hint.put("", "");

            BSONObject queryResult = cl.queryOne(matcher, null, null, hint, flag);
            if (queryResult == null){
                return null;
            }else {
                return new UploadMeta(queryResult);
            }
        } catch (Exception e){
            logger.warn("query upload failed. uploadId:{}, error:{}", uploadId, e.getMessage());
            throw e;
        } finally {
            if (connection == null){
                sdbDatasourceWrapper.releaseSequoiadb(sdb);
            }
        }
    }

    @Override
    public void deleteUploadByUploadId(ConnectionDao connection, long bucketId,
                                       String objectName, long uploadId) throws S3ServerException {
        Sequoiadb sdb = null;
        try {
            if (connection != null) {
                sdb = ((SdbConnectionDao) connection).getConnection();
            } else {
                sdb = sdbDatasourceWrapper.getSequoiadb();
            }

            CollectionSpace cs = sdb.getCollectionSpace(config.getMetaCsName());
            DBCollection cl = cs.getCollection(DaoCollectionDefine.UPLOAD_LIST);

            BSONObject matcher = new BasicBSONObject();
            matcher.put(UploadMeta.META_BUCKET_ID, bucketId);
            matcher.put(UploadMeta.META_KEY_NAME, objectName);
            matcher.put(UploadMeta.META_UPLOAD_ID, uploadId);

            BSONObject hint = new BasicBSONObject();
            hint.put("", "");

            cl.delete(matcher, hint);
        } catch (Exception e){
            logger.error("delete upload failed. uploadId:{}", uploadId);
            throw e;
        } finally {
            if (connection == null){
                sdbDatasourceWrapper.releaseSequoiadb(sdb);
            }
        }
    }

    @Override
    public QueryDbCursor queryInvalidUploads() throws S3ServerException{
        Sequoiadb sdb = null;
        DBCursor dbCursor = null;
        try {
            sdb = sdbDatasourceWrapper.getSequoiadb();
            CollectionSpace cs = sdb.getCollectionSpace(config.getMetaCsName());
            DBCollection cl = cs.getCollection(DaoCollectionDefine.UPLOAD_LIST);

            BSONObject statusMatcher = new BasicBSONObject();
            statusMatcher.put(DBParamDefine.NOT_EQUAL, UploadMeta.UPLOAD_INIT);
            BSONObject matcher = new BasicBSONObject();
            matcher.put(UploadMeta.META_STATUS, statusMatcher);

            BSONObject hint = new BasicBSONObject();
            hint.put("", "");

            dbCursor = cl.query(matcher, null, null, hint, 0);
            return new SdbQueryDbCursor(sdb, dbCursor);
        } catch (Exception e){
            sdbBaseOperation.releaseDBCursor(dbCursor);
            sdbDatasourceWrapper.releaseSequoiadb(sdb);
            logger.error("query invalid upload list failed.");
            throw e;
        }
    }

    @Override
    public QueryDbCursor queryExceedUploads(long exceedTime) throws S3ServerException{
        Sequoiadb sdb = null;
        DBCursor dbCursor = null;
        try {
            sdb = sdbDatasourceWrapper.getSequoiadb();
            CollectionSpace cs = sdb.getCollectionSpace(config.getMetaCsName());
            DBCollection cl = cs.getCollection(DaoCollectionDefine.UPLOAD_LIST);

            BSONObject matcher = new BasicBSONObject();
            matcher.put(UploadMeta.META_STATUS, UploadMeta.UPLOAD_INIT);

            BSONObject timeMatcher = new BasicBSONObject();
            timeMatcher.put(DBParamDefine.LESS_THAN, exceedTime);
            matcher.put(UploadMeta.META_INIT_TIME, timeMatcher);

            BSONObject hint = new BasicBSONObject();
            hint.put("", "");

            dbCursor = cl.query(matcher, null, null, hint, 0);
            return new SdbQueryDbCursor(sdb, dbCursor);
        } catch (Exception e){
            sdbBaseOperation.releaseDBCursor(dbCursor);
            sdbDatasourceWrapper.releaseSequoiadb(sdb);
            logger.error("query invalid upload list failed.");
            throw e;
        }
    }

    @Override
    public QueryDbCursor queryUploadsByBucket(long bucketId, String prefix,
                                              String keyMarker, Long uploadMarker,
                                              Integer status) throws S3ServerException {
        Sequoiadb sdb = null;
        DBCursor dbCursor = null;
        try {
            sdb = sdbDatasourceWrapper.getSequoiadb();
            CollectionSpace cs = sdb.getCollectionSpace(config.getMetaCsName());
            DBCollection cl = cs.getCollection(DaoCollectionDefine.UPLOAD_LIST);

            BSONObject matcher = new BasicBSONObject();
            matcher.put(UploadMeta.META_BUCKET_ID, bucketId);
            matcher.put(UploadMeta.META_STATUS, status);
            BSONObject keyMatcher = new BasicBSONObject();
            if (prefix != null && prefix.length() > 0){
                keyMatcher.put(DBParamDefine.NOT_SMALL, prefix);
                String prefixEnd = prefix.substring(0,prefix.length()-1) + (char)(prefix.charAt(prefix.length()-1)+1);
                keyMatcher.put(DBParamDefine.LESS_THAN, prefixEnd);
            }
            if (keyMarker != null){
                if (uploadMarker != null) {
                    if(prefix == null || keyMarker.compareTo(prefix) > 0) {
                        keyMatcher.put(DBParamDefine.NOT_SMALL, keyMarker);
                    }
                }else{
                    keyMatcher.put(DBParamDefine.GREATER, keyMarker);
                }
            }
            if (!keyMatcher.isEmpty()) {
                matcher.put(UploadMeta.META_KEY_NAME, keyMatcher);
            }

            BSONObject orderBy = new BasicBSONObject();
            orderBy.put(UploadMeta.META_KEY_NAME, 1);
            orderBy.put(UploadMeta.META_UPLOAD_ID, 1);

            BSONObject hint = new BasicBSONObject();
            hint.put("", "");

            dbCursor = cl.query(matcher, null, null, hint, 0);
            return new SdbQueryDbCursor(sdb, dbCursor);
        } catch (Exception e){
            sdbBaseOperation.releaseDBCursor(dbCursor);
            sdbDatasourceWrapper.releaseSequoiadb(sdb);
            logger.error("query upload list failed. bucketId:{}", bucketId);
            throw e;
        }
    }

    @Override
    public void setUploadsStatus(long bucketId, Long uploadId, int status) throws S3ServerException {
        Sequoiadb sdb = null;
        try {
            sdb = sdbDatasourceWrapper.getSequoiadb();
            CollectionSpace cs = sdb.getCollectionSpace(config.getMetaCsName());
            DBCollection cl = cs.getCollection(DaoCollectionDefine.UPLOAD_LIST);

            BSONObject matcher = new BasicBSONObject();
            matcher.put(UploadMeta.META_BUCKET_ID, bucketId);
            if (uploadId != null) {
                matcher.put(UploadMeta.META_UPLOAD_ID, uploadId);
            }

            BSONObject modifyStatus = new BasicBSONObject();
            modifyStatus.put(UploadMeta.META_STATUS, status);
            BSONObject setUpdate = new BasicBSONObject();
            setUpdate.put(DBParamDefine.MODIFY_SET, modifyStatus);

            BSONObject hint = new BasicBSONObject();
            hint.put("", "");

            cl.update(matcher, setUpdate, hint);
        } catch (Exception e){
            logger.error("update upload status failed. status:{}", status);
            throw e;
        } finally {
            sdbDatasourceWrapper.releaseSequoiadb(sdb);
        }
    }

    private BSONObject convertMetaToBson(UploadMeta meta){
        BSONObject uploadMeta = new BasicBSONObject();
        uploadMeta.put(UploadMeta.META_BUCKET_ID, meta.getBucketId());
        uploadMeta.put(UploadMeta.META_KEY_NAME, meta.getKey());
        uploadMeta.put(UploadMeta.META_UPLOAD_ID, meta.getUploadId());
        uploadMeta.put(UploadMeta.META_INIT_TIME, meta.getLastModified());
        uploadMeta.put(UploadMeta.META_CS_NAME, meta.getCsName());
        uploadMeta.put(UploadMeta.META_CL_NAME, meta.getClName());
        uploadMeta.put(UploadMeta.META_LOB_ID, meta.getLobId());
        uploadMeta.put(UploadMeta.META_CACHE_CONTROL, meta.getCacheControl());
        uploadMeta.put(UploadMeta.META_CONTENT_DISPOSITION, meta.getContentDisposition());
        uploadMeta.put(UploadMeta.META_CONTENT_ENCODING, meta.getContentEncoding());
        uploadMeta.put(UploadMeta.META_CONTENT_LANGUAGE, meta.getContentLanguage());
        uploadMeta.put(UploadMeta.META_CONTENT_TYPE, meta.getContentType());
        uploadMeta.put(UploadMeta.META_EXPIRES, meta.getExpires());
        uploadMeta.put(UploadMeta.META_LIST, meta.getMetaList());
        uploadMeta.put(UploadMeta.META_STATUS, meta.getUploadStatus());

        return uploadMeta;
    }
}
