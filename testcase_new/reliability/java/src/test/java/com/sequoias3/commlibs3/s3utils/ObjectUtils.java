package com.sequoias3.commlibs3.s3utils;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.*;
import com.sequoias3.commlibs3.S3TestBase;
import com.sequoias3.commlibs3.TestTools;
import org.springframework.util.LinkedMultiValueMap;
import org.springframework.util.MultiValueMap;
import org.testng.Assert;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.*;

public class ObjectUtils extends S3TestBase {

    /**
     * download the object ,than get the object content md5
     * 
     * @param s3Client
     * @param localPath
     * @param bucketName
     * @param keyName
     * @return md5
     */
    public static String getMd5OfObject( AmazonS3 s3Client, File localPath,
            String bucketName, String key ) throws Exception {
        return getMd5OfObject( s3Client, localPath, bucketName, key, null );
    }

    /**
     * download the object with versionId,than get the object content md5
     * 
     * @param s3Client
     * @param localPath
     * @param bucketName
     * @param keyName
     * @param versionId
     * @return md5
     */
    public static String getMd5OfObject( AmazonS3 s3Client, File localPath,
            String bucketName, String key, String versionId ) throws Exception {
        GetObjectRequest request = new GetObjectRequest( bucketName, key,
                versionId );
        S3Object object = s3Client.getObject( request );
        S3ObjectInputStream s3is = object.getObjectContent();
        String downloadPath = TestTools.LocalFile.initDownloadPath( localPath,
                TestTools.getMethodName(), Thread.currentThread().getId() );
        inputStream2File( s3is, downloadPath );
        s3is.close();
        String getMd5 = TestTools.getMD5( downloadPath );
        return getMd5;
    }

    /**
     * input stream to file
     * 
     * @param inputStream
     * @param downloadPath
     */
    public static String inputStream2File( InputStream inputStream,
            String downloadPath ) throws IOException {
        FileOutputStream fos = null;
        try {
            fos = new FileOutputStream( new File( downloadPath ), true );
            byte[] read_buf = new byte[ 1024 ];
            int read_len = 0;
            while ( ( read_len = inputStream.read( read_buf ) ) > -1 ) {
                fos.write( read_buf, 0, read_len );
            }
        } finally {
            if ( fos != null ) {
                fos.close();
            }
        }
        return downloadPath;
    }

    /**
     * delete the object of all versions(required for versioned buckets)
     * 
     * @param s3Client
     * @param bucketName
     * @param keyName
     */
    public static void deleteObjectAllVersions( AmazonS3 s3Client,
            String bucketName, String keyName ) {
        VersionListing versionList = s3Client.listVersions(
                new ListVersionsRequest().withBucketName( bucketName ) );
        while ( true ) {
            Iterator< S3VersionSummary > versionIter = versionList
                    .getVersionSummaries().iterator();

            while ( versionIter.hasNext() ) {
                S3VersionSummary vs = versionIter.next();
                String getKey = vs.getKey();

                if ( getKey.equals( keyName ) ) {
                    s3Client.deleteVersion( bucketName, vs.getKey(),
                            vs.getVersionId() );
                }

            }

            if ( versionList.isTruncated() ) {
                versionList = s3Client.listNextBatchOfVersions( versionList );
            } else {
                break;
            }
        }
    }

    public static void clearOneObject( AmazonS3 s3Client, String bucketName,
            String key ) {
        if ( s3Client.doesObjectExist( bucketName, key ) ) {
            s3Client.deleteObject( bucketName, key );
        }
    }

    public static void checkListVSResults( VersionListing vsList,
            List< String > expCommonPrefixes,
            MultiValueMap< String, String > expMap ) {
        Collections.sort( expCommonPrefixes );
        List< String > actCommonPrefixes = vsList.getCommonPrefixes();
        Assert.assertEquals( actCommonPrefixes, expCommonPrefixes,
                "actCommonPrefixes = " + actCommonPrefixes.toString()
                        + ",expCommonPrefixes = "
                        + expCommonPrefixes.toString() );
        List< S3VersionSummary > vsSummaryList = vsList.getVersionSummaries();
        MultiValueMap< String, String > actMap = new LinkedMultiValueMap< String, String >();
        for ( S3VersionSummary versionSummary : vsSummaryList ) {
            actMap.add( versionSummary.getKey(),
                    versionSummary.getVersionId() );
        }
        Assert.assertEquals( actMap.size(), expMap.size(), "actMap = "
                + actMap.toString() + ",expMap = " + expMap.toString() );
        for ( Map.Entry< String, List< String > > entry : expMap.entrySet() ) {
            Assert.assertEquals( actMap.get( entry.getKey() ),
                    expMap.get( entry.getKey() ),
                    "actMap = " + actMap.toString() + ",expMap = "
                            + expMap.toString() );
        }
    }

    public static List< String > getCommPrefixes( String[] objectNames,
            String prefix, String delimiter ) {
        List< String > commPrefixes = new ArrayList< String >();
        for ( String objectName : objectNames ) {
            if ( objectName.startsWith( prefix ) ) {
                int end = objectName.indexOf( delimiter, prefix.length() );
                if ( end != -1 ) {
                    String commPrefix = objectName.substring( 0,
                            end + delimiter.length() );
                    if ( !commPrefixes.contains( commPrefix ) ) {
                        commPrefixes.add( commPrefix );
                    }
                }
            }
        }
        return commPrefixes;
    }

    public static List< String > getKeys( String[] objectNames, String prefix,
            String delimiter ) {
        List< String > keys = new ArrayList< String >();
        for ( String objectName : objectNames ) {
            if ( objectName.startsWith( prefix ) ) {
                int index = objectName.indexOf( delimiter, prefix.length() );
                if ( index == -1 ) {
                    keys.add( objectName );
                }
            }
        }
        return keys;
    }

    public static void checkListObjectsV2Commprefixes(
            List< String > resultList, List< String > expresultList ) {
        Collections.sort( expresultList );
        Assert.assertEquals( resultList.size(), expresultList.size(),
                "The expected results do not match the actual number of returns" );
        for ( int i = 0; i < resultList.size(); i++ ) {
            Assert.assertEquals( resultList.get( i ), expresultList.get( i ),
                    "commonPrefixes is wrong" );
        }
    }

    public static void checkListObjectsV2KeyName(
            List< S3ObjectSummary > objectSummaries,
            List< String > expresultList ) {
        Collections.sort( expresultList );
        Assert.assertEquals( objectSummaries.size(), expresultList.size(),
                "The number of returned results is wrong" );
        for ( int i = 0; i < objectSummaries.size(); i++ ) {
            Assert.assertEquals( objectSummaries.get( i ).getKey(),
                    expresultList.get( i ), "keyName is wrong" );
        }
    }

    public static String getRandomString( int length ) {
        String str = "adcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
        Random random = new Random();
        StringBuffer sb = new StringBuffer();
        for ( int i = 0; i < length; i++ ) {
            int number = random.nextInt( str.length() );
            sb.append( str.charAt( number ) );
        }
        return sb.toString();
    }
}
