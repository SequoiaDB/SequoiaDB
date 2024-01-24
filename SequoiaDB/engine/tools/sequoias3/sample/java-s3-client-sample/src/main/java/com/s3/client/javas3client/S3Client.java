package com.s3.client.javas3client;

import com.amazonaws.AmazonServiceException;
import com.amazonaws.auth.AWSCredentials;
import com.amazonaws.auth.AWSStaticCredentialsProvider;
import com.amazonaws.auth.BasicAWSCredentials;
import com.amazonaws.client.builder.AwsClientBuilder;
import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.AmazonS3ClientBuilder;
import com.amazonaws.services.s3.model.*;
import com.sequoias3.SequoiaS3;
import com.sequoias3.SequoiaS3ClientBuilder;
import com.sequoias3.exception.SequoiaS3ClientException;
import com.sequoias3.exception.SequoiaS3ServiceException;
import com.sequoias3.model.GetRegionResult;
import com.sequoias3.model.ListRegionsResult;
import com.sequoias3.model.Region;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

public class S3Client {
    private static Logger logger = LoggerFactory.getLogger(S3Client.class);
    private AmazonS3 awsS3;
    private SequoiaS3 sequoiaS3;

    public S3Client(String endpointarg) {
        String accessKey = "ABCDEFGHIJKLMNOPQRST";
        String secretKey = "abcdefghijklmnopqrstuvwxyz0123456789ABCD";
        AWSCredentials credentials = new BasicAWSCredentials(accessKey, secretKey);

        String endPoint = "http://localhost:8002";
        if (endpointarg != null) {
            endPoint = endpointarg;
        }
        AwsClientBuilder.EndpointConfiguration endpointConfiguration =
                new AwsClientBuilder.EndpointConfiguration(endPoint, null);

        awsS3 = AmazonS3ClientBuilder.standard()
                .withEndpointConfiguration(endpointConfiguration)
                .withCredentials(new AWSStaticCredentialsProvider(credentials))
                .build();

        sequoiaS3 = SequoiaS3ClientBuilder.standard()
                .withAccessKeys(accessKey, secretKey)
                .withEndpoint(endPoint)
                .build();
    }

    public void shutdown() {
        awsS3.shutdown();
        sequoiaS3.shutdown();
    }

    public void putBucket(String bucketName, String region) {
        logger.debug("putBucket enter");
        try {
            CreateBucketRequest request = new CreateBucketRequest(bucketName, region);
            awsS3.createBucket(request);
        } catch (AmazonServiceException e) {
            logger.error("status code:" + e.getStatusCode());
            logger.error("error code:" + e.getErrorCode());
            logger.error("error message:" + e.getErrorMessage());
        } catch (Exception e) {
            logger.error("error message:" + e.getMessage());
//            throw e;
        } finally {
            logger.debug("putBucket exit");
        }
    }

    public void deleteBucket(String bucket) {
        logger.debug("deleteBucket enter");
        try {
            DeleteBucketRequest request = new DeleteBucketRequest(bucket);
            awsS3.deleteBucket(request);
        } catch (AmazonServiceException e) {
            logger.error("status code:" + e.getStatusCode());
            logger.error("error code:" + e.getErrorCode());
            logger.error("error message:" + e.getErrorMessage());
        } catch (Exception e) {
            logger.error("error message:" + e.getMessage());
//            throw e;
        } finally {
            logger.debug("deleteBucket exit");
        }
    }

    public void listBuckets() {
        logger.debug("listBuckets enter");
        try {
            List<Bucket> buckets = awsS3.listBuckets();
            System.out.println("bucket count:" + buckets.size());
            for (int i = 0; i < buckets.size(); i++) {
                Bucket bucket = buckets.get(i);
                System.out.println("name:" + bucket.getName() +
                        ", createTime:" + bucket.getCreationDate());
            }
        } catch (AmazonServiceException e) {
            logger.error("status code:" + e.getStatusCode());
            logger.error("error code:" + e.getErrorCode());
            logger.error("error message:" + e.getErrorMessage());
        } catch (Exception e) {
            logger.error("error message:" + e.getMessage());
//            throw e;
        } finally {
            logger.debug("listBuckets exit");
        }
    }

    public void listObjects(String bucketName) {
        logger.debug("listObjects enter");
        try {
            ListObjectsV2Request request = new ListObjectsV2Request();
            request.setBucketName(bucketName);
            while (true) {
                ListObjectsV2Result result = awsS3.listObjectsV2(request);

                List<String> commonPrefix = result.getCommonPrefixes();
                System.out.println("commonPrefix count: " + commonPrefix.size());
                for (int i = 0; i < commonPrefix.size(); i++) {
                    System.out.println("commonPrefix:" + commonPrefix.get(i));
                }
                List<S3ObjectSummary> objectList = result.getObjectSummaries();
                System.out.println("key count: " + objectList.size());
                for (int i = 0; i < objectList.size(); i++) {
                    System.out.println("name: " + objectList.get(i).getKey());
                }
                if (result.isTruncated()) {
                    request.withContinuationToken(result.getNextContinuationToken());
                    System.out.println("NextContinuationToken:" + result.getNextContinuationToken());
                } else {
                    break;
                }
            }
        } catch (AmazonServiceException e) {
            logger.error("status code:" + e.getStatusCode());
            logger.error("error code:" + e.getErrorCode());
            logger.error("error message:" + e.getErrorMessage());
        } catch (Exception e) {
            logger.error("error message:" + e.getMessage());
//            throw e;
        } finally {
            logger.debug("listObjects exit");
        }
    }

    public void listVersions(String bucketName, String prefix, String delimiter, String keyMarker) {
        logger.debug("listVersions enter. bucket:" + bucketName + ", prefix:" + prefix + ", delimiter:" + delimiter);
        try {
            ListVersionsRequest request = new ListVersionsRequest().withBucketName(bucketName);
            request.setBucketName(bucketName);
            request.setDelimiter(delimiter);
            request.setKeyMarker(keyMarker);
            request.setPrefix(prefix);
            VersionListing result = awsS3.listVersions(request);
            while (true) {
                List<String> commonPrefixList = result.getCommonPrefixes();
                for (int i = 0; i < commonPrefixList.size(); i++) {
                    System.out.println("getCommonPrefixes:" + commonPrefixList.get(i));
                }
                List<S3VersionSummary> versionSummaries = result.getVersionSummaries();
                for (int i = 0; i < versionSummaries.size(); i++) {
                    System.out.println("key:" + versionSummaries.get(i).getKey());
                    System.out.println("versionId:" + versionSummaries.get(i).getVersionId());
                    System.out.println("lastmodified:" + versionSummaries.get(i).getLastModified());
                }
                if (result.isTruncated()) {
                    result = awsS3.listNextBatchOfVersions(result);
                } else {
                    break;
                }
            }
        } catch (AmazonServiceException e) {
            logger.error("status code:" + e.getStatusCode());
            logger.error("status code:" + e.getErrorCode());
            logger.error("status code:" + e.getErrorMessage());
        } catch (Exception e) {
            logger.error("status code:" + e.getMessage());
        } finally {
            logger.debug("listVersions exit");
        }
    }

    public void multiPartUploadObject(String bucketName, String objectName, File file) {
        logger.debug("Multi part upload  enter");
        long maxsize = 5 * 1024 * 1024;

        try {
            List<PartETag> partETags = new ArrayList<PartETag>();
            InitiateMultipartUploadRequest initRequest = new InitiateMultipartUploadRequest(bucketName, objectName);
            InitiateMultipartUploadResult result = awsS3.initiateMultipartUpload(initRequest);
            String uploadId = result.getUploadId();

            long filepositon = 0;
            long filelength = file.length();
            for (int i = 1; filepositon < filelength; i++) {
                long partsize = Math.min(maxsize, (filelength - filepositon));
                UploadPartRequest partRequest = new UploadPartRequest()
                        .withFile(file)
                        .withFileOffset(filepositon)
                        .withPartNumber(i)
                        .withPartSize(partsize)
                        .withBucketName(bucketName)
                        .withKey(objectName)
                        .withUploadId(uploadId);
                UploadPartResult uploadPartResult = awsS3.uploadPart(partRequest);
                partETags.add(uploadPartResult.getPartETag());
                filepositon += partsize;
            }

            ListPartsRequest listPartsRequest =
                    new ListPartsRequest(bucketName, objectName, uploadId);
            PartListing listResult = awsS3.listParts(listPartsRequest);
            int size = listResult.getParts().size();
            for (int i = 0; i < size; i++) {
                System.out.println("i:" + i +
                        ", part number:" + listResult.getParts().get(i).getPartNumber() +
                        ", part size:" + listResult.getParts().get(i).getSize() +
                        ", part etag:" + listResult.getParts().get(i).getETag());
            }

            CompleteMultipartUploadRequest completeRequest = new CompleteMultipartUploadRequest()
                    .withBucketName(bucketName)
                    .withKey(objectName)
                    .withUploadId(uploadId)
                    .withPartETags(partETags);
            CompleteMultipartUploadResult result1 = awsS3.completeMultipartUpload(completeRequest);
            System.out.println("complete eTag:" + result1.getETag());
        } catch (AmazonServiceException e) {
            logger.error("status code:" + e.getStatusCode());
            logger.error("error code:" + e.getErrorCode());
            logger.error("error message:" + e.getErrorMessage());
        } catch (Exception e) {
            logger.error("error message:" + e.getMessage());
//            throw e;
        } finally {
            logger.debug("Multi part upload exit");
        }
    }

    public void putObjectWithContent(String bucketName, String objectName, String content) {
        logger.debug("putObject enter");
        try {
            awsS3.putObject(bucketName, objectName, content);
        } catch (AmazonServiceException e) {
            logger.error("status code:" + e.getStatusCode());
            logger.error("error code:" + e.getErrorCode());
            logger.error("error message:" + e.getErrorMessage());
        } catch (Exception e) {
            logger.error("error:" + e.getMessage());
            logger.debug("error:" + e.getMessage(), e);
        } finally {
            logger.debug("putObject exit");
        }
    }

    public void putObjectWithFile(String bucketName, String objectName, File file) {
        logger.debug("putObjectWithFile enter");
        try {
            PutObjectRequest request = new PutObjectRequest(bucketName, objectName, file);
            awsS3.putObject(request);
        } catch (AmazonServiceException e) {
            logger.error("status code:" + e.getStatusCode());
            logger.error("error code:" + e.getErrorCode());
            logger.error("error message:" + e.getErrorMessage());
        } catch (Exception e) {
            logger.error("error message:" + e.getMessage());
            logger.debug("error:" + e.getMessage(), e);
        } finally {
            logger.debug("putObjectWithFile exit");
        }
    }

    public void copyObject(String sourceBucket, String sourceObject, String destBucket, String destObject) {
        logger.info("copyObject enter. destBucket:" + destBucket + ", destObject:" + destObject);
        try {
            CopyObjectRequest request = new CopyObjectRequest(sourceBucket, sourceObject, destBucket, destObject);
            CopyObjectResult result = awsS3.copyObject(request);
            System.out.println("ETag:" + result.getETag());
            System.out.println("VersionId:" + result.getVersionId());
            System.out.println("lastModifiedDate:" + result.getLastModifiedDate());
        } catch (AmazonServiceException e) {
            logger.error("status code:" + e.getStatusCode());
            logger.error("error code:" + e.getErrorCode());
            logger.error("error message:" + e.getErrorMessage());
        } catch (Exception e) {
            logger.error("status code:" + e.getMessage());
        } finally {
            logger.info("copyObject exit");
        }
    }

    public void deleteObject(String bucketName, String objectName) {
        logger.debug("deleteObject enter");
        try {
            awsS3.deleteObject(bucketName, objectName);
        } catch (AmazonServiceException e) {
            logger.error("status code:" + e.getStatusCode());
            logger.error("error code:" + e.getErrorCode());
            logger.error("error message:" + e.getErrorMessage());
        } catch (Exception e) {
            logger.error("error message:" + e.getMessage());
        } finally {
            logger.debug("deleteObject exit");
        }
    }

    public void deleteVersion(String bucketName, String objectName, String versionId) {
        logger.debug("deleteVersion enter: bucket:" + bucketName + ", key:" + objectName + "versionId:" + versionId);
        try {
            awsS3.deleteVersion(bucketName, objectName, versionId);
        } catch (AmazonServiceException e) {
            logger.error("status code" + e.getStatusCode());
            logger.error("status code:" + e.getErrorCode());
            logger.error("status code:" + e.getErrorMessage());
        } catch (Exception e) {
            logger.error("status code:" + e.getMessage());
        } finally {
            logger.debug("deleteVersion exit");
        }
    }

    public void deleteAllVersions(String bucketName) {
        logger.debug("delete all versions enter");
        try {
            ListVersionsRequest request = new ListVersionsRequest();
            request.setBucketName(bucketName);
            VersionListing result = awsS3.listVersions(request);
            while (true) {
                List<S3VersionSummary> versionSummaries = result.getVersionSummaries();
                for (int i = 0; i < versionSummaries.size(); i++) {
                    System.out.println("delete key:" + versionSummaries.get(i).getKey() + ", versionId:" + versionSummaries.get(i).getVersionId());
                    awsS3.deleteVersion(bucketName, versionSummaries.get(i).getKey(), versionSummaries.get(i).getVersionId());
                }
                if (result.isTruncated()) {
                    result = awsS3.listNextBatchOfVersions(result);
                } else {
                    break;
                }
            }
        } catch (AmazonServiceException e) {
            System.out.println("status code:" + e.getStatusCode());
            System.out.println("status code:" + e.getErrorCode());
            System.out.println("status code:" + e.getErrorMessage());
        } finally {
            logger.debug("delete all versions exit");
        }
    }

    public void getObject(String bucketName, String objectName, String versionId, String path) throws IOException {
        logger.debug("getObject enter");
        S3ObjectInputStream s3is = null;
        FileOutputStream fos = null;
        try {
            GetObjectRequest request = new GetObjectRequest(bucketName, objectName, versionId);
            S3Object result = awsS3.getObject(request);

            s3is = result.getObjectContent();
            File file = new File(path + objectName);
            System.out.println("file path:" + file.getAbsolutePath());
            fos = new FileOutputStream(file);
            byte[] read_buf = new byte[1024 * 1024];
            int read_len;
            while ((read_len = s3is.read(read_buf)) > 0) {
                fos.write(read_buf, 0, read_len);
            }
            s3is.close();
            fos.close();
        } catch (AmazonServiceException e) {
            logger.error("status code:" + e.getStatusCode());
            logger.error("error code:" + e.getErrorCode());
            logger.error("error message:" + e.getErrorMessage());
        } catch (Exception e) {
            logger.error("error message:" + e.getMessage());
//            throw e;
        } finally {
            logger.debug("getObject exit");
            if (fos != null) {
                try {
                    fos.close();
                } catch (Exception e) {
                    logger.error("fos.close() failed. e:" + e.getMessage());
                }
            }
            if (s3is != null) {
                try {
                    s3is.close();
                } catch (Exception e) {
                    logger.error("s3is.close() failed. e:" + e.getMessage());
                }
            }
        }
    }

    public void createRegion(String regionName) {
        logger.debug("create region enter, regionName:" + regionName);
        try {
            sequoiaS3.createRegion(regionName);
        } catch (SequoiaS3ServiceException e) {
            logger.error("status code:" + e.getStatusCode());
            logger.error("error code:" + e.getErrorCode());
            logger.error("error message:" + e.getErrorMessage());
        } catch (SequoiaS3ClientException e) {
            logger.error("error message:" + e.getMessage());
//            throw e;
        } finally {
            logger.debug("create region exit.");
        }
    }

    public void getRegion(String regionName) {
        logger.debug("get region enter, regionName:" + regionName);
        try {
            GetRegionResult regionResult = sequoiaS3.getRegion(regionName);
            Region region = regionResult.getRegion();
            List<String> buckets = regionResult.getBuckets();
            System.out.println("region:" + region.toString());

            for (int i = 0; i < buckets.size(); i++) {
                String bucket = buckets.get(i);
                System.out.println("Name:" + bucket);
            }
        } catch (SequoiaS3ServiceException e) {
            logger.error("status code:" + e.getStatusCode());
            logger.error("error code:" + e.getErrorCode());
            logger.error("error message:" + e.getErrorMessage());
        } catch (SequoiaS3ClientException e) {
            logger.error("status code:" + e.getMessage());
        } catch (Exception e) {
            logger.error("status code:" + e.getMessage());
//            throw e;
        } finally {
            logger.debug("get region exit.");
        }
    }

    public void headRegion(String regionName) {
        logger.debug("head region enter, regionName:" + regionName);
        try {
            Boolean isRegionExist = sequoiaS3.headRegion(regionName);
            System.out.println("Region(" + regionName + ") exist: " + isRegionExist);
        } catch (SequoiaS3ServiceException e) {
            logger.error("status code:" + e.getStatusCode());
            logger.error("error code:" + e.getErrorCode());
            logger.error("error message:" + e.getErrorMessage());
        } catch (Exception e) {
            logger.error("status code:" + e.getMessage());
//            throw e;
        } finally {
            logger.debug("head region exit.");
        }
    }

    public void deleteRegion(String regionName) {
        logger.debug("delete region enter, regionName:" + regionName);
        try {
            sequoiaS3.deleteRegion(regionName);
        } catch (SequoiaS3ServiceException e) {
            logger.error("status code:" + e.getStatusCode());
            logger.error("error code:" + e.getErrorCode());
            logger.error("error message:" + e.getErrorMessage());
        } catch (Exception e) {
            logger.error("status code:" + e.getMessage());
//            throw e;
        } finally {
            logger.debug("delete region exit.");
        }
    }

    public void listRegions() {
        logger.debug("list regions enter.");
        try {
            ListRegionsResult listRegionsResult = sequoiaS3.listRegions();
            logger.info("regionResult" + listRegionsResult.getRegions());
        } catch (SequoiaS3ServiceException e) {
            logger.error("status code:" + e.getStatusCode());
            logger.error("error code:" + e.getErrorCode());
            logger.error("error message:" + e.getErrorMessage());
        } catch (Exception e) {
            logger.error("status code:" + e.getMessage());
//            throw e;
        } finally {
            logger.debug("list regions exit.");
        }
    }

    public void putVersioningStatus(String bucket, String status) {
        logger.debug("putVersioningStatus enter");
        try {
            BucketVersioningConfiguration cfg = new BucketVersioningConfiguration(status);
            SetBucketVersioningConfigurationRequest request = new SetBucketVersioningConfigurationRequest(bucket, cfg);
            awsS3.setBucketVersioningConfiguration(request);
        } catch (AmazonServiceException e) {
            logger.error("status code" + e.getStatusCode());
            logger.error("status code:" + e.getErrorCode());
            logger.error("status code:" + e.getErrorMessage());
        } finally {
            logger.debug("putVersioningStatus exit");
        }
    }
}
