package com.sequoias3.privilege;

import java.io.File;
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
import com.amazonaws.services.s3.model.CannedAccessControlList;
import com.amazonaws.services.s3.model.Grant;
import com.amazonaws.services.s3.model.GroupGrantee;
import com.amazonaws.services.s3.model.Permission;
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
import com.sequoias3.commlibs3.s3utils.PrivilegeUtils;

/**
 * @description seqDB-19484 :更新对象acl过程中db端节点异常
 * @author wangkexin
 * @date 2019.09.26
 * @updateUser wuyan
 * @updateDate 2021.11.1
 * @updateRemark 减少操作对象数据量，在满足测试场景基础上减少大数据量同步等待时间
 * @version 1.00
 */
public class SetObjectAclAndKillData19484 extends S3TestBase {
    private boolean runSuccess = false;
    private AmazonS3 s3Client = null;
    private String bucketName = "bucket19484";
    private String keyName_base = "key19484";
    private int keyNum = 50;
    private int fileSize = 100 * 1024;
    private String filePath = null;
    private File localPath = null;
    private File file = null;
    private List< String > setObjectAclSucceedList = new CopyOnWriteArrayList< String >();
    private List< String > keyNameList = new ArrayList<>();

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

        // put objects and set object acl
        for ( int i = 0; i < keyNum; i++ ) {
            String keyName = keyName_base + "_" + i;
            s3Client.putObject( bucketName, keyName, file );
            keyNameList.add( keyName );
            s3Client.setObjectAcl( bucketName, keyName,
                    CannedAccessControlList.PublicRead );
        }
    }

    @Test
    public void test() throws Exception {
        // kill data node
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

        // set object acl
        Grant expGrant = new Grant( GroupGrantee.AllUsers, Permission.ReadAcp );
        for ( String key : keyNameList ) {
            mgr.addTask( new SetObjectAcl( key, expGrant ) );
        }
        mgr.execute();
        Assert.assertTrue( mgr.isAllSuccess(), mgr.getErrorMsg() );
        // check whether the cluster is normal and lsn consistency ,the longest
        // waiting time is 600S
        Assert.assertEquals( groupMgr.checkBusinessWithLSN( 600 ), true,
                "checkBusinessWithLSN() occurs timeout" );

        // set obeject acl again
        setObjectAclAgain( expGrant );
        checkResult( expGrant );
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

    public class SetObjectAcl extends OperateTask {
        private String keyName;
        private Grant[] grants;
        private AmazonS3 s3 = CommLibS3.buildS3Client();

        private SetObjectAcl( String keyName, Grant... grants ) {
            this.keyName = keyName;
            this.grants = grants;
        }

        @Override
        public void exec() throws Exception {
            try {
                PrivilegeUtils.setObjectAclByBody( s3, bucketName, keyName,
                        grants );
                setObjectAclSucceedList.add( keyName );
            } catch ( AmazonServiceException e ) {
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

    private void setObjectAclAgain( Grant... grants ) {
        List< String > setAgainKeyList = new ArrayList<>( keyNameList );
        setAgainKeyList.removeAll( setObjectAclSucceedList );
        for ( String key : setAgainKeyList ) {
            PrivilegeUtils.setObjectAclByBody( s3Client, bucketName, key,
                    grants );
        }
    }

    private void checkResult( Grant... expGrants ) throws Exception {
        for ( String key : keyNameList ) {
            PrivilegeUtils.checkSetObjectAclResult( s3Client, bucketName, key,
                    expGrants );
        }
    }
}
