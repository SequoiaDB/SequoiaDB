package com.sequoias3.partupload;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.amazonaws.AmazonServiceException;
import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.PartETag;
import com.amazonaws.services.s3.model.UploadPartRequest;
import com.amazonaws.services.s3.model.UploadPartResult;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.fault.KillNode;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;
import com.sequoias3.commlibs3.CommLibS3;
import com.sequoias3.commlibs3.S3TestBase;
import com.sequoias3.commlibs3.TestTools;
import com.sequoias3.commlibs3.s3utils.ObjectUtils;
import com.sequoias3.commlibs3.s3utils.PartUploadUtils;

/**
 * test content: 上传分段长度不相同，完成分段上传过程中db端节点异常 testlink-case: seqDB-18786
 * 
 * @author wangkexin
 * @Date 2019.08.19
 * @version 1.00
 */
public class CompleteMultipartUploadWithKillCoord18786 extends S3TestBase {
    private GroupMgr groupMgr = null;
    private String bucketName = "bucket18786";
    private String keyName = "key18786";
    private long[] partSizes = { 8 * 1024 * 1024, 9 * 1024 * 1024,
            6 * 1024 * 1024, 5 * 1024 * 1024, 10 * 1024 * 1024 };
    private AmazonS3 s3Client = null;
    private GroupWrapper coordGroup = null;
    private long fileSize = 38 * 1024 * 1024;
    private List< PartETag > partEtags = new ArrayList< PartETag >();
    private File localPath = null;
    private File file = null;
    private String filePath = null;
    private String uploadId;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        groupMgr = GroupMgr.getInstance();
        coordGroup = groupMgr.getGroupByName( "SYSCoord" );

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

    @Test
    public void testCompleteMultipartUpload() throws Exception {
        uploadId = PartUploadUtils.initPartUpload( s3Client, bucketName,
                keyName );
        uploadPartsWithDiffSize();
        TaskMgr mgr = new TaskMgr();
        for ( NodeWrapper node : coordGroup.getNodes() ) {
            FaultMakeTask faultTask = KillNode.getFaultMakeTask( node, 0 );
            mgr.addTask( faultTask );
        }

        CompleteMultipartUploadTask cTask = new CompleteMultipartUploadTask();
        mgr.addTask( cTask );
        mgr.execute();
        Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );
        Assert.assertEquals( groupMgr.checkBusinessWithLSN( 600 ), true,
                "checkBusinessWithLSN() occurs timeout" );
        completeUploadAgainAndCheck();
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( runSuccess ) {
                CommLibS3.clearBucket( s3Client, bucketName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }

    private class CompleteMultipartUploadTask extends OperateTask {
        @Override
        public void exec() {
            AmazonS3 s3Client = CommLibS3.buildS3Client();
            try {
                PartUploadUtils.completeMultipartUpload( s3Client, bucketName,
                        keyName, uploadId, partEtags );
            } catch ( AmazonServiceException e ) {
                // 完成上传时合入lob失败，清理失败，可能报Status Code: 0
                // 完成分段上传后data节点被杀，可能导致coord返回失败，s3尝试再次完成分段上传报NoSuchUpload
                if ( e.getStatusCode() != 500 && e.getStatusCode() != 0
                        && e.getStatusCode() != 404 ) {
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
        try {
            PartUploadUtils.completeMultipartUpload( s3Client, bucketName,
                    keyName, uploadId, partEtags );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "NoSuchUpload" );
        }
        String expMd5 = TestTools.getMD5( filePath );
        String downloadMd5 = ObjectUtils.getMd5OfObject( s3Client, localPath,
                bucketName, keyName );
        Assert.assertEquals( downloadMd5, expMd5 );
    }

    private void uploadPartsWithDiffSize() {
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