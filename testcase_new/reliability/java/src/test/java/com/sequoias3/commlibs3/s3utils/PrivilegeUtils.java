package com.sequoias3.commlibs3.s3utils;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Set;

import org.apache.http.client.methods.HttpPut;
import org.springframework.util.LinkedMultiValueMap;
import org.springframework.util.MultiValueMap;
import org.testng.Assert;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AccessControlList;
import com.amazonaws.services.s3.model.GetObjectAclRequest;
import com.amazonaws.services.s3.model.Grant;
import com.amazonaws.services.s3.model.SetObjectAclRequest;
import com.sequoias3.commlibs3.RestClient;
import com.sequoias3.commlibs3.S3TestBase;

public class PrivilegeUtils extends S3TestBase {

    /**
     * set bucket acl by request header (x-amz-grant*)
     * 
     * @author wangkexin
     * @param accessKeyId
     * @param bucketName
     * @param grants
     * @return
     */
    public static void setBucketAclByHeader( String accessKeyId,
            String bucketName, Grant... grants ) throws Exception {
        MultiValueMap< String, String > grantHeaderMap = new LinkedMultiValueMap< String, String >();

        HttpPut request = new HttpPut(
                S3TestBase.s3ClientUrl + "/" + bucketName + "/?acl" );
        request.setHeader( UserUtils.AUTHORIZATION,
                UserUtils.AUTH_VAL_PRE + accessKeyId + "/" );
        for ( Grant grant : grants ) {
            grantHeaderMap.add( grant.getPermission().getHeaderName(),
                    grant.getGrantee().getTypeIdentifier() + "="
                            + grant.getGrantee().getIdentifier() );
            ;
        }
        Set< String > headers = grantHeaderMap.keySet();
        for ( String header : headers ) {
            String grantStr = grantHeaderMap.get( header ).toString();
            request.setHeader( header,
                    grantStr.substring( 1, grantStr.length() - 1 ) );
        }
        RestClient.sendRequest( RestClient.createHttpClient(), request );
    }

    /**
     * set bucket acl by request body
     * 
     * @author wangkexin
     * @param s3Client
     * @param bucketName
     * @param grants
     * @return
     */
    public static void setBucketAclByBody( AmazonS3 s3Client, String bucketName,
            Grant... grants ) {
        AccessControlList bucketAcl = new AccessControlList();
        bucketAcl.grantAllPermissions( grants );
        bucketAcl.setOwner( s3Client.getS3AccountOwner() );
        s3Client.setBucketAcl( bucketName, bucketAcl );
    }

    /**
     * check bucket acl
     * 
     * @author wangkexin
     * @param s3Client
     * @param bucketName
     * @param expGrants
     * @return
     */
    public static void checkSetBucketAclResult( AmazonS3 s3Client,
            String bucketName, Grant... expGrants ) {
        boolean isEqual = false;
        List< Grant > expGrantsList = new ArrayList<>();
        AccessControlList result = s3Client.getBucketAcl( bucketName );
        if ( expGrants != null ) {
            expGrantsList = new ArrayList<>( Arrays.asList( expGrants ) );
        }
        List< Grant > actGrantsList = result.getGrantsAsList();
        if ( actGrantsList.size() == expGrantsList.size()
                && actGrantsList.containsAll( expGrantsList )
                && expGrantsList.containsAll( actGrantsList ) ) {
            isEqual = true;
        }
        if ( !isEqual ) {
            Assert.fail( "bucket acl is wrong! exp grants = "
                    + expGrantsList.toString() + ", act grants = "
                    + actGrantsList.toString() );
        }
    }

    /**
     * set object acl by request header (x-amz-grant*)
     * 
     * @author wangkexin
     * @param accessKeyId
     * @param bucketName
     * @param keyName
     * @param grants
     * @return
     */
    public static void setObjectAclByHeader( String accessKeyId,
            String bucketName, String keyName, Grant... grants )
            throws Exception {
        setObjectAclByHeader( accessKeyId, bucketName, keyName, null, grants );
    }

    /**
     * set object acl by request header (x-amz-grant*)
     * 
     * @author wangkexin
     * @param accessKeyId
     * @param bucketName
     * @param keyName
     * @param versionId
     * @param grants
     * @return
     */
    public static void setObjectAclByHeader( String accessKeyId,
            String bucketName, String keyName, String versionId,
            Grant... grants ) throws Exception {
        MultiValueMap< String, String > grantHeaderMap = new LinkedMultiValueMap< String, String >();
        HttpPut request = null;
        if ( versionId == null ) {
            request = new HttpPut( S3TestBase.s3ClientUrl + "/" + bucketName
                    + "/" + keyName + "?acl" );
        } else {
            request = new HttpPut( S3TestBase.s3ClientUrl + "/" + bucketName
                    + "/" + keyName + "?acl&versionId=" + versionId );
        }
        request.setHeader( UserUtils.AUTHORIZATION,
                UserUtils.AUTH_VAL_PRE + accessKeyId + "/" );
        for ( Grant grant : grants ) {
            grantHeaderMap.add( grant.getPermission().getHeaderName(),
                    grant.getGrantee().getTypeIdentifier() + "="
                            + grant.getGrantee().getIdentifier() );
            ;
        }
        Set< String > headers = grantHeaderMap.keySet();
        for ( String header : headers ) {
            String grantStr = grantHeaderMap.get( header ).toString();
            request.setHeader( header,
                    grantStr.substring( 1, grantStr.length() - 1 ) );
        }
        RestClient.sendRequest( RestClient.createHttpClient(), request );
    }

    /**
     * set object acl by request body
     * 
     * @author wangkexin
     * @param s3Client
     * @param bucketName
     * @param keyName
     * @param grants
     * @return
     */
    public static void setObjectAclByBody( AmazonS3 s3Client, String bucketName,
            String keyName, Grant... grants ) {
        setObjectAclByBody( s3Client, bucketName, keyName, null, grants );
    }

    /**
     * set object acl by request body
     * 
     * @author wangkexin
     * @param s3Client
     * @param bucketName
     * @param keyName
     * @param versionId
     * @param grants
     * @return
     */
    public static void setObjectAclByBody( AmazonS3 s3Client, String bucketName,
            String keyName, String versionId, Grant... grants ) {
        AccessControlList objectAcl = new AccessControlList();
        objectAcl.grantAllPermissions( grants );
        objectAcl.setOwner( s3Client.getS3AccountOwner() );
        s3Client.setObjectAcl( new SetObjectAclRequest( bucketName, keyName,
                versionId, objectAcl ) );
    }

    /**
     * check object acl
     * 
     * @author wangkexin
     * @param s3Client
     * @param bucketName
     * @param keyName
     * @param expGrants
     * @return
     */
    public static void checkSetObjectAclResult( AmazonS3 s3Client,
            String bucketName, String keyName, Grant... expGrants ) {
        checkSetObjectAclResult( s3Client, bucketName, keyName, null,
                expGrants );
    }

    /**
     * check object acl
     * 
     * @author wangkexin
     * @param s3Client
     * @param bucketName
     * @param keyName
     * @param versionId
     * @param expGrants
     * @return
     */
    public static void checkSetObjectAclResult( AmazonS3 s3Client,
            String bucketName, String keyName, String versionId,
            Grant... expGrants ) {
        boolean isEqual = false;
        List< Grant > expGrantsList = new ArrayList<>();
        AccessControlList result = s3Client.getObjectAcl(
                new GetObjectAclRequest( bucketName, keyName, versionId ) );
        if ( expGrants != null ) {
            expGrantsList = new ArrayList<>( Arrays.asList( expGrants ) );
        }
        List< Grant > actGrantsList = result.getGrantsAsList();
        if ( actGrantsList.size() == expGrantsList.size()
                && actGrantsList.containsAll( expGrantsList )
                && expGrantsList.containsAll( actGrantsList ) ) {
            isEqual = true;
        }
        if ( !isEqual ) {
            Assert.fail( "object acl is wrong! " + " key = " + keyName
                    + " , exp grants = " + expGrantsList.toString()
                    + ", act grants = " + actGrantsList.toString()
                    + " versionId = " + versionId );
        }
    }

}
