package com.sequoias3.partupload;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
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
 * @Description seqDB-18787 :不存在为1的分段号，完成分段上传过程中SequoiaS3节点异常
 * @author wuyan
 * @Date 2019.08.13
 * @version 1.00
 */
public class CompleteMultipartUploadAndS3ReStart18787 extends S3TestBase {
    private boolean runSuccess = false;
    private AmazonS3 s3Client = null;
    private String bucketName = "bucket18787";
    private String keyName = "/dir/test18787.txt";
    private int fileSize = 1024 * 1024 * 138;
    private String filePath = null;
    private File localPath = null;
    private File file = null;

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
        List< PartETag > partEtags = uploadParts( uploadId );

        FaultMakeTask faultMakeTask = S3NodeRestart
                .getFaultMakeTask( new S3NodeWrapper(), 0, 15 );
        TaskMgr mgr = new TaskMgr( faultMakeTask );
        mgr.addTask( new CompleteMultipartUpload( uploadId, partEtags ) );

        mgr.execute();
        Assert.assertTrue( mgr.isAllSuccess(), mgr.getErrorMsg() );

        checkResult( uploadId, partEtags );
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

    public class CompleteMultipartUpload extends OperateTask {
        private List< PartETag > partEtags = new ArrayList< >();
        private String uploadId;
        private AmazonS3 s3Client1 = CommLibS3.buildS3Client();

        private CompleteMultipartUpload( String uploadId,
                List< PartETag > partEtags ) {
            this.uploadId = uploadId;
            this.partEtags = partEtags;
        }

        @Override
        public void exec() throws Exception {
            try {
                PartUploadUtils.completeMultipartUpload( s3Client1, bucketName,
                        keyName, uploadId, partEtags );
            } catch ( AmazonS3Exception e ) {
                // e:0 Get connection failed.e:404 NoSuchUpload.
                if ( e.getStatusCode() != 0 && e.getStatusCode() != 500
                        && e.getStatusCode() != 404 ) {
                    throw new Exception( keyName, e );
                }
            } catch ( SdkClientException e ) {
                if ( !e.getMessage()
                        .contains( "Unable to execute HTTP request" ) ) {
                    throw new Exception( keyName, e );
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

    private List< PartETag > uploadParts( String uploadId ) {
        int[] partSizes = { 1024 * 1024 * 6, 1024 * 1024 * 5, 1024 * 1024 * 6,
                1024 * 1024 * 10, 1024 * 1024 * 20, 1024 * 1024 * 6,
                1024 * 1024 * 8, 1024 * 1024 * 20, 1024 * 1024 * 50,
                1024 * 1024 * 7 };
        int partNumbers = 10;
        int filePosition = 0;
        List< PartETag > partEtags = new ArrayList< >();
        for ( int i = 0; i < partNumbers; i++ ) {
            // 分段号从2开始，不存在为1的分段号
            int partNumber = i + 2;
            int partSize = partSizes[ i ];
            long eachPartSize = Math.min( partSize, fileSize - filePosition );
            UploadPartRequest partRequest = new UploadPartRequest()
                    .withFile( file ).withFileOffset( filePosition )
                    .withPartNumber( partNumber ).withPartSize( eachPartSize )
                    .withBucketName( bucketName ).withKey( keyName )
                    .withUploadId( uploadId );
            UploadPartResult uploadPartResult = s3Client
                    .uploadPart( partRequest );
            partEtags.add( uploadPartResult.getPartETag() );
            filePosition += partSize;
        }
        return partEtags;
    }

    private void checkResult( String uploadId, List< PartETag > partEtags )
            throws Exception {
        // 重新完成分段上传，如果故障时完成分段上传已成功，则再次完成分段上传会报错NoSuchUpload
        try {
            PartUploadUtils.completeMultipartUpload( s3Client, bucketName,
                    keyName, uploadId, partEtags );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "NoSuchUpload",
                    "the error is " + e.getErrorMessage() );
        }

        // 检查上传对象内容
        String downfileMd5 = ObjectUtils.getMd5OfObject( s3Client, localPath,
                bucketName, keyName );
        Assert.assertEquals( downfileMd5, TestTools.getMD5( filePath ) );
    }
}
