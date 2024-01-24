package com.sequoias3.service.impl;

import com.sequoias3.common.DBParamDefine;
import com.sequoias3.common.DelimiterStatus;
import com.sequoias3.common.VersioningStatusType;
import com.sequoias3.config.BucketConfig;
import com.sequoias3.core.*;
import com.sequoias3.dao.*;
import com.sequoias3.exception.S3Error;
import com.sequoias3.exception.S3ServerException;
import com.sequoias3.model.GetServiceResult;
import com.sequoias3.model.LocationConstraint;
import com.sequoias3.service.BucketService;
import com.sequoias3.service.ObjectService;
import org.bson.BSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;

import java.util.List;

@Service
public class BucketServiceImpl implements BucketService {
    private static final Logger logger = LoggerFactory.getLogger(BucketServiceImpl.class);

    @Autowired
    BucketDao bucketDao;

    @Autowired
    UserDao userDao;

    @Autowired
    BucketConfig bucketConfig;

    @Autowired
    ObjectService objectService;

    @Autowired
    RegionDao regionDao;

    @Autowired
    DaoMgr daoMgr;

    @Autowired
    Transaction transaction;

    @Autowired
    IDGeneratorDao idGeneratorDao;

    @Autowired
    DirDao dirDao;

    @Autowired
    TaskDao taskDao;

    @Autowired
    UploadDao uploadDao;

    @Autowired
    MetaDao metaDao;

    @Autowired
    AclDao aclDao;

    @Override
    public void createBucket(long ownerID, String bucketName, String region) throws S3ServerException {
        int tryTime = DBParamDefine.DB_DUPLICATE_MAX_TIME;

        //check bucketname
        if (!isValidBucketName(bucketName)){
            throw new S3ServerException(S3Error.BUCKET_INVALID_BUCKETNAME,
                    "Invalid bucket name. bucket name = "+bucketName);
        }
        String newBucketName = bucketName.toLowerCase();
        while (tryTime > 0){
            tryTime--;
            ConnectionDao connection = daoMgr.getConnectionDao();
            transaction.begin(connection);
            try {
                //check duplicate bucket
                Bucket result = bucketDao.getBucketByName(newBucketName);
                if (null != result){
                    if (result.getOwnerId() == ownerID){
                        if (bucketConfig.getAllowreput()){
                            return;
                        }else {
                            throw new S3ServerException(S3Error.BUCKET_ALREADY_OWNED_BY_YOU,
                                    "Bucket already owned you. bucket name=" + bucketName);
                        }
                    }else {
                        throw new S3ServerException(S3Error.BUCKET_ALREADY_EXIST,
                                "Bucket already exist. bucket name="+bucketName);
                    }
                }

                if (region != null) {
                    Region regionCon = regionDao.queryForUpdateRegion(connection, region);
                    if (null == regionCon){
                        throw new S3ServerException(S3Error.REGION_NO_SUCH_REGION,
                                "no such region. regionName:"+region);
                    }
                }

                //check bucket number
                long bucketLimit = bucketConfig.getLimit();
                long bucketCount = bucketDao.getBucketNumber(ownerID);
                if (bucketCount >= bucketLimit){
                    throw new S3ServerException(S3Error.BUCKET_TOO_MANY_BUCKETS,
                            "You have attempted to create more buckets than allowed. bucket count="
                                    +bucketCount+", bucket limit="+bucketLimit);
                }

                //insert bucket
                Bucket bucket = new Bucket();
                bucket.setBucketId(idGeneratorDao.getNewId(IDGenerator.TYPE_BUCKET));
                bucket.setBucketName(newBucketName);
                bucket.setOwnerId(ownerID);
                bucket.setTimeMillis(System.currentTimeMillis());
                bucket.setVersioningStatus(VersioningStatusType.NONE.getName());
                bucket.setDelimiter(1);
                bucket.setDelimiter1(DBParamDefine.DB_AUTO_DELIMITER);
                bucket.setDelimiter1CreateTime(System.currentTimeMillis());
                bucket.setDelimiter1ModTime(System.currentTimeMillis());
                bucket.setDelimiter1Status(DelimiterStatus.NORMAL.getName());
                bucket.setRegion(region);
                bucket.setPrivate(true);
                bucketDao.insertBucket(bucket);
                logger.info("create bucket success. bucket name={}, bucketId={}",
                        bucketName, bucket.getBucketId());
                return;
            }catch (S3ServerException e) {
                logger.warn("Create bucket failed. bucket name = {}, error = {}", bucketName, e.getError().getErrIndex());
                if (e.getError().getErrIndex() == S3Error.DAO_DUPLICATE_KEY.getErrIndex() && tryTime > 0) {
                    continue;
                } else {
                    throw e;
                }
            }catch (Exception e){
                throw new S3ServerException(S3Error.BUCKET_CREATE_FAILED,
                        "create bucket failed. bucket name="+bucketName, e);
            }finally {
                transaction.rollback(connection);
                daoMgr.releaseConnectionDao(connection);
            }
        }
    }

    @Override
    public void deleteBucket(long ownerID, String bucketName) throws S3ServerException {
        try {
            String deleteName = bucketName.toLowerCase();

            //get and check bucket
            Bucket bucket = getBucket(ownerID, bucketName);

            Region region = null;
            if (bucket.getRegion() != null) {
                region = regionDao.queryRegion(bucket.getRegion());
            }

            //is bucket empty
            if (!objectService.isEmptyBucket(null, bucket, region)){
                throw new S3ServerException(S3Error.BUCKET_NOT_EMPTY,
                        "The bucket you tried to delete is not empty. bucket name = "+bucketName);
            }

            ConnectionDao connectionA = daoMgr.getConnectionDao();
            int transTimeout = connectionA.getTransTimeOut();
            transaction.begin(connectionA);
            try {
                bucket = bucketDao.queryBucketForUpdate(connectionA, deleteName);
                if (bucket == null){
                    throw new S3ServerException(S3Error.BUCKET_NOT_EXIST,
                            "The specified bucket does not exist. bucket name = "+bucketName);
                }
                // update nothing, only for x lock of the bucket
                bucketDao.updateBucketDelimiter(connectionA, deleteName, bucket);

                connectionA.setTransTimeOut(10);
                if (!objectService.isEmptyBucket(connectionA, bucket, region)){
                    throw new S3ServerException(S3Error.BUCKET_NOT_EMPTY,
                            "The bucket you tried to delete is not empty. bucket name = "+bucketName);
                }

                //delete bucket
                bucketDao.deleteBucket(connectionA, deleteName);
                transaction.commit(connectionA);
            } catch (S3ServerException e){
                transaction.rollback(connectionA);
                throw e;
            } catch (Exception e){
                transaction.rollback(connectionA);
                throw e;
            } finally {
                connectionA.setTransTimeOut(transTimeout);
                daoMgr.releaseConnectionDao(connectionA);
            }

            try {
                //delete acl
                if (bucket.getAclId() != null) {
                    aclDao.deleteAcl(null, bucket.getAclId());
                }

                //delete dir
                String metaCSName = regionDao.getMetaCurCSName(regionDao.queryRegion(bucket.getRegion()));
                dirDao.delete(null, metaCSName, bucket.getBucketId(), null, null);

                //task
                if (bucket.getTaskID() != null) {
                    taskDao.deleteTaskId(null, bucket.getTaskID());
                }

                //multi part upload
                uploadDao.setUploadsStatus(bucket.getBucketId(), null, UploadMeta.UPLOAD_ABORT);
            }catch (Exception e){
                logger.error("clean bucket failed, might something is left. " +
                        "bucketId:"+bucket.getBucketId(), e);
            }
        }catch (S3ServerException e) {
            throw e;
        }catch (Exception e){
            throw new S3ServerException(S3Error.BUCKET_DELETE_FAILED,
                    "delete bucket failed. bucket name = "+bucketName, e);
        }
    }

    @Override
    public GetServiceResult getService(User owner) throws S3ServerException {
        GetServiceResult result = new GetServiceResult();
        try {
            //get owner
            result.setOwner(owner);

            //get bucket list
            List<Bucket> bucketArrayList = bucketDao.getBucketListByOwnerID(owner.getUserId());
            if (bucketArrayList.size() > 0){
                result.setBuckets(bucketArrayList);
            }

            return result;
        }catch (S3ServerException e) {
            throw e;
        }catch (Exception e){
            throw new S3ServerException(S3Error.BUCKET_GET_SERVICE_FAILED,
                    "Get bucket list error. ownerID="+owner.getUserId(), e);
        }
    }

    @Override
    public Bucket getBucket(long ownerID, String bucketName)
            throws S3ServerException{
        Bucket bucket = bucketDao.getBucketByName(bucketName.toLowerCase());
        if (bucket == null){
            throw new S3ServerException(S3Error.BUCKET_NOT_EXIST,
                    "The specified bucket does not exist. bucket name = "+bucketName);
        }
        if (bucket.getOwnerId() != ownerID && bucket.isPrivate()){
            throw new S3ServerException(S3Error.ACCESS_DENIED,
                    "You can not access the specified bucket. bucket name = "+bucketName+", ownerID = "+ownerID);
        }

        return bucket;
    }

    @Override
    public void deleteBucketForce(Bucket bucket) throws S3ServerException {
        try {
            Region region = null;
            if (bucket.getRegion() != null) {
                region = regionDao.queryRegion(bucket.getRegion());
            }

            while (!objectService.isEmptyBucket(null, bucket, region)) {
                //delete objects in the bucket
                objectService.deleteObjectByBucket(bucket);
            }

            //delete bucket
            bucketDao.deleteBucket(null, bucket.getBucketName());

            try {
                while (!objectService.isEmptyBucket(null, bucket, region)) {
                    objectService.deleteObjectByBucket(bucket);
                }
            }catch (Exception e){
                logger.error("delete bucket force. clean objects failed. " +
                        "bucketName=" + bucket.getBucketName() + ", bucketId={}" + bucket.getBucketId(), e);
            }

            try {
                //delete dir
                String metaCSName = regionDao.getMetaCurCSName(regionDao.queryRegion(bucket.getRegion()));
                dirDao.delete(null, metaCSName, bucket.getBucketId(), null, null);
            } catch (Exception e){
                logger.error("delete bucket force. clean dir failed. " +
                        "bucketName=" + bucket.getBucketName() + ", bucketId={}" + bucket.getBucketId(), e);
            }

            try {
                //task
                if (bucket.getTaskID() != null) {
                    taskDao.deleteTaskId(null, bucket.getTaskID());
                }
            }catch (Exception e){
                logger.error("delete bucket force. clean task failed. " +
                        "bucketName=" + bucket.getBucketName() + ", bucketId={}" + bucket.getBucketId(), e);
            }

            try {
                //delete acl
                if (bucket.getAclId() != null) {
                    aclDao.deleteAcl(null, bucket.getAclId());
                }
            }catch (Exception e){
                logger.error("delete bucket force. clean acl failed. " +
                        "bucketName=" + bucket.getBucketName() + ", bucketId={}" + bucket.getBucketId(), e);
            }

            try{
                //uploadId
                uploadDao.setUploadsStatus(bucket.getBucketId(), null, UploadMeta.UPLOAD_ABORT);
            } catch (Exception e){
                logger.error("delete bucket force. clean uploads failed. " +
                        "bucketName=" + bucket.getBucketName() + ", bucketId={}" + bucket.getBucketId(), e);
            }
        }catch (S3ServerException e) {
            throw e;
        }catch (Exception e){
            throw new S3ServerException(S3Error.BUCKET_DELETE_FAILED,
                    "delete bucket force failed. bucket name = "+bucket.getBucketName(), e);
        }
    }

    @Override
    public LocationConstraint getBucketLocation(long ownerID, String bucketName) throws S3ServerException {
        try{
            Bucket bucket = getBucket(ownerID, bucketName);
            LocationConstraint locationConstraint = new LocationConstraint();
            locationConstraint.setLocation(bucket.getRegion());
            return locationConstraint;
        }catch (S3ServerException e){
            throw e;
        }catch (Exception e){
            throw new S3ServerException(S3Error.BUCKET_LOCATION_GET_FAILED,
                    "get bucket location failed. bucketName="+bucketName, e);
        }
    }

    private Boolean isValidBucketName(String bucketName){
        if (bucketName.length() < 3 || bucketName.length() > 63){
            return false;
        }
        return true;
    }
}
