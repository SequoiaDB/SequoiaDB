package com.sequoias3.dao.sequoiadb;

import com.sequoiadb.base.*;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoias3.common.DBParamDefine;
import com.sequoias3.common.DelimiterStatus;
import com.sequoias3.config.SequoiadbConfig;
import com.sequoias3.core.Bucket;
import com.sequoias3.dao.BucketDao;
import com.sequoias3.dao.ConnectionDao;
import com.sequoias3.dao.DaoCollectionDefine;
import com.sequoias3.exception.S3Error;
import com.sequoias3.exception.S3ServerException;
import com.sequoias3.utils.DataFormatUtils;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Repository;

import java.util.ArrayList;
import java.util.List;

@Repository("BucketDao")
public class SequoiadbBucketDao implements BucketDao {
    private static final Logger logger = LoggerFactory.getLogger(SequoiadbBucketDao.class);

    @Autowired
    SdbDataSourceWrapper sdbDatasourceWrapper;

    @Autowired
    SequoiadbConfig config;

    @Autowired
    SdbBaseOperation sdbBaseOperation;

    @Override
    public void insertBucket(Bucket bucket) throws S3ServerException {
        Sequoiadb sdb = null;
        try {
            sdb = sdbDatasourceWrapper.getSequoiadb();
            CollectionSpace cs = sdb.getCollectionSpace(config.getMetaCsName());
            DBCollection cl = cs.getCollection(DaoCollectionDefine.BUCKET_LIST_COLLECTION);

            BSONObject newBucket = new BasicBSONObject();
            newBucket.put(Bucket.BUCKET_ID, bucket.getBucketId());
            newBucket.put(Bucket.BUCKET_NAME, bucket.getBucketName());
            newBucket.put(Bucket.BUCKET_OWNERID, bucket.getOwnerId());
            newBucket.put(Bucket.BUCKET_CREATETIME, bucket.getTimeMillis());
            newBucket.put(Bucket.BUCKET_VERSIONINGSTATUS, bucket.getVersioningStatus());
            newBucket.put(Bucket.BUCKET_REGION, bucket.getRegion());
            newBucket.put(Bucket.BUCKET_DELIMITER, bucket.getDelimiter());
            newBucket.put(Bucket.BUCKET_DELIMITER1,bucket.getDelimiter1());
            newBucket.put(Bucket.BUCKET_DELIMITER1CREATETIME, bucket.getDelimiter1CreateTime());
            newBucket.put(Bucket.BUCKET_DELIMITER1MODTIME, bucket.getDelimiter1ModTime());
            newBucket.put(Bucket.BUCKET_DELIMITER1STATUS, bucket.getDelimiter1Status());
            newBucket.put(Bucket.BUCKET_DELIMITER2, bucket.getDelimiter2());
            newBucket.put(Bucket.BUCKET_DELIMITER2CREATETIME, bucket.getDelimiter2CreateTime());
            newBucket.put(Bucket.BUCKET_DELIMITER2MODTIME, bucket.getDelimiter2ModTime());
            newBucket.put(Bucket.BUCKET_DELIMITER2STATUS, bucket.getDelimiter2Status());
            newBucket.put(Bucket.BUCKET_TASKID, bucket.getTaskID());
            newBucket.put(Bucket.BUCKET_PRIVATE, bucket.isPrivate());

            cl.insert(newBucket);
        }catch (BaseException e){
            if (e.getErrorType() == SDBError.SDB_IXM_DUP_KEY.name()) {
                throw new S3ServerException(S3Error.DAO_DUPLICATE_KEY, "Duplicate key.");
            } else {
                throw e;
            }
        }
        catch (Exception e) {
            logger.error("insertBucket failed. errorMessage = " + e.getMessage());
            throw e;
        }finally {
            sdbDatasourceWrapper.releaseSequoiadb(sdb);
        }
    }

    @Override
    public void deleteBucket(ConnectionDao connection, String bucketName) throws S3ServerException {
        Sequoiadb sdb = null;
        try {
            if (connection != null){
                sdb = ((SdbConnectionDao)connection).getConnection();
            }else {
                sdb = sdbDatasourceWrapper.getSequoiadb();
            }
            CollectionSpace cs = sdb.getCollectionSpace(config.getMetaCsName());
            DBCollection cl = cs.getCollection(DaoCollectionDefine.BUCKET_LIST_COLLECTION);

            BSONObject deleteBucket = new BasicBSONObject();
            deleteBucket.put(Bucket.BUCKET_NAME, bucketName);

            BSONObject hint = new BasicBSONObject();
            hint.put("", Bucket.NAME_INDEX);

            cl.delete(deleteBucket, hint);
        }catch (Exception e) {
            logger.error("deleteBucket failed. errorMessage = " + e.getMessage());
            throw e;
        }finally {
            if (connection == null) {
                sdbDatasourceWrapper.releaseSequoiadb(sdb);
            }
        }
    }

    @Override
    public Bucket getBucketByName(String bucketName) throws S3ServerException {
        Sequoiadb sdb = null;
        try {
            sdb = sdbDatasourceWrapper.getSequoiadb();
            CollectionSpace cs = sdb.getCollectionSpace(config.getMetaCsName());
            DBCollection cl = cs.getCollection(DaoCollectionDefine.BUCKET_LIST_COLLECTION);

            BSONObject matcher = new BasicBSONObject();
            matcher.put(Bucket.BUCKET_NAME, bucketName);

            BSONObject hint = new BasicBSONObject();
            hint.put("", Bucket.NAME_INDEX);

            BSONObject queryResult = cl.queryOne(matcher,null,null,hint,0);
            return convertBsonToBucket(queryResult);
        }catch (Exception e) {
            logger.error("getBucketByName failed. bucketName:" + bucketName + ", errorMessage = " + e.getMessage());
            throw e;
        }  finally {
            sdbDatasourceWrapper.releaseSequoiadb(sdb);
        }
    }

    @Override
    public Bucket getBucketById(ConnectionDao connection, long bucketId) throws S3ServerException {
        Sequoiadb sdb = null;
        try {
            if (connection != null){
                sdb = ((SdbConnectionDao)connection).getConnection();
            }else {
                sdb = sdbDatasourceWrapper.getSequoiadb();
            }
            CollectionSpace cs = sdb.getCollectionSpace(config.getMetaCsName());
            DBCollection cl = cs.getCollection(DaoCollectionDefine.BUCKET_LIST_COLLECTION);

            BSONObject matcher = new BasicBSONObject();
            matcher.put(Bucket.BUCKET_ID, bucketId);

            BSONObject hint = new BasicBSONObject();
            hint.put("", Bucket.ID_INDEX);

            BSONObject queryResult = cl.queryOne(matcher,null,null,hint,0);
            return convertBsonToBucket(queryResult);
        } catch (Exception e) {
            logger.error("getBucketById failed. bucketId:" + bucketId + ", errorMessage = " + e.getMessage());
            throw e;
        } finally {
            if (connection == null) {
                sdbDatasourceWrapper.releaseSequoiadb(sdb);
            }
        }
    }

    @Override
    public List<Bucket> getBucketListByOwnerID(long ownerId) throws S3ServerException {
        Sequoiadb sdb = null;
        DBCursor cursor = null;
        ArrayList<Bucket> bucketList = new ArrayList<Bucket>();
        try {
            sdb = sdbDatasourceWrapper.getSequoiadb();
            CollectionSpace cs = sdb.getCollectionSpace(config.getMetaCsName());
            DBCollection cl = cs.getCollection(DaoCollectionDefine.BUCKET_LIST_COLLECTION);

            BSONObject matcher = new BasicBSONObject();
            matcher.put(Bucket.BUCKET_OWNERID, ownerId);

            BSONObject orderBy = new BasicBSONObject();
            orderBy.put(Bucket.BUCKET_NAME, 1);

            cursor = cl.query(matcher, null,orderBy,null);
            while (cursor.hasNext()){
                BSONObject record = cursor.getNext();
                Bucket bucket = convertBsonToBucket(record);
                bucketList.add(bucket);
            }

            return bucketList;
        }catch (Exception e) {
            logger.error("getBucketListByOwnerID failed. errorMessage = " + e.getMessage());
            throw e;
        } finally {
            sdbBaseOperation.releaseDBCursor(cursor);
            sdbDatasourceWrapper.releaseSequoiadb(sdb);
        }
    }

    @Override
    public List<Bucket> getBucketListByRegion(ConnectionDao connection, String regionName) throws S3ServerException {
        Sequoiadb sdb = null;
        DBCursor cursor = null;
        ArrayList<Bucket> bucketList = new ArrayList<Bucket>();
        try {
            if (connection != null){
                sdb = ((SdbConnectionDao)connection).getConnection();
            }else {
                sdb = sdbDatasourceWrapper.getSequoiadb();
            }
            CollectionSpace cs = sdb.getCollectionSpace(config.getMetaCsName());
            DBCollection cl = cs.getCollection(DaoCollectionDefine.BUCKET_LIST_COLLECTION);

            BSONObject matcher = new BasicBSONObject();
            matcher.put(Bucket.BUCKET_REGION, regionName);

            BSONObject orderBy = new BasicBSONObject();
            orderBy.put(Bucket.BUCKET_NAME, 1);

            cursor = cl.query(matcher, null,orderBy,null);
            while (cursor.hasNext()){
                BSONObject record = cursor.getNext();
                Bucket bucket = convertBsonToBucket(record);
                bucketList.add(bucket);
            }
            return bucketList;
        }catch (Exception e) {
            logger.error("getBucketListByRegion failed. errorMessage = " + e.getMessage());
            throw e;
        } finally {
            sdbBaseOperation.releaseDBCursor(cursor);
            if (null == connection){
                sdbDatasourceWrapper.releaseSequoiadb(sdb);
            }
        }
    }

    @Override
    public List<Bucket> getBucketListByDelimiterStatus(DelimiterStatus status, Long overTime) throws S3ServerException {
        Sequoiadb sdb = null;
        DBCursor cursor = null;
        ArrayList<Bucket> bucketList = new ArrayList<Bucket>();
        try {
            sdb = sdbDatasourceWrapper.getSequoiadb();
            CollectionSpace cs = sdb.getCollectionSpace(config.getMetaCsName());
            DBCollection cl = cs.getCollection(DaoCollectionDefine.BUCKET_LIST_COLLECTION);

            BSONObject timeMatcher = new BasicBSONObject();
            timeMatcher.put(DBParamDefine.LESS_THAN, System.currentTimeMillis() - overTime);

            List<BSONObject> matcher = new ArrayList<>();
            BSONObject matcher1 = new BasicBSONObject();
            matcher1.put(Bucket.BUCKET_DELIMITER1STATUS, status.getName());
            matcher1.put(Bucket.BUCKET_DELIMITER1MODTIME, timeMatcher);

            BSONObject matcher2 = new BasicBSONObject();
            matcher2.put(Bucket.BUCKET_DELIMITER2STATUS, status.getName());
            matcher2.put(Bucket.BUCKET_DELIMITER2MODTIME, timeMatcher);

            matcher.add(matcher1);
            matcher.add(matcher2);

            BSONObject matcherMulti = new BasicBSONObject();
            matcherMulti.put(DBParamDefine.OR, matcher);

            cursor = cl.query(matcherMulti, null, null,null);
            while (cursor.hasNext()){
                BSONObject record = cursor.getNext();
                Bucket bucket = convertBsonToBucket(record);
                bucketList.add(bucket);
            }
            return bucketList;
        }catch (Exception e) {
            logger.error("getBucketListByRegion failed. errorMessage = " + e.getMessage());
            throw e;
        } finally {
            sdbBaseOperation.releaseDBCursor(cursor);
            sdbDatasourceWrapper.releaseSequoiadb(sdb);
        }
    }

    @Override
    public long getBucketNumber(long ownerID) throws S3ServerException{
        Sequoiadb sdb = null;
        try {
            sdb = sdbDatasourceWrapper.getSequoiadb();
            CollectionSpace cs = sdb.getCollectionSpace(config.getMetaCsName());
            DBCollection cl = cs.getCollection(DaoCollectionDefine.BUCKET_LIST_COLLECTION);

            BSONObject matcher = new BasicBSONObject();
            matcher.put(Bucket.BUCKET_OWNERID, ownerID);

            return cl.getCount(matcher);
        }catch (Exception e) {
            logger.error("getBucketNumber failed. errorMessage = " + e.getMessage());
            throw e;
        }finally {
            sdbDatasourceWrapper.releaseSequoiadb(sdb);
        }
    }

    @Override
    public void updateBucketVersioning(String bucketName, String status) throws S3ServerException {
        Sequoiadb sdb = null;
        try {
            sdb = sdbDatasourceWrapper.getSequoiadb();
            CollectionSpace cs = sdb.getCollectionSpace(config.getMetaCsName());
            DBCollection cl = cs.getCollection(DaoCollectionDefine.BUCKET_LIST_COLLECTION);

            BSONObject matcher = new BasicBSONObject();
            matcher.put(Bucket.BUCKET_NAME, bucketName);

            BSONObject modifier = new BasicBSONObject();
            if (status != null){
                modifier.put(Bucket.BUCKET_VERSIONINGSTATUS, status);
            }

            BSONObject setModifier = new BasicBSONObject();
            setModifier.put(DBParamDefine.MODIFY_SET, modifier);

            BSONObject hint = new BasicBSONObject();
            hint.put("", Bucket.NAME_INDEX);

            cl.update(matcher, setModifier, hint);
            return;
        }catch (Exception e) {
            logger.error("updateBucket failed. errorMessage = " + e.getMessage());
            throw e;
        }  finally {
            sdbDatasourceWrapper.releaseSequoiadb(sdb);
        }
    }

    @Override
    public void updateBucketDelimiter(ConnectionDao connection, String bucketName, Bucket bucket)
            throws S3ServerException {
        Sequoiadb sdb = null;
        try{
            if (connection != null) {
                sdb = ((SdbConnectionDao) connection).getConnection();
            }else {
                sdb = sdbDatasourceWrapper.getSequoiadb();
            }
            CollectionSpace cs = sdb.getCollectionSpace(config.getMetaCsName());
            DBCollection    cl = cs.getCollection(DaoCollectionDefine.BUCKET_LIST_COLLECTION);

            BSONObject matcher = new BasicBSONObject();
            matcher.put(Bucket.BUCKET_NAME, bucketName);

            BSONObject update = new BasicBSONObject();
            if (bucket.getDelimiter() != null) {
                update.put(Bucket.BUCKET_DELIMITER, bucket.getDelimiter());
            }
            if (bucket.getDelimiter1() != null) {
                update.put(Bucket.BUCKET_DELIMITER1, bucket.getDelimiter1());
            }
            if (bucket.getDelimiter1CreateTime() != null) {
                update.put(Bucket.BUCKET_DELIMITER1CREATETIME, bucket.getDelimiter1CreateTime());
            }
            if (bucket.getDelimiter1ModTime() != null) {
                update.put(Bucket.BUCKET_DELIMITER1MODTIME, bucket.getDelimiter1ModTime());
            }
            if (bucket.getDelimiter1Status() != null) {
                update.put(Bucket.BUCKET_DELIMITER1STATUS, bucket.getDelimiter1Status());
            }
            if (bucket.getDelimiter2() != null) {
                update.put(Bucket.BUCKET_DELIMITER2, bucket.getDelimiter2());
            }
            if (bucket.getDelimiter2CreateTime() != null) {
                update.put(Bucket.BUCKET_DELIMITER2CREATETIME, bucket.getDelimiter2CreateTime());
            }
            if (bucket.getDelimiter2ModTime() != null) {
                update.put(Bucket.BUCKET_DELIMITER2MODTIME, bucket.getDelimiter2ModTime());
            }
            if (bucket.getDelimiter2Status() != null) {
                update.put(Bucket.BUCKET_DELIMITER2STATUS, bucket.getDelimiter2Status());
            }
            if (bucket.getTaskID() != null){
                update.put(Bucket.BUCKET_TASKID, bucket.getTaskID());
            }

            BSONObject upSet = new BasicBSONObject();
            upSet.put(DBParamDefine.MODIFY_SET, update);

            BSONObject hint = new BasicBSONObject();
            hint.put("", Bucket.NAME_INDEX);

            cl.update(matcher, upSet, hint);
        }catch (Exception e){
            logger.error("update bucket delimiter failed. bucketNme:{}", bucketName);
            throw e;
        }finally {
            if (connection == null){
                sdbDatasourceWrapper.releaseSequoiadb(sdb);
            }
        }
    }

    @Override
    public void updateBucketAcl(ConnectionDao connection, String bucketName, Long aclId, Boolean isPrivate) throws S3ServerException {
        Sequoiadb sdb = null;
        try{
            if (connection != null) {
                sdb = ((SdbConnectionDao) connection).getConnection();
            }else {
                sdb = sdbDatasourceWrapper.getSequoiadb();
            }
            CollectionSpace cs = sdb.getCollectionSpace(config.getMetaCsName());
            DBCollection cl = cs.getCollection(DaoCollectionDefine.BUCKET_LIST_COLLECTION);

            BSONObject matcher = new BasicBSONObject();
            matcher.put(Bucket.BUCKET_NAME, bucketName);
            //如果 aclId is null，则将aclId为null作为条件
            //如果 aclId is not null，则将aclId作为修改的字段
            if (aclId == null){
                BSONObject isNull = new BasicBSONObject();
                isNull.put(DBParamDefine.IS_NULL, 1);
                matcher.put(Bucket.BUCKET_ACLID, isNull);
            }

            BSONObject modifier = new BasicBSONObject();
            if (aclId != null){
                modifier.put(Bucket.BUCKET_ACLID, aclId);
            }
            if (isPrivate != null) {
                modifier.put(Bucket.BUCKET_PRIVATE, isPrivate);
            }

            BSONObject setModifier = new BasicBSONObject();
            setModifier.put(DBParamDefine.MODIFY_SET, modifier);

            BSONObject hint = new BasicBSONObject();
            hint.put("", Bucket.NAME_INDEX);

            cl.update(matcher, setModifier, hint);
            return;
        }catch (Exception e) {
            logger.error("update bucket acl failed. errorMessage = " + e.getMessage());
            throw e;
        }  finally {
            if (connection == null){
                sdbDatasourceWrapper.releaseSequoiadb(sdb);
            }
        }
    }

    @Override
    public void cleanBucketDelimiter(ConnectionDao connection, String bucketName, int delimiter)
            throws S3ServerException{
        try {
            Sequoiadb sdb = ((SdbConnectionDao)connection).getConnection();
            CollectionSpace cs = sdb.getCollectionSpace(config.getMetaCsName());
            DBCollection    cl = cs.getCollection(DaoCollectionDefine.BUCKET_LIST_COLLECTION);

            BSONObject matcher = new BasicBSONObject();
            matcher.put(Bucket.BUCKET_NAME, bucketName);

            BSONObject update = new BasicBSONObject();
            if (delimiter == 1) {
                update.put(Bucket.BUCKET_DELIMITER1, null);
                update.put(Bucket.BUCKET_DELIMITER1CREATETIME, null);
                update.put(Bucket.BUCKET_DELIMITER1MODTIME, null);
                update.put(Bucket.BUCKET_DELIMITER1STATUS, null);
                update.put(Bucket.BUCKET_TASKID, null);
            }else {
                update.put(Bucket.BUCKET_DELIMITER2, null);
                update.put(Bucket.BUCKET_DELIMITER2CREATETIME, null);
                update.put(Bucket.BUCKET_DELIMITER2MODTIME, null);
                update.put(Bucket.BUCKET_DELIMITER2STATUS, null);
                update.put(Bucket.BUCKET_TASKID, null);
            }
            BSONObject unSet = new BasicBSONObject();
            unSet.put(DBParamDefine.MODIFY_UNSET, update);

            BSONObject hint = new BasicBSONObject();
            hint.put("", Bucket.NAME_INDEX);

            cl.update(matcher, unSet, hint);
        }catch (Exception e){
            logger.error("clear bucket delimiter:{} failed. bucketName:{}", delimiter, bucketName);
            throw e;
        }
    }

    @Override
    public Bucket queryBucketForUpdate(ConnectionDao connection, String bucketName) throws S3ServerException {
        try {
            Sequoiadb sdb = ((SdbConnectionDao)connection).getConnection();
            CollectionSpace cs = sdb.getCollectionSpace(config.getMetaCsName());
            DBCollection cl = cs.getCollection(DaoCollectionDefine.BUCKET_LIST_COLLECTION);

            BSONObject matcher = new BasicBSONObject();
            matcher.put(Bucket.BUCKET_NAME, bucketName);

            BSONObject hint = new BasicBSONObject();
            hint.put("", Bucket.NAME_INDEX);

            BSONObject queryResult = cl.queryOne(matcher,null,null,hint, DBQuery.FLG_QUERY_FOR_UPDATE);
            return convertBsonToBucket(queryResult);
        }catch (Exception e) {
            logger.error("getBucketByName failed. errorMessage = " + e.getMessage());
            throw e;
        }
    }

    private Bucket convertBsonToBucket(BSONObject bsonObject) {
        if (null == bsonObject) {
            return null;
        }
        Bucket bucket = new Bucket();
        bucket.setBucketId((long)bsonObject.get(Bucket.BUCKET_ID));
        bucket.setBucketName(bsonObject.get(Bucket.BUCKET_NAME).toString());
        bucket.setOwnerId((long)bsonObject.get(Bucket.BUCKET_OWNERID));
        bucket.setTimeMillis((long)bsonObject.get(Bucket.BUCKET_CREATETIME));
        bucket.setFormatDate(DataFormatUtils.formatDate((long)bsonObject.get(Bucket.BUCKET_CREATETIME)));
        bucket.setVersioningStatus(bsonObject.get(Bucket.BUCKET_VERSIONINGSTATUS).toString());
        if (bsonObject.get(Bucket.BUCKET_REGION) != null){
            bucket.setRegion(bsonObject.get(Bucket.BUCKET_REGION).toString());
        }
        bucket.setDelimiter((int)bsonObject.get(Bucket.BUCKET_DELIMITER));
        if (bsonObject.get(Bucket.BUCKET_DELIMITER1) != null){
            bucket.setDelimiter1(bsonObject.get(Bucket.BUCKET_DELIMITER1).toString());
        }
        if (bsonObject.get(Bucket.BUCKET_DELIMITER1CREATETIME) != null){
            bucket.setDelimiter1CreateTime((long)bsonObject.get(Bucket.BUCKET_DELIMITER1CREATETIME));
        }
        if (bsonObject.get(Bucket.BUCKET_DELIMITER1MODTIME) != null){
            bucket.setDelimiter1ModTime((long)bsonObject.get(Bucket.BUCKET_DELIMITER1MODTIME));
        }
        if (bsonObject.get(Bucket.BUCKET_DELIMITER1STATUS) != null){
            bucket.setDelimiter1Status(bsonObject.get(Bucket.BUCKET_DELIMITER1STATUS).toString());
        }
        if (bsonObject.get(Bucket.BUCKET_DELIMITER2) != null){
            bucket.setDelimiter2(bsonObject.get(Bucket.BUCKET_DELIMITER2).toString());
        }
        if (bsonObject.get(Bucket.BUCKET_DELIMITER2CREATETIME) != null){
            bucket.setDelimiter2CreateTime((long)bsonObject.get(Bucket.BUCKET_DELIMITER2CREATETIME));
        }
        if (bsonObject.get(Bucket.BUCKET_DELIMITER2MODTIME) != null){
            bucket.setDelimiter2ModTime((long)bsonObject.get(Bucket.BUCKET_DELIMITER2MODTIME));
        }
        if (bsonObject.get(Bucket.BUCKET_DELIMITER2STATUS) != null){
            bucket.setDelimiter2Status(bsonObject.get(Bucket.BUCKET_DELIMITER2STATUS).toString());
        }
        if (bsonObject.get(Bucket.BUCKET_TASKID) != null){
            bucket.setTaskID((long)bsonObject.get(Bucket.BUCKET_TASKID));
        }
        if (bsonObject.get(Bucket.BUCKET_ACLID) != null){
            bucket.setAclId((long) bsonObject.get(Bucket.BUCKET_ACLID));
        }
        if (bsonObject.get(Bucket.BUCKET_PRIVATE) != null){
            bucket.setPrivate((boolean) bsonObject.get(Bucket.BUCKET_PRIVATE));
        }else {
            bucket.setPrivate(true);
        }
        return bucket;
    }
}