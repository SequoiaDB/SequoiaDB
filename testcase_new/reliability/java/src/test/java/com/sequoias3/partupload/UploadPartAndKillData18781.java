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
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.SdbTestBase;
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
 * @Description seqDB-18781 : 再次上传相同分段过程中db端节点故障
 * @author wuyan
 * @Date 2019.08.12
 * @version 1.00
 */
public class UploadPartAndKillData18781 extends S3TestBase {
    private boolean runSuccess = false;
    private AmazonS3 s3Client = null;
    private String bucketName = "bucket18781";
    private String keyName = "/dir/test18781.tar";
    private int fileSize = 1024 * 1024 * 100;
    private int partSize = 1024 * 1024 * 10;
    private File localPath = null;
    private String filePath1 = null;
    private String filePath2 = null;

    private File file1 = null;
    private File file2 = null;
    private List< PartETag > partEtags = Collections
            .synchronizedList( new ArrayList< PartETag >() );

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( SdbTestBase.workDir + File.separator
                + TestTools.getClassName() );
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        filePath1 = localPath + File.separator + "localFile_" + fileSize
                + ".txt";
        filePath2 = localPath + File.separator + "localFile_" + fileSize
                + ".txt";
        TestTools.LocalFile.createFile( filePath1, fileSize );
        file1 = new File( filePath1 );
        file2 = new File( filePath2 );
        s3Client = CommLibS3.buildS3Client();
        CommLibS3.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
    }

    @Test
    public void test() throws Exception {
        String uploadId = PartUploadUtils.initPartUpload( s3Client, bucketName,
                keyName );
        List< PartETag > partEtags1 = PartUploadUtils.partUpload( s3Client,
                bucketName, keyName, uploadId, file1, partSize );
        TaskMgr mgr = new TaskMgr();
        int partNums = fileSize / partSize;
        for ( int i = 0; i < partNums; i++ ) {
            int partNum = i + 1;
            mgr.addTask( new PartUpload( partNum, partSize, file2, uploadId ) );
        }

        GroupMgr groupMgr = GroupMgr.getInstance();
        List< GroupWrapper > glist = groupMgr.getAllDataGroup();
        for ( int i = 0; i < glist.size(); i++ ) {
            String groupName = glist.get( i ).getGroupName();
            GroupWrapper group = groupMgr.getGroupByName( groupName );
            NodeWrapper node = group.getMaster();
            FaultMakeTask faultTask = KillNode.getFaultMakeTask( node, 4 );
            mgr.addTask( faultTask );
            System.out.println( "KillNode:i=" + i + "" + node.hostName() + ":"
                    + node.svcName() );
        }

        mgr.execute();
        Assert.assertTrue( mgr.isAllSuccess(), mgr.getErrorMsg() );
        Assert.assertTrue( groupMgr.checkBusinessWithLSN( 120 ),
                "node start fail!" );

        checkResult( uploadId, partEtags1 );
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
            } catch ( AmazonS3Exception e ) {
                if ( e.getStatusCode() != 500 ) {
                    throw new Exception( keyName + ":" + partNum, e );
                }
            } catch ( SdkClientException e ) {
                if ( !e.getMessage()
                        .contains( "Unable to execute HTTP request" ) ) {
                    throw new Exception( keyName + ":" + partNum, e );
                }
            } finally {
                if ( s3Client1 != null ) {
                    s3Client1.shutdown();
                }
            }
        }
    }

    private void checkResult( String uploadId, List< PartETag > partEtags )
            throws Exception {
        // 完成分段上传
        PartUploadUtils.completeMultipartUpload( s3Client, bucketName, keyName,
                uploadId, partEtags );
        // 检查上传文件内容
        String downfileMd5 = ObjectUtils.getMd5OfObject( s3Client, localPath,
                bucketName, keyName );
        Assert.assertEquals( downfileMd5, TestTools.getMD5( filePath1 ) );
    }
}
