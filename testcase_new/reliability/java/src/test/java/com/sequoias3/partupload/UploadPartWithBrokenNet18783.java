package com.sequoias3.partupload;

import java.io.File;
import java.io.IOException;
import java.util.List;
import java.util.concurrent.CopyOnWriteArrayList;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.amazonaws.SdkClientException;
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
 * @Description seqDB-18783 :上传分段过程中sequoiaS3和db端网络故障，其中分段长度不同
 * @author wangkexin
 * @Date 2019.08.16
 * @version 1.00
 */
public class UploadPartWithBrokenNet18783 extends S3TestBase {
    private String bucketName = "bucket18783";
    private String keyName = "key18783";
    private long[] partSizes = { 18 * 1024 * 1024, 19 * 1024 * 1024,
            16 * 1024 * 1024, 15 * 1024 * 1024, 20 * 1024 * 1024,
            12 * 1024 * 1024 };
    private AmazonS3 s3Client = null;
    private long fileSize = 100 * 1024 * 1024;
    private List< PartETag > partEtags = new CopyOnWriteArrayList< PartETag >();
    private List< Long > putPartList = new CopyOnWriteArrayList< Long >();
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

    @Test(enabled = false)
    public void test() throws Exception {
        FaultMakeTask faultTask = BrokenNetwork
                .getFaultMakeTask( S3TestBase.s3HostName, 0, 30 );
        TaskMgr mgr = new TaskMgr( faultTask );
        uploadId = PartUploadUtils.initPartUpload( s3Client, bucketName,
                keyName );

        long filePosition = 0;
        for ( int i = 0; i < partSizes.length; i++ ) {
            UploadPartTask cTask = new UploadPartTask( i + 1, filePosition,
                    partSizes[ i ] );
            mgr.addTask( cTask );
            filePosition += partSizes[ i ];
        }
        mgr.execute();
        Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );
        uploadPartAgainAndCheck();
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
            } catch ( AmazonS3Exception e ) {
                if ( e.getStatusCode() != 500 ) {
                    throw e;
                }
            } catch ( SdkClientException e ) {
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
