package com.sequoias3.delimiter;

import com.amazonaws.AmazonServiceException;
import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.BucketVersioningConfiguration;
import com.amazonaws.services.s3.model.S3Object;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.fault.KillNode;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;
import com.sequoias3.commlibs3.CommLibS3;
import com.sequoias3.commlibs3.S3TestBase;
import com.sequoias3.commlibs3.s3utils.DelimiterUtils;
import com.sequoias3.commlibs3.s3utils.UserUtils;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * test content: 开启版本控制，删除对象过程中db端节点异常 testlink-case: seqDB-18200
 * 
 * @author wangkexin
 * @Date 2019.05.10
 * @version 1.00
 */
public class DeleteObjectWithKillCoord18200 extends S3TestBase {
    private GroupMgr groupMgr = null;
    private String userName = "user18200";
    private String bucketName = "bucket18200";
    private String keyName = "dir1/key18200?test.txt";
    private String delimiter = "?";
    private String roleName = "normal";
    private String context = "content18200";
    private int versionNum = 20;
    private String[] accessKeys = null;
    private AmazonS3 s3Client = null;
    private GroupWrapper coordGroup = null;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        groupMgr = GroupMgr.getInstance();
        coordGroup = groupMgr.getGroupByName( "SYSCoord" );

        CommLibS3.clearUser( userName );
        accessKeys = UserUtils.createUser( userName, roleName );
        s3Client = CommLibS3.buildS3Client( accessKeys[ 0 ], accessKeys[ 1 ] );
        CommLibS3.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
        CommLibS3.setBucketVersioning( s3Client, bucketName,
                BucketVersioningConfiguration.ENABLED );
        DelimiterUtils.putBucketDelimiter( bucketName, delimiter,
                accessKeys[ 0 ] );
        DelimiterUtils.checkCurrentDelimiteInfo( bucketName, delimiter,
                accessKeys[ 0 ] );
        for ( int i = 0; i < versionNum; i++ ) {
            s3Client.putObject( bucketName, keyName, context );
        }
    }

    @Test
    public void testDeleteObject() throws Exception {
        // kill coord node
        TaskMgr mgr = new TaskMgr();
        for ( NodeWrapper node : coordGroup.getNodes() ) {
            FaultMakeTask faultTask = KillNode.getFaultMakeTask( node, 0 );
            mgr.addTask( faultTask );
        }

        DeleteObjectTask dTask = new DeleteObjectTask();
        mgr.addTask( dTask );
        mgr.execute();
        Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

        // delete again
        if ( s3Client.doesObjectExist( bucketName, keyName ) ) {
            S3Object obj = s3Client.getObject( bucketName, keyName );
            int versionid = Integer
                    .valueOf( obj.getObjectMetadata().getVersionId() );
            for ( int i = versionid; i >= 0; i-- ) {
                s3Client.deleteVersion( bucketName, keyName,
                        String.valueOf( i ) );
            }
            Assert.assertFalse(
                    s3Client.doesObjectExist( bucketName, keyName ) );
        }

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( runSuccess ) {
                UserUtils.deleteUser( userName );
            }
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }

    private class DeleteObjectTask extends OperateTask {
        @Override
        public void exec() throws Exception {
            AmazonS3 s3Client = CommLibS3.buildS3Client( accessKeys[ 0 ],
                    accessKeys[ 1 ] );
            try {
                for ( int i = versionNum - 1; i >= 0; i-- ) {
                    s3Client.deleteVersion( bucketName, keyName,
                            String.valueOf( i ) );
                }
            } catch ( AmazonServiceException e ) {
                if ( !e.getErrorCode().equals( "GetDBConnectFail" ) ) {
                    throw e;
                }
            } finally {
                if ( s3Client != null ) {
                    s3Client.shutdown();
                }
            }
        }
    }
}