package com.sequoias3.service;

import com.sequoias3.exception.S3ServerException;
import com.sequoias3.model.DelimiterConfiguration;

public interface BucketDelimiterService {
    void putBucketDelimiter(long ownerID, String bucketName, String delimiter) throws S3ServerException;

    DelimiterConfiguration getBucketDelimiter(long ownerID, String bucketName, String encodingType) throws S3ServerException;
}
