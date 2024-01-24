package com.sequoias3.partupload;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import org.springframework.util.LinkedMultiValueMap;
import org.springframework.util.MultiValueMap;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.amazonaws.SdkClientException;
import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AbortMultipartUploadRequest;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.ListMultipartUploadsRequest;
import com.amazonaws.services.s3.model.MultipartUploadListing;
import com.amazonaws.services.s3.model.UploadPartRequest;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;
import com.sequoias3.commlibs3.CommLibS3;
import com.sequoias3.commlibs3.S3TestBase;
import com.sequoias3.commlibs3.TestTools;
import com.sequoias3.commlibs3.s3utils.PartUploadUtils;
import com.sequoias3.commlibs3.s3utils.S3NodeRestart;
import com.sequoias3.commlibs3.s3utils.bean.S3NodeWrapper;

/**
 * @Description seqDB-19138 :取消分段上传过程中SequoiaS3异常
 * @author wuyan
 * @version 1.00
 * @Date 2019.08.13
 */
public class AbortMultipartUploadAndS3ReStart19138 extends S3TestBase {
    private boolean runSuccess = false;
    private AmazonS3 s3Client = null;
    private String bucketName = "bucket19138";
    private String baseKeyName = "/test19138.txt";
    private int keyNum = 30;
    private int fileSize = 1024 * 1024 * 60;
    private String filePath = null;
    private File localPath = null;
    private File file = null;
    private MultiValueMap< String, String > successKeyAndUploadIds = new LinkedMultiValueMap< String, String >();
    private MultiValueMap< String, String > keyAndUploadIds = new LinkedMultiValueMap< String, String >();

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( SdbTestBase.workDir + File.separator
                + TestTools.getClassName() );
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        filePath = localPath + File.separator + "localFile_" + fileSize
                + ".txt";
        TestTools.LocalFile.createFile( filePath, fileSize );
        file = new File( filePath );
        s3Client = CommLibS3.buildS3Client();
        CommLibS3.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
    }

    @Test
    public void test() throws Exception {
        FaultMakeTask faultMakeTask = S3NodeRestart
                .getFaultMakeTask( new S3NodeWrapper(), 0, 15 );
        TaskMgr mgr = new TaskMgr( faultMakeTask );
        for ( int i = 0; i < keyNum; i++ ) {
            String keyName = "/dir/" + i + baseKeyName;
            String uploadId = PartUploadUtils.initPartUpload( s3Client,
                    bucketName, keyName );
            uploadParts( keyName, uploadId );
            keyAndUploadIds.add( keyName, uploadId );
        }
        mgr.addTask( new AbortMultipartUpload( keyAndUploadIds ) );
        mgr.execute();
        Assert.assertTrue( mgr.isAllSuccess(), mgr.getErrorMsg() );

        checkResult();
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                CommLibS3.clearBucket( s3Client, bucketName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            if ( s3Client != null )
                s3Client.shutdown();

        }
    }

    public class AbortMultipartUpload extends OperateTask {
        private MultiValueMap< String, String > keyAndUploadIds;
        private AmazonS3 s3Client1 = CommLibS3.buildS3Client();
        private String keyName;

        private AbortMultipartUpload(
                MultiValueMap< String, String > keyAndUploadIds ) {
            this.keyAndUploadIds = keyAndUploadIds;
        }

        @Override
        public void exec() throws Exception {
            try {
                for ( int i = 0; i < keyAndUploadIds.size(); i++ ) {
                    String keyName = keyAndUploadIds.keySet().toArray()[ i ]
                            .toString();
                    String uploadId = keyAndUploadIds.get( keyName ).get( 0 );
                    AbortMultipartUploadRequest request = new AbortMultipartUploadRequest(
                            bucketName, keyName, uploadId );
                    s3Client1.abortMultipartUpload( request );
                    successKeyAndUploadIds.add( keyName, uploadId );
                }
            } catch ( AmazonS3Exception e ) {
                // e:0 Get connection failed.e:404 NoSuchUpload.
                if ( e.getStatusCode() != 0 && e.getStatusCode() != 500
                        && e.getStatusCode() != 404 ) {
                    throw new Exception( keyName, e );
                }
            } catch ( SdkClientException e ) {
                if ( !e.getMessage()
                        .contains( "Unable to execute HTTP request" ) ) {
                    throw e;
                }
            } catch ( Exception e ) {
                if ( !e.getMessage().contains( "I/O error on POST request" ) ) {
                    throw e;
                }
            } finally {
                if ( s3Client1 != null ) {
                    s3Client1.shutdown();
                }
            }
        }
    }

    private void uploadParts( String keyName, String uploadId ) {
        int[] partSizes = { 1024 * 1024 * 6, 1024 * 1024 * 5, 1024 * 1024 * 6,
                1024 * 1024 * 8, 1024 * 1024 * 9, 1024 * 1024 * 6,
                1024 * 1024 * 8, 1024 * 1024 * 7, 1024 * 1024 * 5 };
        int partNumbers = 9;
        int filePosition = 0;
        new ArrayList< >();
        for ( int i = 0; i < partNumbers; i++ ) {
            // 分段号从1开始
            int partNumber = i + 1;
            int partSize = partSizes[ i ];
            long eachPartSize = Math.min( partSize, fileSize - filePosition );
            UploadPartRequest partRequest = new UploadPartRequest()
                    .withFile( file ).withFileOffset( filePosition )
                    .withPartNumber( partNumber ).withPartSize( eachPartSize )
                    .withBucketName( bucketName ).withKey( keyName )
                    .withUploadId( uploadId );
            s3Client.uploadPart( partRequest );
            filePosition += partSize;
        }
    }

    private void checkResult() {
        for ( int i = 0; i < successKeyAndUploadIds.size(); i++ ) {
            String keyName = successKeyAndUploadIds.keySet().toArray()[ i ]
                    .toString();
            keyAndUploadIds.remove( keyName );
        }

        for ( int i = 0; i < keyAndUploadIds.size(); i++ ) {
            try {
                String keyName = keyAndUploadIds.keySet().toArray()[ i ]
                        .toString();
                String uploadId = keyAndUploadIds.get( keyName ).get( 0 );
                AbortMultipartUploadRequest request = new AbortMultipartUploadRequest(
                        bucketName, keyName, uploadId );
                s3Client.abortMultipartUpload( request );
            } catch ( AmazonS3Exception e ) {
                // 如果db端已提交事务，返回s3时s3故障，则实际上取消分段上传操作成功，再次执行分段上传报错
                if ( e.getStatusCode() != 404
                        || !e.getErrorCode().contains( "NoSuchUpload" ) ) {
                    Assert.fail( "abortMultipart fail! e=" + e.getErrorCode()
                            + e.getStatusCode() );
                }
            }

        }

        // 查询分段上传列表显示不存在分段信息
        ListMultipartUploadsRequest request = new ListMultipartUploadsRequest(
                bucketName );
        MultipartUploadListing result = s3Client
                .listMultipartUploads( request );
        MultiValueMap< String, String > expUpload = new LinkedMultiValueMap< String, String >();
        List< String > expCommonPrefixes = new ArrayList< >();
        PartUploadUtils.checkListMultipartUploadsResults( result,
                expCommonPrefixes, expUpload );

    }
}
