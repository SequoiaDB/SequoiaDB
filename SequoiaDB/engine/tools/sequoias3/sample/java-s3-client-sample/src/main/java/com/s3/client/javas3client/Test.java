package com.s3.client.javas3client;

import java.io.File;

public class Test {

    public static void main(String[] args){
        String endpoint = "http://localhost:8002";
        S3Client s3Client = new S3Client(endpoint);

        String regionName = "region0";
        String bucketName = "my-bucket";
        String objectName1 = "object1";
        String objectName2 = "object2";
        File file = new File("example");

        s3Client.createRegion(regionName);

        s3Client.listRegions();

        s3Client.getRegion(regionName);

        s3Client.putBucket(bucketName, regionName);

        s3Client.putVersioningStatus(bucketName, "Enabled");

        s3Client.putObjectWithContent(bucketName, objectName1, "test content");

        s3Client.putObjectWithFile(bucketName, objectName2, file);

        s3Client.copyObject(bucketName, objectName1, bucketName, objectName2);

        s3Client.deleteObject(bucketName, objectName1);

        s3Client.listObjects(bucketName);

        s3Client.listVersions(bucketName, null, null, null);

        s3Client.listBuckets();

        s3Client.deleteAllVersions(bucketName);

        s3Client.deleteBucket(bucketName);

        s3Client.deleteRegion(regionName);

        s3Client.shutdown();
    }
}
