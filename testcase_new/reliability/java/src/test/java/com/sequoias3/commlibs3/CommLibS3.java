package com.sequoias3.commlibs3;

import com.amazonaws.ClientConfiguration;
import com.amazonaws.auth.AWSCredentials;
import com.amazonaws.auth.AWSStaticCredentialsProvider;
import com.amazonaws.auth.BasicAWSCredentials;
import com.amazonaws.client.builder.AwsClientBuilder;
import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.AmazonS3ClientBuilder;
import com.amazonaws.services.s3.model.*;
import com.sequoias3.commlibs3.s3utils.UserUtils;
import org.springframework.http.HttpStatus;
import org.springframework.web.client.HttpClientErrorException;
import org.testng.Assert;

import java.util.Iterator;
import java.util.List;

public class CommLibS3 {
    private static String AWS_ACCESS_KEY = "ABCDEFGHIJKLMNOPQRST";
    private static String AWS_SECRET_KEY = "abcdefghijklmnopqrstuvwxyz0123456789ABCD";
    private static String clientRegion = "us-east-1";

    /**
     * build S3 client by admin
     * 
     * @param sdb
     * @return s3Client
     */
    public static AmazonS3 buildS3Client() {
        return buildS3Client( AWS_ACCESS_KEY, AWS_SECRET_KEY );
    }

    public static AmazonS3 buildS3Client( String ACCESS_KEY,
            String SECRET_KEY ) {
        AmazonS3 s3Client = null;
        AWSCredentials credentials = new BasicAWSCredentials( ACCESS_KEY,
                SECRET_KEY );
        AwsClientBuilder.EndpointConfiguration endpointConfiguration = new AwsClientBuilder.EndpointConfiguration(
                S3TestBase.s3ClientUrl, clientRegion );
        ClientConfiguration config = new ClientConfiguration();
        config.setUseExpectContinue( false );
        config.setSocketTimeout( 300000 );
        s3Client = AmazonS3ClientBuilder.standard()
                .withEndpointConfiguration( endpointConfiguration )
                .withClientConfiguration( config )
                .withChunkedEncodingDisabled( true )
                .withPathStyleAccessEnabled( true )
                .withCredentials(
                        new AWSStaticCredentialsProvider( credentials ) )
                .build();
        return s3Client;
    }

    /*
     * public static AmazonS3 buildS3ClientWithVersion(){ AmazonS3 s3Client =
     * null; AWSCredentials credentials = new
     * BasicAWSCredentials(AWS_ACCESS_KEY,AWS_SECRET_KEY);
     * AwsClientBuilder.EndpointConfiguration endpointConfiguration = new
     * AwsClientBuilder.EndpointConfiguration( S3TestBase.s3ClientUrl + "/s3",
     * clientRegion); BucketVersioningConfiguration configuration = new
     * BucketVersioningConfiguration().withStatus("Enabled"); s3Client =
     * AmazonS3ClientBuilder.standard()
     * .withEndpointConfiguration(endpointConfiguration) .withCredentials(new
     * AWSStaticCredentialsProvider(credentials)) .build(); return s3Client; }
     */

    /**
     * set the bucket versioning status
     * 
     * @param s3Client
     * @param bucketName
     * @param status:"null","Suspended","Enable"
     */
    public static void setBucketVersioning( AmazonS3 s3Client,
            String bucketName, String status ) {
        BucketVersioningConfiguration configuration = new BucketVersioningConfiguration()
                .withStatus( status );
        SetBucketVersioningConfigurationRequest setBucketVersioningConfigurationRequest = new SetBucketVersioningConfigurationRequest(
                bucketName, configuration );
        s3Client.setBucketVersioningConfiguration(
                setBucketVersioningConfigurationRequest );
    }

    /**
     * delete one bucket by bucketName
     * 
     * @param s3Client,bucketName
     */
    @SuppressWarnings("deprecation")
    public static void clearBucket( AmazonS3 s3Client, String bucketName ) {
        if ( s3Client.doesBucketExist( bucketName ) ) {
            String bucketVerStatus = s3Client
                    .getBucketVersioningConfiguration( bucketName ).getStatus();
            if ( bucketVerStatus == "Off" ) {
                deleteAllObjects( s3Client, bucketName );
            } else {
                deleteAllObjectVersions( s3Client, bucketName );
                ;
            }
            s3Client.deleteBucket( bucketName );
        }
    }

    /**
     * delete all object from the bucket
     * 
     * @param s3Client
     * @param bucketName
     */
    public static void deleteAllObjects( AmazonS3 s3Client,
            String bucketName ) {
        ListObjectsV2Request request = new ListObjectsV2Request()
                .withBucketName( bucketName ).withEncodingType( "url" );
        ListObjectsV2Result result;
        do {
            result = s3Client.listObjectsV2( request );
            Iterator< S3ObjectSummary > objIter = result.getObjectSummaries()
                    .iterator();
            while ( objIter.hasNext() ) {
                S3ObjectSummary vs = objIter.next();
                s3Client.deleteObject( bucketName, vs.getKey() );
            }
            String continuationToken = result.getNextContinuationToken();
            request.setContinuationToken( continuationToken );
        } while ( result.isTruncated() );
    }

    /**
     * delete all object versions(required for versioned buckets)
     * 
     * @param s3Client
     * @param bucketName
     */
    public static void deleteAllObjectVersions( AmazonS3 s3Client,
            String bucketName ) {
        VersionListing versionList = s3Client.listVersions(
                new ListVersionsRequest().withBucketName( bucketName ) );
        while ( true ) {
            Iterator< S3VersionSummary > versionIter = versionList
                    .getVersionSummaries().iterator();
            while ( versionIter.hasNext() ) {
                S3VersionSummary vs = versionIter.next();
                s3Client.deleteVersion( bucketName, vs.getKey(),
                        vs.getVersionId() );
            }

            if ( versionList.isTruncated() ) {
                versionList = s3Client.listNextBatchOfVersions( versionList );
            } else {
                break;
            }
        }
    }

    /**
     * delete all buckets
     * 
     * @param s3Client
     */
    public static void clearBuckets( AmazonS3 s3Client ) {
        List< Bucket > buckets = s3Client.listBuckets();
        if ( buckets.size() != 0 ) {
            for ( int i = 0; i < buckets.size(); i++ ) {
                String bucketName = buckets.get( i ).getName();
                deleteAllObjects( s3Client, bucketName );
                deleteAllObjectVersions( s3Client, bucketName );
                s3Client.deleteBucket( bucketName );
            }
        }
    }

    public static void clearUser( String userName ) {
        try {
            UserUtils.deleteUser( userName );
        } catch ( HttpClientErrorException e ) {
            if ( e.getStatusCode() != ( HttpStatus.NOT_FOUND ) ) {
                Assert.fail( e.getMessage() );
            }
        }
    }
}
