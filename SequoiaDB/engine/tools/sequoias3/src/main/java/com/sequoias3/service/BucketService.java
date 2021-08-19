package com.sequoias3.service;

import com.sequoias3.core.Bucket;
import com.sequoias3.model.GetServiceResult;
import com.sequoias3.core.User;
import com.sequoias3.exception.S3ServerException;
import com.sequoias3.model.LocationConstraint;

public interface BucketService {
    void createBucket(long ownerID, String bucketName, String region) throws S3ServerException;

    void deleteBucket(long ownerID, String bucketName) throws S3ServerException;

    GetServiceResult getService(User owner) throws S3ServerException;

    Bucket getBucket(long ownerID, String bucketName) throws S3ServerException;

    void deleteBucketForce(Bucket bucket) throws S3ServerException;

    LocationConstraint getBucketLocation(long ownerID, String bucketName) throws S3ServerException;
}
