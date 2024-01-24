package com.sequoias3.partupload;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.PartETag;
import com.amazonaws.services.s3.model.UploadPartRequest;
import com.amazonaws.services.s3.model.UploadPartResult;
import com.sequoiadb.fault.BrokenNetwork;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;
import com.sequoias3.commlibs3.CommLibS3;
import com.sequoias3.commlibs3.S3TestBase;
import com.sequoias3.commlibs3.TestTools;
import com.sequoias3.commlibs3.s3utils.ObjectUtils;
import com.sequoias3.commlibs3.s3utils.PartUploadUtils;

/**
 * @Description seqDB-18788 : 存在为1的分段号，完成分段上传过程中SequoiaS3和db端网络异常
 * @author wangkexin
 * @Date 2019.08.19
 * @version 1.00
 */
public class CompleteMultipartUploadWithBrokenNet18788 extends S3TestBase {
    private String bucketName = "bucket18788";
    private String keyName = "key18788";
    private long[] partSizes = { 5 * 1024 * 1024, 5 * 1024 * 1024,
            6 * 1024 * 1024, 14 * 1024 * 1024 };
    private AmazonS3 s3Client = null;
    private long fileSize = 30 * 1024 * 1024;
    private List< PartETag > partEtags = new ArrayList< PartETag >();
    private File localPath = null;
    private File file = null;
    private String filePath = null;
    private String uploadId;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        filePath = localPath + File.separator + "localFile_" + fileSize
                + ".txt";

        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );
        file = new File( filePath );

        s3Client = CommLibS3.buildS3Client();
        CommLibS3.clearBucket( s3Client, bucketName );
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
    }

    // 将断网用例暂时屏蔽，记录在excel表格中，待优化故障模块后解除
    @Test(enabled = false)
    public void test() throws Exception {
        uploadId = PartUploadUtils.initPartUpload( s3Client, bucketName,
                keyName );
        uploadParts();

        FaultMakeTask faultTask = BrokenNetwork
                .getFaultMakeTask( S3TestBase.hostName, 0, 20 );
        TaskMgr mgr = new TaskMgr( faultTask );
        CompleteMultipartUploadTask cTask = new CompleteMultipartUploadTask();
        mgr.addTask( cTask );
        mgr.execute();
        Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

        completeUploadAgainAndCheck();
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
            s3Client.shutdown();
        }
    }

    private class CompleteMultipartUploadTask extends OperateTask {
        @Override
        public void exec() {
            AmazonS3 s3Client = CommLibS3.buildS3Client();
            try {
                PartUploadUtils.completeMultipartUpload( s3Client, bucketName,
                        keyName, uploadId, partEtags );
            } catch ( AmazonS3Exception e ) {
                if ( !e.getErrorCode().equals( "NoSuchUpload" )
                        && !e.getErrorCode()
                                .equals( "CompleteMultipartUploadFailed" )
                        && e.getStatusCode() != 500 ) {
                    throw e;
                }
            } finally {
                if ( s3Client != null ) {
                    s3Client.shutdown();
                }
            }
        }
    }

    private void completeUploadAgainAndCheck() throws Exception {
        AmazonS3 s3Client = CommLibS3.buildS3Client();
        try {
            try {
                PartUploadUtils.completeMultipartUpload( s3Client, bucketName,
                        keyName, uploadId, partEtags );
            } catch ( AmazonS3Exception e ) {
                Assert.assertEquals( e.getErrorCode(), "NoSuchUpload" );
            }
            String expMd5 = TestTools.getMD5( filePath );
            String downloadMd5 = ObjectUtils.getMd5OfObject( s3Client,
                    localPath, bucketName, keyName );
            Assert.assertEquals( downloadMd5, expMd5 );
        } finally {
            s3Client.shutdown();
        }
    }

    private void uploadParts() {
        long filePosition = 0;
        for ( int i = 0; i < partSizes.length; i++ ) {
            UploadPartRequest partRequest = new UploadPartRequest()
                    .withFile( file ).withFileOffset( filePosition )
                    .withPartNumber( i + 1 ).withPartSize( partSizes[ i ] )
                    .withBucketName( bucketName ).withKey( keyName )
                    .withUploadId( uploadId );
            UploadPartResult uploadPartResult = s3Client
                    .uploadPart( partRequest );
            partEtags.add( uploadPartResult.getPartETag() );
            filePosition += partSizes[ i ];
        }
    }
}
