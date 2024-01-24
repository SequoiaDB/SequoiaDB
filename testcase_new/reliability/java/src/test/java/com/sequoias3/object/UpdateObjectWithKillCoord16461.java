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
import com.amazonaws.services.s3.model.ObjectMetadata;
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
 * @FileName seqDB-16461:开启版本控制，更新对象过程中db端节点异常
 * @Author wangkexin
 * @Date 2019.01.09
 */

public class UpdateObjectWithKillCoord16461 extends S3TestBase {
    private boolean runSuccess = false;
    private String userName = "user16461";
    private String roleName = "normal";
    private String[] accessKeys = null;
    private AmazonS3 s3Client = null;

    private String bucketName = "bucket16461";
    private String keyName = "key16461";
    private List< String > keyNames = new ArrayList< >();
    private int objectNum = 30;
    private int fileSize = 1024 * 50;
    private int updateSize = 1024 * 200;
    private File localPath = null;
    private String filePath = null;
    private String updatePath = null;
    private List< String > updatedObjectList = new CopyOnWriteArrayList< String >();

    private GroupMgr groupMgr = null;
    private GroupWrapper coordGroup = null;

    @BeforeClass
    private void setUp() throws Exception {
        groupMgr = GroupMgr.getInstance();
        coordGroup = groupMgr.getGroupByName( "SYSCoord" );

        CommLibS3.clearUser( userName );
        accessKeys = UserUtils.createUser( userName, roleName );
        s3Client = CommLibS3.buildS3Client( accessKeys[ 0 ], accessKeys[ 1 ] );
        s3Client.createBucket( bucketName );
        CommLibS3.setBucketVersioning( s3Client, bucketName,
                BucketVersioningConfiguration.ENABLED );

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
        for ( int i = 0; i < objectNum; i++ ) {
            String currentKey = keyName + "_" + i;
            s3Client.putObject( bucketName, currentKey, new File( filePath ) );
            keyNames.add( currentKey );
        }
    }

    @Test
    public void testUpdateObject() throws Exception {
        TaskMgr mgr = new TaskMgr();
        for ( NodeWrapper node : coordGroup.getNodes() ) {
            FaultMakeTask faultTask = KillNode.getFaultMakeTask( node, 0 );
            mgr.addTask( faultTask );
        }

        for ( int i = 0; i < keyNames.size(); i++ ) {
            UpdateObjectTask cTask = new UpdateObjectTask( keyNames.get( i ) );
            mgr.addTask( cTask );
        }
        mgr.execute();
        Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

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

    private void updateObjectAgainAndCheck() throws Exception {
        // 排除已被update成功的对象
        List< String > remainOldObjects = new ArrayList< String >();
        remainOldObjects.addAll( keyNames );
        remainOldObjects.removeAll( updatedObjectList );
        // 排除故障时实际update成功但是线程报错而未被加入到updatedObjectList的对象
        for ( String keyName : remainOldObjects ) {
            ObjectMetadata objectMetadata = null;
            try {

                objectMetadata = s3Client.getObjectMetadata( bucketName,
                        keyName );
            } catch ( AmazonServiceException e ) {
                // 元数据存在lob不存在时getObjectMetadata会失败，打印keyName方便定位
                System.out
                        .println( "getObjectMetadata failed, key: " + keyName );
                throw e;
            }
            if ( objectMetadata.getVersionId().equals( "1" ) ) {
                System.out.println(
                        "update return fail but actual success, keyName: "
                                + keyName );
                updatedObjectList.add( keyName );
            }
        }
        remainOldObjects.removeAll( updatedObjectList );
        // update未成功的对象
        for ( String keyName : remainOldObjects ) {
            s3Client.putObject( bucketName, keyName, new File( updatePath ) );
        }

        // 检查当前版本和历史版本所有对象metadata及lob正确性
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
