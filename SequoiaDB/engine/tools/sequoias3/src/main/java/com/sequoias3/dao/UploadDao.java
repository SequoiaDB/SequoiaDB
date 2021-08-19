package com.sequoias3.dao;

import com.sequoias3.core.UploadMeta;
import com.sequoias3.exception.S3ServerException;

public interface UploadDao {
    void insertUploadMeta(ConnectionDao connectionDao, long bucketId, String objectName, Long uploadId, UploadMeta upload)
            throws S3ServerException;

    void updateUploadMeta(ConnectionDao connection, long bucketId,
                          String objectName, Long uploadId, UploadMeta upload)
            throws S3ServerException;

    UploadMeta queryUploadByUploadId(ConnectionDao connection, Long bucketId,
                                     String objectName, long uploadId, Boolean forUpdate)
            throws S3ServerException;

    void deleteUploadByUploadId(ConnectionDao connection, long bucketId,
                                String objectName, long uploadId)
            throws S3ServerException;

    QueryDbCursor queryInvalidUploads() throws S3ServerException;

    QueryDbCursor queryExceedUploads(long exceedTime) throws S3ServerException;

    QueryDbCursor queryUploadsByBucket(long bucketId, String prefix, String keyMarker,
                                       Long uploadMarker, Integer status) throws S3ServerException;

    void setUploadsStatus(long bucketId, Long uploadId, int status) throws S3ServerException;
}
