package com.sequoias3.service.impl;

import com.sequoias3.common.VersioningStatusType;
import com.sequoias3.core.Bucket;
import com.sequoias3.model.*;
import com.sequoias3.dao.BucketDao;
import com.sequoias3.dao.UserDao;
import com.sequoias3.exception.S3Error;
import com.sequoias3.exception.S3ServerException;
import com.sequoias3.service.BucketService;
import com.sequoias3.service.BucketVersioningService;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;

@Service
public class BucketVersioningServiceImpl implements BucketVersioningService {
    private static final Logger logger = LoggerFactory.getLogger(BucketVersioningServiceImpl.class);

    @Autowired
    BucketDao bucketDao;

    @Autowired
    BucketService bucketService;

    @Autowired
    UserDao userDao;

    @Override
    public void putBucketVersioning(long ownerID, String bucketName, String status) throws S3ServerException {
        try {
            bucketService.getBucket(ownerID, bucketName);
            bucketDao.updateBucketVersioning(bucketName, status);
        }catch (S3ServerException e){
            throw e;
        }catch (Exception e){
            throw new S3ServerException(S3Error.BUCKET_VERSIONING_SET_FAILED,
                    "put bucket versioning failed. bucketname="+bucketName+",status="+status, e);
        }
    }

    @Override
    public VersioningConfigurationNull getBucketVersioning(long ownerID, String bucketName)
            throws S3ServerException{
        try{
            Bucket bucket = bucketService.getBucket(ownerID, bucketName);
            if (!bucket.getVersioningStatus().equals(VersioningStatusType.NONE.getName())){
                VersioningConfiguration versioningCfg = new VersioningConfiguration();
                versioningCfg.setStatus(bucket.getVersioningStatus());
                return versioningCfg;
            }else {
                return new VersioningConfigurationNull();
            }
        }catch (S3ServerException e){
            throw e;
        }catch (Exception e){
            throw new S3ServerException(S3Error.BUCKET_VERSIONING_GET_FAILED,
                    "get bucket versioning failed. bucketname="+bucketName, e);
        }
    }
}
