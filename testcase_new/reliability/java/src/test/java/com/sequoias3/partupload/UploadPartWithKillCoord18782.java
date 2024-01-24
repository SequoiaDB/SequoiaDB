package com.sequoias3.partupload;

import java.io.File;
import java.util.List;
import java.util.concurrent.CopyOnWriteArrayList;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.amazonaws.AmazonServiceException;
import com.amazonaws.SdkClientException;
import com.amazonaws.services.s3.AmazonS3;
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
 * test content: 上传分段过程中db端节点故障，其中分段长度不同 testlink-case: seqDB-18782
 * 
 * @author wangkexin
 * @Date 2019.08.16
 * @version 1.00
 */
public class UploadPartWithKillCoord18782 extends S3TestBase {
    private GroupMgr groupMgr = null;
    private String bucketName = "bucket18782";
    private String keyName = "key18782";
    private long[] partSizes = { 8 * 1024 * 1024, 9 * 1024 * 1024,
            6 * 1024 * 1024, 5 * 1024 * 1024, 10 * 1024 * 1024 };
    private AmazonS3 s3Client = null;
    private GroupWrapper coordGroup = null;
    private long fileSize = 38 * 1024 * 1024;
    private List< PartETag > partEtags = new CopyOnWriteArrayList< PartETag >();
    private List< Long > putPartList = new CopyOnWriteArrayList< Long >();
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
    public void testUploadPart() throws Exception {
        uploadId = PartUploadUtils.initPartUpload( s3Client, bucketName,
                keyName );
        TaskMgr mgr = new TaskMgr();
        for ( NodeWrapper node : coordGroup.getNodes() ) {
            FaultMakeTask faultTask = KillNode.getFaultMakeTask( node, 1 );
            mgr.addTask( faultTask );
        }

        long filePosition = 0;
        for ( int i = 0; i < partSizes.length; i++ ) {
            UploadPartTask cTask = new UploadPartTask( i + 1, filePosition,
                    partSizes[ i ] );
            mgr.addTask( cTask );
            filePosition += partSizes[ i ];
        }
        mgr.execute();
        Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );
        Assert.assertEquals( groupMgr.checkBusinessWithLSN( 600 ), true,
                "checkBusinessWithLSN() occurs timeout" );
        uploadPartAgainAndCheck();
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

    private class UploadPartTask extends OperateTask {
        private int partNumber = 0;
        private long filePosition = 0;
        private long partSize = 0;

        public UploadPartTask( int partNumber, long filePosition,
                long partSize ) {
            this.partNumber = partNumber;
            this.filePosition = filePosition;
            this.partSize = partSize;
        }

        @Override
        public void exec() {
            AmazonS3 s3Client = CommLibS3.buildS3Client();
            try {
                UploadPartRequest partRequest = new UploadPartRequest()
                        .withFile( file ).withFileOffset( filePosition )
                        .withPartNumber( partNumber ).withPartSize( partSize )
                        .withBucketName( bucketName ).withKey( keyName )
                        .withUploadId( uploadId );
                UploadPartResult uploadPartResult = s3Client
                        .uploadPart( partRequest );
                putPartList.add( partSize );
                partEtags.add( uploadPartResult.getPartETag() );
            } catch ( AmazonServiceException e ) {
                // Upload part failed:500
                if ( e.getStatusCode() != 500 ) {
                    throw e;
                }
            } catch ( SdkClientException e ) {
                // SEQUOIADBMAINSTREAM-4784
                if ( !e.getMessage()
                        .contains( "Unable to execute HTTP request" ) ) {
                    throw e;
                }
            } finally {
                if ( s3Client != null ) {
                    s3Client.shutdown();
                }
            }
        }
    }

    private void uploadPartAgainAndCheck() throws Exception {
        long filePosition = 0;
        for ( int i = 0; i < partSizes.length; i++ ) {
            if ( !putPartList.contains( partSizes[ i ] ) ) {
                UploadPartRequest partRequest = new UploadPartRequest()
                        .withFile( file ).withFileOffset( filePosition )
                        .withPartNumber( i + 1 ).withPartSize( partSizes[ i ] )
                        .withBucketName( bucketName ).withKey( keyName )
                        .withUploadId( uploadId );
                UploadPartResult uploadPartResult = s3Client
                        .uploadPart( partRequest );
                partEtags.add( uploadPartResult.getPartETag() );
            }
            filePosition += partSizes[ i ];
        }
        PartUploadUtils.completeMultipartUpload( s3Client, bucketName, keyName,
                uploadId, partEtags );
        String expMd5 = TestTools.getMD5( filePath );
        String downloadMd5 = ObjectUtils.getMd5OfObject( s3Client, localPath,
                bucketName, keyName );
        Assert.assertEquals( downloadMd5, expMd5 );
    }
}