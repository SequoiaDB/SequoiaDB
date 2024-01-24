package com.sequoias3.bucket;

import java.util.ArrayList;
import java.util.List;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.amazonaws.AmazonServiceException;
import com.amazonaws.services.s3.AmazonS3;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.fault.KillNode;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;
import com.sequoias3.commlibs3.CommLibS3;
import com.sequoias3.commlibs3.S3TestBase;

/**
 * @Description seqDB-15907:创建桶过程中db端节点异常
 * @author fanyu
 * @Date:2020年01月06日
 * @version 1.00
 */
public class CreateBucketWithKillData15907 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketNameBase = "bucket15907-";
    private int bucketNum = 80;
    private List< String > bucketNameList = new ArrayList<>();
    private List< String > successBucketList = new ArrayList<>();
    private AmazonS3 s3Client = null;

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLibS3.buildS3Client();
        for ( int i = 0; i < bucketNum; i++ ) {
            CommLibS3.clearBucket( s3Client, bucketNameBase + i );
            bucketNameList.add( bucketNameBase + i );
        }
    }

    @Test
    public void testCreateBucket() throws Exception {
        TaskMgr mgr = new TaskMgr();
        GroupMgr groupMgr = GroupMgr.getInstance();
        List< GroupWrapper > dataGroups = groupMgr.getAllDataGroup();
        for ( int i = 0; i < dataGroups.size(); i++ ) {
            String groupName = dataGroups.get( i ).getGroupName();
            GroupWrapper group = groupMgr.getGroupByName( groupName );
            NodeWrapper node = group.getMaster();
            FaultMakeTask faultTask = KillNode.getFaultMakeTask( node, 1 );
            mgr.addTask( faultTask );
        }
        for ( int i = 0; i < bucketNum; i++ ) {
            CreateBucket cTask = new CreateBucket( bucketNameList.get( i ) );
            mgr.addTask( cTask );
        }
        mgr.execute();
        Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );
        // check whether the cluster is normal and lsn consistency ,the
        // longest waiting time is 600S
        Assert.assertEquals( groupMgr.checkBusinessWithLSN( 120 ), true,
                "checkBusinessWithLSN() occurs timeout" );
        //check results
        for ( String bucketName : successBucketList ) {
            Assert.assertTrue( s3Client.doesBucketExistV2( bucketName ),
                    "bucketName = " + bucketName );
        }
        //create failed bucket again
        bucketNameList.removeAll( successBucketList );
        for ( String bucketName : bucketNameList ) {
            s3Client.createBucket( bucketName );
            successBucketList.add( bucketName );
        }
        //check bucket existed
        for ( String bucketName : bucketNameList ) {
            Assert.assertTrue( s3Client.doesBucketExistV2( bucketName ),
                    "bucketName = " + bucketName );
        }
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( runSuccess ) {
                for ( String bucketName : successBucketList ) {
                    s3Client.deleteBucket( bucketName );
                }
            }
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }

    private class CreateBucket extends OperateTask {
        private String bucketName;

        public CreateBucket( String bucketName ) {
            this.bucketName = bucketName;
        }

        @Override
        public void exec() {
            AmazonS3 s3Client = CommLibS3.buildS3Client();
            try {
                s3Client.createBucket( bucketName );
                successBucketList.add( bucketName );
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
}