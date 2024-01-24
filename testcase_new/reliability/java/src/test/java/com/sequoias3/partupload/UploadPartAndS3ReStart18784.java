package com.sequoias3.partupload;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.amazonaws.SdkClientException;
import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.PartETag;
import com.amazonaws.services.s3.model.UploadPartRequest;
import com.amazonaws.services.s3.model.UploadPartResult;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;
import com.sequoias3.commlibs3.CommLibS3;
import com.sequoias3.commlibs3.S3TestBase;
import com.sequoias3.commlibs3.TestTools;
import com.sequoias3.commlibs3.s3utils.ObjectUtils;
import com.sequoias3.commlibs3.s3utils.PartUploadUtils;
import com.sequoias3.commlibs3.s3utils.S3NodeRestart;
import com.sequoias3.commlibs3.s3utils.bean.S3NodeWrapper;

/**
 * @Description seqDB-18784 :上传分段过程中sequoiaS3故障，故障恢复后再次上传分段
 * @author wuyan
 * @Date 2019.08.13
 * @version 1.00
 */
public class UploadPartAndS3ReStart18784 extends S3TestBase {
    private boolean runSuccess = false;
    private AmazonS3 s3Client = null;
    private String bucketName = "bucket18784";
    private String keyName = "/dir/test18784.pdf";
    private int fileSize = 1024 * 1024 * 200;
    private int partSize = 1024 * 1024 * 5;
    private String filePath = null;
    private File localPath = null;
    private File file = null;
    private List< PartETag > partEtags = Collections
            .synchronizedList( new ArrayList< PartETag >() );
    private List< Integer > uploadSuccessPartNums = Collections
            .synchronizedList( new ArrayList< Integer >() );
    private List< Integer > expPartNums = new ArrayList<>();

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
        String uploadId = PartUploadUtils.initPartUpload( s3Client, bucketName,
                keyName );
        FaultMakeTask faultMakeTask = S3NodeRestart
                .getFaultMakeTask( new S3NodeWrapper(), 5, 8 );
        TaskMgr mgr = new TaskMgr( faultMakeTask );

        int partNums = fileSize / partSize;
        for ( int i = 0; i < partNums; i++ ) {
            int partNum = i + 1;
            mgr.addTask( new PartUpload( partNum, partSize, file, uploadId ) );
            expPartNums.add( partNum );
        }

        mgr.execute();
        Assert.assertTrue( mgr.isAllSuccess(), mgr.getErrorMsg() );

        checkResult( uploadId );
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

    public class PartUpload extends OperateTask {
        private int partNum;
        private int partSize;
        private File file;
        private String uploadId;
        private AmazonS3 s3Client1 = CommLibS3.buildS3Client();

        private PartUpload( int partNum, int partSize, File file,
                String uploadId ) {
            this.partNum = partNum;
            this.partSize = partSize;
            this.file = file;
            this.uploadId = uploadId;
        }

        @Override
        public void exec() throws Exception {
            try {
                int filePosition = ( partNum - 1 ) * partSize;
                UploadPartRequest partRequest = new UploadPartRequest()
                        .withFile( file ).withFileOffset( filePosition )
                        .withPartNumber( partNum ).withPartSize( partSize )
                        .withBucketName( bucketName ).withKey( keyName )
                        .withUploadId( uploadId );
                UploadPartResult uploadPartResult = s3Client1
                        .uploadPart( partRequest );
                partEtags.add( uploadPartResult.getPartETag() );
                uploadSuccessPartNums.add( partNum );
            } catch ( AmazonS3Exception e ) {
                if ( e.getStatusCode() != 500 ) {
                    throw new Exception( keyName + ":" + partNum, e );
                }
            } catch ( SdkClientException e ) {
                if ( !e.getMessage()
                        .contains( "Unable to execute HTTP request" ) ) {
                    throw new Exception( keyName + ":" + partNum, e );
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

    private void checkResult( String uploadId ) throws Exception {
        expPartNums.removeAll( uploadSuccessPartNums );
        // 重新上传失败的分段
        for ( int partNum : expPartNums ) {
            int filePosition = ( partNum - 1 ) * partSize;
            UploadPartRequest partRequest = new UploadPartRequest()
                    .withFile( file ).withFileOffset( filePosition )
                    .withPartNumber( partNum ).withPartSize( partSize )
                    .withBucketName( bucketName ).withKey( keyName )
                    .withUploadId( uploadId );
            UploadPartResult uploadPartResult = s3Client
                    .uploadPart( partRequest );
            partEtags.add( uploadPartResult.getPartETag() );
        }

        // 完成分段上传
        PartUploadUtils.completeMultipartUpload( s3Client, bucketName, keyName,
                uploadId, partEtags );

        // check the upload file
        String downfileMd5 = ObjectUtils.getMd5OfObject( s3Client, localPath,
                bucketName, keyName );
        Assert.assertEquals( downfileMd5, TestTools.getMD5( filePath ) );
    }
}
