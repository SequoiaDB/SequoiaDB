package com.sequoias3.service;

import com.sequoias3.exception.S3ServerException;
import com.sequoias3.model.AccessControlPolicy;
import com.sequoias3.model.ObjectUri;

public interface ACLService {

    void putBucketAcl(long ownerID, String bucketName, AccessControlPolicy acl)
            throws S3ServerException;

    AccessControlPolicy getBucketAcl(long ownerID, String bucketName)
            throws S3ServerException;

    String putObjectAcl(long ownerID, ObjectUri objectUri,
                      AccessControlPolicy acl)
            throws S3ServerException;

    AccessControlPolicy getObjectAcl(long ownerID, ObjectUri objectUri)
            throws S3ServerException;
}
