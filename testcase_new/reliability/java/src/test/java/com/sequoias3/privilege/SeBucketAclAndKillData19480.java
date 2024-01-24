package com.sequoias3.privilege;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CopyOnWriteArrayList;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.amazonaws.AmazonServiceException;
import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CanonicalGrantee;
import com.amazonaws.services.s3.model.Grant;
import com.amazonaws.services.s3.model.Permission;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.fault.KillNode;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;
import com.sequoias3.commlibs3.CommLibS3;
import com.sequoias3.commlibs3.S3TestBase;
import com.sequoias3.commlibs3.s3utils.PrivilegeUtils;

/**
 * @Description seqDB-19480:配置桶acl过程中db端节点异常
 * @Author huangxiaoni
 * @Date 2019.09.26
 */
public class SeBucketAclAndKillData19480 extends S3TestBase {
    private boolean runSuccess = false;
    private int threadNum = 20;
    private String tcId = "19480";
    private AmazonS3 adminS3 = null;
    private String ownerId;
    private String bucketNameBase = "bucket" + tcId + "a";
    private List< String > bucketNames = new ArrayList<>();
    private Grant grant;
    private List< String > setBucketAclFailList = new CopyOnWriteArrayList< String >();

    @BeforeClass
    private void setUp() throws IOException {
        adminS3 = CommLibS3.buildS3Client();
        ownerId = adminS3.getS3AccountOwner().getId();
        for ( int i = 0; i < threadNum; i++ ) {
            String bucketName = bucketNameBase + i;
            CommLibS3.clearBucket( adminS3, bucketName );
            adminS3.createBucket( bucketName );
            bucketNames.add( bucketName );
        }
        grant = new Grant( new CanonicalGrantee( ownerId ), Permission.Read );
    }

    @Test
    public void test() throws Exception {
        TaskMgr mgr = new TaskMgr();
        // set bucket acl
        for ( String bucketName : bucketNames ) {
            mgr.addTask( new ThreadSetBucketAcl( bucketName ) );
        }

        // kill data node
        GroupMgr groupMgr = GroupMgr.getInstance();
        List< GroupWrapper > dataGroups = groupMgr.getAllDataGroup();
        for ( int i = 0; i < dataGroups.size(); i++ ) {
            String groupName = dataGroups.get( i ).getGroupName();
            GroupWrapper group = groupMgr.getGroupByName( groupName );
            NodeWrapper node = group.getMaster();
            FaultMakeTask faultTask = KillNode.getFaultMakeTask( node, 0 );
            mgr.addTask( faultTask );
        }

        mgr.execute();
        Assert.assertTrue( mgr.isAllSuccess(), mgr.getErrorMsg() );

        // wait business recover
        Assert.assertEquals( groupMgr.checkBusinessWithLSN( 600 ), true,
                "checkBusinessWithLSN() occurs timeout" );

        // set failed bucket again
        for ( String bucketName : setBucketAclFailList ) {
            PrivilegeUtils.setBucketAclByBody( adminS3, bucketName, grant );
        }

        // check all bucket restults
        for ( String bucketName : bucketNames ) {
            PrivilegeUtils.checkSetBucketAclResult( adminS3, bucketName,
                    grant );
        }

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                for ( String bucketName : bucketNames ) {
                    CommLibS3.clearBucket( adminS3, bucketName );
                }
            }
        } finally {
            if ( adminS3 != null )
                adminS3.shutdown();
        }
    }

    private class ThreadSetBucketAcl extends OperateTask {
        private String bucketName;

        private ThreadSetBucketAcl( String bucketName ) {
            this.bucketName = bucketName;
        }

        @Override
        public void exec() throws Exception {
            AmazonS3 s3 = null;
            try {
                s3 = CommLibS3.buildS3Client();
                PrivilegeUtils.setBucketAclByBody( s3, bucketName, grant );
            } catch ( AmazonServiceException e ) {
                setBucketAclFailList.add( bucketName );
                if ( e.getStatusCode() != 500 ) {
                    throw e;
                }
            } finally {
                if ( s3 != null ) {
                    s3.shutdown();
                }
            }
        }
    }
}
