package com.sequoias3.dao;

import com.sequoias3.common.DelimiterStatus;
import com.sequoias3.core.Bucket;
import com.sequoias3.exception.S3ServerException;

import java.text.ParseException;
import java.util.List;

public interface BucketDao {
    void insertBucket(Bucket bucket) throws S3ServerException,ParseException;

    void deleteBucket(ConnectionDao connection, String bucketName) throws S3ServerException;

    Bucket getBucketByName(String bucketName) throws S3ServerException;

    Bucket getBucketById(ConnectionDao connection, long bucketId) throws S3ServerException;

    List<Bucket> getBucketListByOwnerID(long ownerId) throws S3ServerException;

    List<Bucket> getBucketListByRegion(ConnectionDao connection, String regionName) throws S3ServerException;

    List<Bucket> getBucketListByDelimiterStatus(DelimiterStatus status, Long overTime) throws S3ServerException;

    long getBucketNumber(long ownerID) throws S3ServerException;

    void updateBucketVersioning(String bucketName, String status)
            throws S3ServerException;

    void updateBucketDelimiter(ConnectionDao connection, String bucketName, Bucket bucket)
            throws S3ServerException;

    void updateBucketAcl(ConnectionDao connection, String bucketName, Long aclId, Boolean isPrivate)
            throws S3ServerException;

    void cleanBucketDelimiter(ConnectionDao connection, String bucketName, int delimiter)
            throws S3ServerException;

    Bucket queryBucketForUpdate(ConnectionDao connection, String bucketName)
            throws S3ServerException;
}
