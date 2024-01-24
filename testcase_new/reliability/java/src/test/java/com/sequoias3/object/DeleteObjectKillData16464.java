package com.sequoias3.object;

import com.amazonaws.AmazonServiceException;
import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.BucketVersioningConfiguration;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.fault.KillNode;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;
import com.sequoias3.commlibs3.CommLibS3;
import com.sequoias3.commlibs3.S3TestBase;
import com.sequoias3.commlibs3.s3utils.UserUtils;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CopyOnWriteArrayList;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @description seqDB-16464:开启版本控制，删除对象过程中db端节点异常 *
 * @author wangkexin
 * @date 2019.01.14
 * @updateUser wuyan
 * @updateDate 2021.11.1
 * @updateRemark 减少操作对象数据量，在满足测试场景基础上减少大数据量同步等待时间
 * @version 1.00
 */
public class DeleteObjectKillData16464 extends S3TestBase {
    private String userName = "user16464";
    private String bucketName = "bucket16464";
    private String keyName = "key16464";
    private String roleName = "normal";
    private List< String > keyNames = new ArrayList<>();
    private List< String > deletedObjectList = new CopyOnWriteArrayList< String >();
    private int objectNum = 100;
    private String[] accessKeys = null;
    private AmazonS3 s3Client = null;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        CommLibS3.clearUser( userName );
        accessKeys = UserUtils.createUser( userName, roleName );
        s3Client = CommLibS3.buildS3Client( accessKeys[ 0 ], accessKeys[ 1 ] );
        s3Client.createBucket( bucketName );
        CommLibS3.setBucketVersioning( s3Client, bucketName,
                BucketVersioningConfiguration.ENABLED );

        for ( int i = 0; i < objectNum; i++ ) {
            String currentKey = keyName + "_" + i;
            s3Client.putObject( bucketName, currentKey,
                    currentKey + "_content" );
            keyNames.add( currentKey );
        }
    }

    @Test
    public void testDeleteObject() throws Exception {
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

        for ( int i = 0; i < keyNames.size(); i++ ) {
            DeleteObjectTask cTask = new DeleteObjectTask( keyNames.get( i ) );
            mgr.addTask( cTask );
        }
        mgr.execute();
        Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

        // check whether the cluster is normal and lsn consistency ,the longest
        // waiting time is 600S
        Assert.assertEquals( groupMgr.checkBusinessWithLSN( 600 ), true,
                "checkBusinessWithLSN() occurs timeout" );

        deleteObjectAgainAndCheck();
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
        private String keyName = "";

        public DeleteObjectTask( String keyName ) {
            this.keyName = keyName;
        }

        @Override
        public void exec() {
            AmazonS3 s3Client = CommLibS3.buildS3Client( accessKeys[ 0 ],
                    accessKeys[ 1 ] );
            try {
                s3Client.deleteVersion( bucketName, keyName, "0" );
                deletedObjectList.add( keyName );
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

    private void deleteObjectAgainAndCheck() throws Exception {
        List< String > remainObjects = new ArrayList< String >();
        remainObjects.addAll( keyNames );
        remainObjects.removeAll( deletedObjectList );
        for ( String keyName : remainObjects ) {
            s3Client.deleteVersion( bucketName, keyName, "0" );
        }
        for ( int i = 0; i < keyNames.size(); i++ ) {
            Assert.assertFalse(
                    s3Client.doesObjectExist( bucketName, keyNames.get( i ) ) );
        }
    }
}