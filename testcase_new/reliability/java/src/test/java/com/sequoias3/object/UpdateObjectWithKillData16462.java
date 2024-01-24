package com.sequoias3.object;

import java.io.File;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CopyOnWriteArrayList;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.amazonaws.AmazonServiceException;
import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.BucketVersioningConfiguration;
import com.amazonaws.services.s3.model.ListVersionsRequest;
import com.amazonaws.services.s3.model.S3VersionSummary;
import com.amazonaws.services.s3.model.VersionListing;
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

/**
 * test content: 更新对象过程中db端节点异常 testlink-case: seqDB-16462
 * 
 * @author wangkexin
 * @Date 2019.01.09
 * @version 1.00
 */
public class UpdateObjectWithKillData16462 extends S3TestBase {
    private String userName = "user16462";
    private String bucketName = "bucket16462";
    private String keyName = "key16462";
    private String roleName = "normal";
    private List< String > keyNames = new ArrayList< >();
    private List< String > updatedObjectList = new CopyOnWriteArrayList< String >();
    private int objectNum = 10;
    private String[] accessKeys = null;
    private AmazonS3 s3Client = null;
    private boolean runSuccess = false;
    private int fileSize = 1024 * 50;
    private int updateSize = 1024 * 200;
    private File localPath = null;
    private String filePath = null;
    private String updatePath = null;

    @BeforeClass
    private void setUp() throws Exception {
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        filePath = localPath + File.separator + "localFile_" + fileSize
                + ".txt";
        updatePath = localPath + File.separator + "localFile_" + updateSize
                + ".txt";
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );
        TestTools.LocalFile.createFile( updatePath, updateSize );

        CommLibS3.clearUser( userName );
        accessKeys = UserUtils.createUser( userName, roleName );
        s3Client = CommLibS3.buildS3Client( accessKeys[ 0 ], accessKeys[ 1 ] );
        s3Client.createBucket( bucketName );
        CommLibS3.setBucketVersioning( s3Client, bucketName,
                BucketVersioningConfiguration.ENABLED );

        for ( int i = 0; i < objectNum; i++ ) {
            String currentKey = keyName + "_" + i;
            s3Client.putObject( bucketName, currentKey, new File( filePath ) );
            keyNames.add( currentKey );
        }
    }

    @Test
    public void testUpdateObject() throws Exception {
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

        for ( int i = 0; i < keyNames.size(); i++ ) {
            UpdateObjectTask cTask = new UpdateObjectTask( keyNames.get( i ) );
            mgr.addTask( cTask );
        }
        mgr.execute();
        Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

        // check whether the cluster is normal and lsn consistency ,the
        // longest waiting time is 600S
        Assert.assertEquals( groupMgr.checkBusinessWithLSN( 600 ), true,
                "checkBusinessWithLSN() occurs timeout" );

        updateObjectAgainAndCheck();
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

    private class UpdateObjectTask extends OperateTask {
        private String keyName = "";

        public UpdateObjectTask( String keyName ) {
            this.keyName = keyName;
        }

        @Override
        public void exec() {
            AmazonS3 s3Client = CommLibS3.buildS3Client( accessKeys[ 0 ],
                    accessKeys[ 1 ] );
            try {
                s3Client.putObject( bucketName, keyName,
                        new File( updatePath ) );
                updatedObjectList.add( keyName );
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

    private void updateObjectAgainAndCheck() throws Exception {
        List< String > remainOldObjects = new ArrayList< String >();
        remainOldObjects.addAll( keyNames );
        remainOldObjects.removeAll( updatedObjectList );
        for ( String keyName : remainOldObjects ) {
            s3Client.putObject( bucketName, keyName, new File( updatePath ) );
        }

        VersionListing versions = s3Client.listVersions(
                new ListVersionsRequest().withBucketName( bucketName ) );
        List< S3VersionSummary > objects = versions.getVersionSummaries();

        String expCreateFileEtag = TestTools.getMD5( filePath );
        String expUpdateFileEtag = TestTools.getMD5( updatePath );

        for ( int i = 0; i < objects.size(); i++ ) {
            String key = objects.get( i ).getKey();
            String versionId = objects.get( i ).getVersionId();
            if ( versionId.equals( "0" ) ) {
                String actEtag = ObjectUtils.getMd5OfObject( s3Client,
                        localPath, bucketName, key, versionId );
                Assert.assertEquals( actEtag, expCreateFileEtag,
                        "objectName is : " + key + ",version=" + versionId );
                // 如果异常后重试成功，再次创建对象版本为2
            } else if ( versionId.equals( "1" ) || versionId.equals( "2" ) ) {
                String actEtag = ObjectUtils.getMd5OfObject( s3Client,
                        localPath, bucketName, key, versionId );
                Assert.assertEquals( actEtag, expUpdateFileEtag,
                        " objectName is : " + key + ",version=" + versionId );
            } else {
                Assert.fail( "versionId is error! id=" + versionId + "---key="
                        + key );
            }

        }
    }
}