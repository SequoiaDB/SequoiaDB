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
 * @Description seqDB-18780 :上传分段过程中db端节点故障，其中分段长度相同
 * @author wuyan
 * @Date 2019.08.12
 * @version 1.00
 */
public class UploadPartAndKillData18780 extends S3TestBase {
    private boolean runSuccess = false;
    private AmazonS3 s3Client = null;
    private String bucketName = "bucket18780";
    private String keyName = "/dir/test18780.tar";
    private int fileSize = 1024 * 1024 * 100;
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
        TaskMgr mgr = new TaskMgr();
        int partNums = fileSize / partSize;
        for ( int i = 0; i < partNums; i++ ) {
            int partNum = i + 1;
            mgr.addTask( new PartUpload( partNum, partSize, file, uploadId ) );
            expPartNums.add( partNum );
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
