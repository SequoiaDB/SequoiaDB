package com.sequoias3.service;

import com.sequoias3.core.*;
import com.sequoias3.dao.ConnectionDao;
import com.sequoias3.dao.DataLob;
import com.sequoias3.exception.S3ServerException;
import com.sequoias3.model.*;

import javax.servlet.ServletOutputStream;
import java.io.InputStream;
import java.util.List;
import java.util.Map;

public interface ObjectService {
    PutDeleteResult putObject(Bucket bucket, String objectName,
                              String contentMD5, Map<String, String> requestHeaders,
                              Map<String, String> xMeta, InputStream inputStream,
                              Long contentLength)
            throws S3ServerException;

    GetResult getObject(long ownerID, String bucketName, String objectName,
                        Long versionId, Boolean isNoVersion, Map matchers,
                        Range range)
            throws S3ServerException;

    CopyObjectResult copyObject(long ownerID, String destBucket, String destObject,
                                Map<String, String> requestHeaders, Map<String, String> xMeta,
                                ObjectUri sourceUri, boolean directiveCopy,
                                ServletOutputStream outputStream)
            throws S3ServerException;

    ObjectMeta getSourceObjectMeta(long ownerID, Map<String, String> requestHeaders,
                                   ObjectUri sourceUri)
            throws S3ServerException;

    void releaseGetResult(GetResult result);

    void readObjectData(DataLob data, ServletOutputStream outputStream, Range range)
            throws S3ServerException;

    PutDeleteResult deleteObject(long ownerID, String bucketName, String objectName)
            throws S3ServerException;

    PutDeleteResult deleteObject(long ownerID, String bucketName,
                                 String objectName, Long versionId)
            throws S3ServerException;

    ListObjectsResult listObjects(long ownerID, String bucketName, String prefix,
                                  String delimiter, String startAfter, Integer maxKeys,
                                  String continueToken, String encodingType, Boolean fetchOwner)
            throws S3ServerException;

    ListObjectsResultV1 listObjectsV1(long ownerID, String bucketName, String prefix,
                                      String delimiter, String startAfter, Integer maxKeys,
                                      String encodingType)
            throws S3ServerException;

    ListVersionsResult listVersions(long ownerID, String bucketName, String prefix,
                                    String delimiter, String keyMarker, String versionIdMarker,
                                    Integer maxKeys, String encodingType)
            throws S3ServerException;

    Boolean isEmptyBucket(ConnectionDao connection, Bucket bucket, Region region) throws S3ServerException;

    void deleteObjectByBucket(Bucket bucket) throws S3ServerException;

    InitiateMultipartUploadResult initMultipartUpload(long ownerID, String bucketName, String objectName,
                                                      Map<String, String> requestHeaders,
                                                      Map<String, String> xMeta) throws S3ServerException;

    String uploadPart(Bucket bucket, String objectName, long uploadId,
                      int partnumber, String contentMD5, InputStream inputStream,
                      long contentLength) throws S3ServerException;

    CompleteMultipartUploadResult completeUpload(long ownerID, String bucketName, String objectName,
                                                 String uploadId, List<Part> reqPartList,
                                                 ServletOutputStream outputStream)
            throws S3ServerException;

    void abortUpload(long ownerID, String bucketName, String objectName,
                     String uploadId) throws S3ServerException;

    ListPartsResult listParts(long ownerID, String bucketName, String objectName,
                              String uploadId, Integer partNumberMarker,
                              Integer maxParts, String encodingType)
            throws S3ServerException;

    ListMultipartUploadsResult listUploadLists(long ownerID, String bucketName, String prefix,
                         String delimiter, String keyMarker, Long uploadIdMarker,
                         Integer maxKeys, String encodingType) throws S3ServerException;
}
