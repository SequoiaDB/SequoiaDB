package com.sequoias3.object;

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
import com.sequoias3.commlibs3.TestTools;
import com.sequoias3.commlibs3.s3utils.ObjectUtils;
import com.sequoias3.commlibs3.s3utils.UserUtils;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CopyOnWriteArrayList;

/**
 * test content: 获取对象过程中db端节点异常 testlink-case: seqDB-16463
 * 
 * @author wangkexin
 * @Date 2019.01.14
 * @version 1.00
 */
public class GetObjectKillCoord16463 extends S3TestBase {
    private GroupMgr groupMgr = null;
    private String userName = "user16463";
    private String bucketName = "bucket16463";
    private String keyName = "key16463";
    private String roleName = "normal";
    private List< String > keyNames = new ArrayList<>();
    private List< String > getObjectList = new CopyOnWriteArrayList< String >();
    private int objectNum = 100;
    private String[] accessKeys = null;
    private AmazonS3 s3Client = null;
    private File localPath = null;
    private GroupWrapper coordGroup = null;

    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        groupMgr = GroupMgr.getInstance();
        coordGroup = groupMgr.getGroupByName( "SYSCoord" );

        CommLibS3.clearUser( userName );
        accessKeys = UserUtils.createUser( userName, roleName );
        s3Client = CommLibS3.buildS3Client( accessKeys[ 0 ], accessKeys[ 1 ] );
        s3Client.createBucket( bucketName );

        for ( int i = 0; i < objectNum; i++ ) {
            String currentKey = keyName + "_" + i;
            s3Client.putObject( bucketName, currentKey, currentKey );
            keyNames.add( currentKey );
        }
    }

    @Test
    public void testGetObject() throws Exception {
        TaskMgr mgr = new TaskMgr();
        for ( NodeWrapper node : coordGroup.getNodes() ) {
            FaultMakeTask faultTask = KillNode.getFaultMakeTask( node, 0 );
            mgr.addTask( faultTask );
        }

        for ( int i = 0; i < keyNames.size(); i++ ) {
            GetObjectTask cTask = new GetObjectTask( keyNames.get( i ) );
            mgr.addTask( cTask );
        }
        mgr.execute();
        Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

        getObjectAndCheck();
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( runSuccess ) {
                UserUtils.deleteUser( userName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }

    private class GetObjectTask extends OperateTask {
        private String keyName = "";

        public GetObjectTask( String keyName ) {
            this.keyName = keyName;
        }

        @Override
        public void exec() throws Exception {
            AmazonS3 s3Client = CommLibS3.buildS3Client( accessKeys[ 0 ],
                    accessKeys[ 1 ] );
            try {
                String downfileMd5 = ObjectUtils.getMd5OfObject( s3Client,
                        localPath, bucketName, keyName );
                String expEtag = TestTools.getMD5( keyName.getBytes() );
                Assert.assertEquals( downfileMd5, expEtag, "key : " + keyName );
                getObjectList.add( keyName );
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

    private void getObjectAndCheck() throws Exception {
        List< String > remainObjects = new ArrayList< String >();
        remainObjects.addAll( keyNames );
        remainObjects.removeAll( getObjectList );
        for ( String keyName : remainObjects ) {
            String downfileMd5 = ObjectUtils.getMd5OfObject( s3Client,
                    localPath, bucketName, keyName );
            String expEtag = TestTools.getMD5( keyName.getBytes() );
            Assert.assertEquals( downfileMd5, expEtag, "key : " + keyName );
        }
    }
}