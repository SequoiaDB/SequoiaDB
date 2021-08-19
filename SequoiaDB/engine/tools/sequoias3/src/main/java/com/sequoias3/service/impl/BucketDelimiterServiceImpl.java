package com.sequoias3.service.impl;

import com.sequoias3.common.DBParamDefine;
import com.sequoias3.common.DelimiterStatus;
import com.sequoias3.core.*;
import com.sequoias3.dao.*;
import com.sequoias3.exception.S3Error;
import com.sequoias3.exception.S3ServerException;
import com.sequoias3.model.DelimiterConfiguration;
import com.sequoias3.service.BucketDelimiterService;
import com.sequoias3.service.BucketService;
import com.sequoias3.taskmanager.DelimiterQueue;
import com.sequoias3.utils.DirUtils;
import org.bson.BSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;

@Service
public class BucketDelimiterServiceImpl implements BucketDelimiterService {
    private static final Logger logger = LoggerFactory.getLogger(BucketDelimiterServiceImpl.class);

    @Autowired
    BucketDao bucketDao;

    @Autowired
    BucketService bucketService;

    @Autowired
    DaoMgr daoMgr;

    @Autowired
    Transaction transaction;

    @Autowired
    IDGeneratorDao idGenerator;

    @Autowired
    TaskDao taskDao;

    @Autowired
    DirDao dirDao;

    @Autowired
    MetaDao metaDao;

    @Autowired
    RegionDao regionDao;

    @Autowired
    DelimiterQueue delimiterQueue;

    @Override
    public void putBucketDelimiter(long ownerID, String bucketName, String delimiter)
            throws S3ServerException {
        bucketService.getBucket(ownerID, bucketName);
        Bucket bucket;
        long taskId;
        String newParentIdName;
        ConnectionDao connectionA = daoMgr.getConnectionDao();
        transaction.begin(connectionA);
        try{
            bucket = bucketDao.queryBucketForUpdate(connectionA, bucketName);

            //check delimiter status
            if (bucket.getDelimiter() == 1){
                DelimiterStatus status = DelimiterStatus.getDelimiterStatus(bucket.getDelimiter1Status());
                if (status != DelimiterStatus.NORMAL){
                    throw new S3ServerException(S3Error.BUCKET_DELIMITER_NOT_STABLE, "Delimiter is not stable.");
                }
                if (delimiter.equals(bucket.getDelimiter1())){
                    transaction.rollback(connectionA);
                    return;
                }
                if (bucket.getDelimiter2() != null){
                    bucket.setDelimiter2Status(DelimiterStatus.DELETING.getName());
                    bucket.setDelimiter2ModTime(System.currentTimeMillis());
                    bucketDao.updateBucketDelimiter(connectionA, bucketName, bucket);
                    transaction.commit(connectionA);
                    delimiterQueue.addBucketName(bucketName);
                    throw new S3ServerException(S3Error.BUCKET_DELIMITER_NOT_STABLE, "Delimiter is not stable.");
                }
            }else {
                DelimiterStatus status = DelimiterStatus.getDelimiterStatus(bucket.getDelimiter2Status());
                if (status != DelimiterStatus.NORMAL){
                    throw new S3ServerException(S3Error.BUCKET_DELIMITER_NOT_STABLE, "Delimiter is not stable.");
                }
                if (delimiter.equals(bucket.getDelimiter2())){
                    transaction.rollback(connectionA);
                    return;
                }
                if (bucket.getDelimiter1() != null){
                    bucket.setDelimiter1Status(DelimiterStatus.DELETING.getName());
                    bucket.setDelimiter1ModTime(System.currentTimeMillis());
                    bucketDao.updateBucketDelimiter(connectionA, bucketName, bucket);
                    transaction.commit(connectionA);
                    delimiterQueue.addBucketName(bucketName);
                    throw new S3ServerException(S3Error.BUCKET_DELIMITER_NOT_STABLE, "Delimiter is not stable");
                }
            }

            //get taskId
            taskId = idGenerator.getNewId(IDGenerator.TYPE_TASK);

            // modify delimiter ,status, modtime,
            Bucket tmpBucket = new Bucket();
            if (bucket.getDelimiter() == 1) {
                tmpBucket.setDelimiter(2);
                tmpBucket.setDelimiter1Status(DelimiterStatus.SUSPENDED.getName());
                tmpBucket.setDelimiter1ModTime(System.currentTimeMillis());
                tmpBucket.setDelimiter2(delimiter);
                tmpBucket.setDelimiter2Status(DelimiterStatus.CREATING.getName());
                tmpBucket.setDelimiter2CreateTime(System.currentTimeMillis());
                tmpBucket.setDelimiter2ModTime(System.currentTimeMillis());
                tmpBucket.setTaskID(taskId);
                newParentIdName = ObjectMeta.META_PARENTID2;
            }else {
                tmpBucket.setDelimiter(1);
                tmpBucket.setDelimiter2Status(DelimiterStatus.SUSPENDED.getName());
                tmpBucket.setDelimiter2ModTime(System.currentTimeMillis());
                tmpBucket.setDelimiter1(delimiter);
                tmpBucket.setDelimiter1Status(DelimiterStatus.CREATING.getName());
                tmpBucket.setDelimiter1CreateTime(System.currentTimeMillis());
                tmpBucket.setDelimiter1ModTime(System.currentTimeMillis());
                tmpBucket.setTaskID(taskId);
                newParentIdName = ObjectMeta.META_PARENTID1;
            }

            bucketDao.updateBucketDelimiter(connectionA, bucketName, tmpBucket);

            // insert taskId
            taskDao.insertTaskId(connectionA, taskId);
            // commit
            transaction.commit(connectionA);
            logger.info("create a task of update bucket delimiter." +
                            " bucketName={}, bucketId={}, taskId={}",
                    bucketName, bucket.getBucketId(), taskId);
        }catch (S3ServerException e){
            transaction.rollback(connectionA);
            throw e;
        }catch (Exception e){
            transaction.rollback(connectionA);
            throw new S3ServerException(S3Error.BUCKET_DELIMITER_PUT_FAILED, "Modify bucket delimiter failed.", e);
        }finally {
            daoMgr.releaseConnectionDao(connectionA);
        }

        ConnectionDao connectionB = daoMgr.getConnectionDao();
        transaction.begin(connectionB);
        try{
            //lock taskId
            taskDao.queryTaskId(connectionB, taskId);

            Region region = null;
            if (bucket.getRegion() != null) {
                region = regionDao.queryRegion(bucket.getRegion());
            }
            String metaCsName    = regionDao.getMetaCurCSName(region);
            String metaClName    = regionDao.getMetaCurCLName(region);

            String startAfter = null;
            int tryTime = DBParamDefine.DB_DUPLICATE_MAX_TIME;
            while (tryTime > 0) {
                tryTime--;
                ConnectionDao connectionC = daoMgr.getConnectionDao();
                transaction.begin(connectionC);
                try {
                    QueryDbCursor queryDbCursor = metaDao.queryMetaByBucketForUpdate(connectionC,
                            metaCsName, metaClName, bucket.getBucketId(), null,
                            startAfter, 100);
                    if (queryDbCursor == null || !queryDbCursor.hasNext()) {
                        transaction.rollback(connectionC);
                        break;
                    }

                    String objectName = null;
                    while (queryDbCursor.hasNext()) {
                        BSONObject record = queryDbCursor.getNext();
                        objectName = record.get(ObjectMeta.META_KEY_NAME).toString();
                        updateDirForObject(connectionC, metaCsName, metaClName, bucket,
                                delimiter, newParentIdName, objectName, region);
                    }

                    transaction.commit(connectionC);
                    startAfter = objectName;
                }catch (S3ServerException e) {
                    transaction.rollback(connectionC);
                    if (e.getError().getErrIndex() == S3Error.DAO_DUPLICATE_KEY.getErrIndex() && tryTime > 0) {
                        continue;
                    } else {
                        throw e;
                    }
                } catch (Exception e) {
                    transaction.rollback(connectionC);
                    throw e;
                } finally {
                    daoMgr.releaseConnectionDao(connectionC);
                }
            }

            //modify delimiter
            Bucket newBucket = new Bucket();
            if (bucket.getDelimiter() == 1) {
                newBucket.setDelimiter2Status(DelimiterStatus.NORMAL.getName());
                newBucket.setDelimiter2ModTime(System.currentTimeMillis());
                newBucket.setDelimiter1Status(DelimiterStatus.TOBEDELETE.getName());
                newBucket.setDelimiter1ModTime(System.currentTimeMillis());
            }else {
                newBucket.setDelimiter1Status(DelimiterStatus.NORMAL.getName());
                newBucket.setDelimiter1ModTime(System.currentTimeMillis());
                newBucket.setDelimiter2Status(DelimiterStatus.TOBEDELETE.getName());
                newBucket.setDelimiter2ModTime(System.currentTimeMillis());
            }
            bucketDao.updateBucketDelimiter(connectionB, bucketName, newBucket);
            //commit
            transaction.commit(connectionB);
        }catch (S3ServerException e){
            transaction.rollback(connectionB);
            //失败将禁用的状态恢复为normal，creating改为deleting
            changeCreatingToDeleting(bucketName, taskId);
            throw e;
        }catch (Exception e){
            transaction.rollback(connectionB);
            //失败将禁用的状态恢复为normal，creating改为deleting
            changeCreatingToDeleting(bucketName, taskId);
            throw new S3ServerException(S3Error.BUCKET_DELIMITER_PUT_FAILED, "Modify bucket delimiter failed.", e);
        }finally {
            daoMgr.releaseConnectionDao(connectionB);
        }
    }

    private void changeCreatingToDeleting(String bucketName, Long taskId)throws S3ServerException{
        ConnectionDao connectionD = daoMgr.getConnectionDao();
        transaction.begin(connectionD);
        try{
            Bucket recoverBucket = null;
            taskDao.queryTaskId(connectionD, taskId);
            Bucket bucket = bucketDao.queryBucketForUpdate(connectionD, bucketName);
            if (bucket.getDelimiter() == 1){
                if (bucket.getDelimiter1Status().equals(DelimiterStatus.CREATING.getName())){
                    recoverBucket = new Bucket();
                    recoverBucket.setDelimiter(2);
                    recoverBucket.setDelimiter1Status(DelimiterStatus.DELETING.getName());
                    recoverBucket.setDelimiter1ModTime(System.currentTimeMillis());
                    recoverBucket.setDelimiter2Status(DelimiterStatus.NORMAL.getName());
                    recoverBucket.setDelimiter2ModTime(System.currentTimeMillis());
                }
            }
            if (bucket.getDelimiter() == 2){
                if (bucket.getDelimiter2Status().equals(DelimiterStatus.CREATING.getName())){
                    recoverBucket = new Bucket();
                    recoverBucket.setDelimiter(1);
                    recoverBucket.setDelimiter2Status(DelimiterStatus.DELETING.getName());
                    recoverBucket.setDelimiter2ModTime(System.currentTimeMillis());
                    recoverBucket.setDelimiter1Status(DelimiterStatus.NORMAL.getName());
                    recoverBucket.setDelimiter1ModTime(System.currentTimeMillis());
                }
            }

            if (recoverBucket != null) {
                bucketDao.updateBucketDelimiter(connectionD, bucketName, recoverBucket);
            }

            transaction.commit(connectionD);
        }catch (S3ServerException e){
            transaction.rollback(connectionD);
            throw e;
        }catch (Exception e){
            transaction.rollback(connectionD);
            throw new S3ServerException(S3Error.BUCKET_DELIMITER_PUT_FAILED, "recover bucket delimiter failed.", e);
        }finally {
            daoMgr.releaseConnectionDao(connectionD);
        }
    }

    @Override
    public DelimiterConfiguration getBucketDelimiter(long ownerID, String bucketName, String encodingType)
            throws S3ServerException {
        try {
            Bucket bucket = bucketService.getBucket(ownerID, bucketName);

            DelimiterConfiguration result;
            if (bucket.getDelimiter() == 1) {
                result = new DelimiterConfiguration(bucket.getDelimiter1(), bucket.getDelimiter1Status(), encodingType);
            } else {
                result = new DelimiterConfiguration(bucket.getDelimiter2(), bucket.getDelimiter2Status(), encodingType);
            }
            return result;
        }catch (S3ServerException e){
            throw e;
        }catch (Exception e){
            throw new S3ServerException(S3Error.BUCKET_DELIMITER_GET_FAILED, "get delimiter fail", e);
        }
    }

    private void updateDirForObject(ConnectionDao connection, String metaCsName, String metaClName, Bucket bucket,
                                    String delimiter, String parentIdName, String objectName, Region region)
            throws S3ServerException{
        //phase object name by delimiter get dir id
        Long dirId;
        String dirName = DirUtils.getDir(objectName, delimiter);
        if (dirName != null){
            Dir dir = dirDao.queryDir(connection, metaCsName, bucket.getBucketId(),
                    delimiter, dirName, true);
            if (dir != null){
                dirId = dir.getID();
            }else {
                dirId = idGenerator.getNewId(IDGenerator.TYPE_PARENTID);
                Dir newDir = new Dir(bucket.getBucketId(), delimiter, dirName, dirId);
                dirDao.insertDir(connection, metaCsName, newDir, region);
            }
        }else {
            dirId = 0L;
        }

        //update meta in cur meta cl
        metaDao.updateMetaParentId(connection, metaCsName, metaClName,
                bucket.getBucketId(), objectName, parentIdName, dirId);
    }
}
