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
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.ListPartsRequest;
import com.amazonaws.services.s3.model.PartListing;
import com.amazonaws.services.s3.model.PartSummary;
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
import com.sequoias3.commlibs3.s3utils.PartUploadUtils;

/**
 * test content: 查询分段列表过程中db端节点异常 testlink-case: seqDB-18789
 * 
 * @author wangkexin
 * @Date 2019.08.16
 * @version 1.00
 */
public class ListPartsWithKillCoord18789 extends S3TestBase {
    private GroupMgr groupMgr = null;
    private String bucketName = "bucket18789";
    private String keyName = "key18789";
    private AmazonS3 s3Client = null;
    private GroupWrapper coordGroup = null;
    private long fileSize = 400 * 1024 * 1024;
    private long partSize = 2 * 1024 * 1024;
    private List< Integer > expPartNumberList = new ArrayList<>();
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
        PartUploadUtils.partUpload( s3Client, bucketName, keyName, uploadId,
                file, partSize );
        for ( int i = 0; i < fileSize / partSize; i++ ) {
            expPartNumberList.add( i + 1 );
        }

        TaskMgr mgr = new TaskMgr();
        for ( NodeWrapper node : coordGroup.getNodes() ) {
            FaultMakeTask faultTask = KillNode.getFaultMakeTask( node, 0 );
            mgr.addTask( faultTask );
        }

        ListPartsTask cTask = new ListPartsTask();
        mgr.addTask( cTask );
        mgr.execute();
        Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );
        Assert.assertEquals( groupMgr.checkBusinessWithLSN( 600 ), true,
                "checkBusinessWithLSN() occurs timeout" );
        listPartsAgainAndCheck();
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

    private class ListPartsTask extends OperateTask {
        @Override
        public void exec() throws InterruptedException {
            AmazonS3 s3Client = CommLibS3.buildS3Client();
            try {
                ListPartsRequest request = new ListPartsRequest( bucketName,
                        keyName, uploadId );
                PartListing listResult = s3Client.listParts( request );
                List< PartSummary > listParts = listResult.getParts();
                List< Integer > actPartNumbersList = new ArrayList<>();
                List< String > actEtagList = new ArrayList<>();
                for ( PartSummary partNumbers : listParts ) {
                    int partNumber = partNumbers.getPartNumber();
                    String etag = partNumbers.getETag();
                    actPartNumbersList.add( partNumber );
                    actEtagList.add( etag );
                }
                Assert.assertEquals( actPartNumbersList, expPartNumberList );
            } catch ( AmazonServiceException e ) {
                if ( e.getStatusCode() != 500 ) {
                    throw e;
                }
            } finally {
                if ( s3Client != null ) {
                    s3Client.shutdown();
                }
            }
        }
    }

    private void listPartsAgainAndCheck() throws Exception {
        ListPartsRequest request = new ListPartsRequest( bucketName, keyName,
                uploadId );
        PartListing listResult = s3Client.listParts( request );
        List< PartSummary > listParts = listResult.getParts();
        List< Integer > actPartNumbersList = new ArrayList<>();
        List< String > actEtagList = new ArrayList<>();
        for ( PartSummary partNumbers : listParts ) {
            int partNumber = partNumbers.getPartNumber();
            String etag = partNumbers.getETag();
            actPartNumbersList.add( partNumber );
            actEtagList.add( etag );
        }
        Assert.assertEquals( actPartNumbersList, expPartNumberList );
    }
}