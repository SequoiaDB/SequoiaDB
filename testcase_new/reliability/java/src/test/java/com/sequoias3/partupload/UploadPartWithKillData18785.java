package com.sequoias3.partupload;

import java.io.File;
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
 * test content: 上传分段长度相同，完成分段上传过程中db端节点异常 testlink-case: seqDB-18785
 * 
 * @author wangkexin
 * @Date 2019.08.16
 * @version 1.00
 */
public class UploadPartWithKillData18785 extends S3TestBase {
    private String bucketName = "bucket18785";
    private String keyName = "key18785";
    private AmazonS3 s3Client = null;
    private long fileSize = 38 * 1024 * 1024;
    private List< PartETag > partEtags = null;
    private String uploadId = "";
    private File localPath = null;
    private File file = null;
    private String filePath = null;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
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
        partEtags = PartUploadUtils.partUpload( s3Client, bucketName, keyName,
                uploadId, file );
        TaskMgr mgr = new TaskMgr();
        GroupMgr groupMgr = GroupMgr.getInstance();
        List< GroupWrapper > dataGroups = groupMgr.getAllDataGroup();

        for ( int i = 0; i < dataGroups.size(); i++ ) {
            String groupName = dataGroups.get( i ).getGroupName();
            GroupWrapper group = groupMgr.getGroupByName( groupName );
            NodeWrapper node = group.getMaster();
            FaultMakeTask faultTask = KillNode.getFaultMakeTask( node, 0 );
            mgr.addTask( faultTask );
        }
        mgr.addTask( new UploadpartUploadTask() );
        mgr.execute();
        Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );
        // check whether the cluster is normal and lsn consistency ,the longest
        // waiting time is 600S
        Assert.assertEquals( groupMgr.checkBusinessWithLSN( 600 ), true,
                "checkBusinessWithLSN() occurs timeout" );
        completeMultipartUploadAgainAndCheck();
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

    private class UploadpartUploadTask extends OperateTask {
        @Override
        public void exec() {
            AmazonS3 s3Client = CommLibS3.buildS3Client();
            try {
                PartUploadUtils.completeMultipartUpload( s3Client, bucketName,
                        keyName, uploadId, partEtags );
            } catch ( AmazonServiceException e ) {
                // Status Code: 0; Error Code: CompleteMultipartUploadFailed;
                // Request ID: null; S3 Extended Request ID: null
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

    private void completeMultipartUploadAgainAndCheck() throws Exception {
        try {
            PartUploadUtils.completeMultipartUpload( s3Client, bucketName,
                    keyName, uploadId, partEtags );
        } catch ( AmazonS3Exception e ) {
            if ( !e.getErrorCode().equals( "NoSuchUpload" ) ) {
                throw e;
            }
        }
        String expMd5 = TestTools.getMD5( filePath );
        String downloadMd5 = ObjectUtils.getMd5OfObject( s3Client, localPath,
                bucketName, keyName );
        Assert.assertEquals( downloadMd5, expMd5 );
    }
}