package com.sequoias3.service;

import com.sequoias3.model.VersioningConfigurationNull;
import com.sequoias3.exception.S3ServerException;

public interface BucketVersioningService  {
    void putBucketVersioning(long ownerID, String bucketName, String status) throws S3ServerException;

    VersioningConfigurationNull getBucketVersioning(long ownerID, String bucketName) throws S3ServerException;
}
