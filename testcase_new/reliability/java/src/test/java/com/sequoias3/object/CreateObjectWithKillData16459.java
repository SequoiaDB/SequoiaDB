package com.sequoias3.object;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Random;
import java.util.concurrent.CopyOnWriteArrayList;

import com.amazonaws.services.s3.model.*;
import org.testng.Assert;
import org.testng.SkipException;
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
import com.sequoias3.commlibs3.TestTools;
import com.sequoias3.commlibs3.s3utils.ObjectUtils;
import com.sequoias3.commlibs3.s3utils.UserUtils;

/**
 * @Description seqDB-16459： 开启版本控制，创建对象过程中db端节点异常
 * @author wangkexin
 * @Date 2019.01.09
 * @updateAuthor wuyan
 * @updateDate 2021/8/24
 * @version 1.00
 */
public class CreateObjectWithKillData16459 extends S3TestBase {
    private String userName = "user16459";
    private String bucketName = "bucket16459";
    private String keyName = "key16459";
    private String roleName = "normal";
    private List< String > keyNames = new ArrayList<>();
    private Random random = new Random();
    private List< String > putObjectList = new CopyOnWriteArrayList< String >();
    private int objectNum = 100;
    private String[] accessKeys = null;
    private String currContent = "";
    private AmazonS3 s3Client = null;
    private GroupMgr groupMgr = null;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {

        groupMgr = GroupMgr.getInstance();
        // CheckBusiness(true),检测当前集群环境，若存在异常返回false，
        if ( !groupMgr.checkBusiness( 20 ) ) {
            throw new SkipException( "checkBusiness return false" );
        }
        CommLibS3.clearUser( userName );
        accessKeys = UserUtils.createUser( userName, roleName );
        s3Client = CommLibS3.buildS3Client( accessKeys[ 0 ], accessKeys[ 1 ] );
        s3Client.createBucket( bucketName );
        CommLibS3.setBucketVersioning( s3Client, bucketName,
                BucketVersioningConfiguration.ENABLED );

        for ( int i = 0; i < objectNum; i++ ) {
            keyNames.add( keyName + "_" + i );
        }

        int writeSize = random.nextInt( 1024 );
        currContent = ObjectUtils.getRandomString( writeSize );
    }

    @Test
    public void testCreateObject() throws Exception {
        TaskMgr mgr = new TaskMgr();
        List< GroupWrapper > dataGroups = groupMgr.getAllDataGroup();
        for ( int i = 0; i < dataGroups.size(); i++ ) {
            String groupName = dataGroups.get( i ).getGroupName();
            GroupWrapper group = groupMgr.getGroupByName( groupName );
            NodeWrapper node = group.getMaster();
            FaultMakeTask faultTask = KillNode.getFaultMakeTask( node, 1 );
            mgr.addTask( faultTask );
        }

        for ( int i = 0; i < keyNames.size(); i++ ) {
            CreateObjectTask cTask = new CreateObjectTask( keyNames.get( i ) );
            mgr.addTask( cTask );
        }
        mgr.execute();
        Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

        // check whether the cluster is normal and lsn consistency ,the
        // longest waiting time is 600S
        Assert.assertEquals( groupMgr.checkBusinessWithLSN( 600 ), true,
                "checkBusinessWithLSN() occurs timeout" );

        putObjectAndCheck();
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

    private class CreateObjectTask extends OperateTask {
        private String keyName = "";

        public CreateObjectTask( String keyName ) {
            this.keyName = keyName;
        }

        @Override
        public void exec() {
            AmazonS3 s3Client = CommLibS3.buildS3Client( accessKeys[ 0 ],
                    accessKeys[ 1 ] );
            try {
                s3Client.putObject( bucketName, keyName, currContent );
                putObjectList.add( keyName );
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

    private void putObjectAndCheck() throws Exception {
        List< String > remainObjects = new ArrayList< String >();
        remainObjects.addAll( keyNames );
        remainObjects.removeAll( putObjectList );
        for ( String keyName : remainObjects ) {
            // 如果对象实际上传成功未返回，则该对象不再重复上传
            if ( !s3Client.doesObjectExist( bucketName, keyName ) ) {
                s3Client.putObject( bucketName, keyName, currContent );
            }
        }

        VersionListing versions = s3Client.listVersions(
                new ListVersionsRequest().withBucketName( bucketName ) );
        List< S3VersionSummary > objects = versions.getVersionSummaries();
        List< String > multiVersionKeys = new ArrayList<>();
        List< String > allKeys = new ArrayList<>();
        String expMd5 = TestTools.getMD5( currContent.getBytes() );
        for ( S3VersionSummary obj : objects ) {
            String key = obj.getKey();
            String actEtag = obj.getETag();
            String versionId = obj.getVersionId();
            allKeys.add( key );
            // 可能写元数据过程中异常返回失败实际创建成功，导致获取对象时不存在再次创建后存在多个版本对象
            if ( versionId.equals( "1" ) ) {
                multiVersionKeys.add( key );
            } else {
                Assert.assertEquals( versionId, "0", "objectName is : " + key );
            }
            Assert.assertEquals( actEtag, expMd5,
                    "objectName is : " + key + ", versionid=" + versionId );
        }
        // 删除历史版本对象后比较所有当前对象个数
        HashSet currentKeys = new HashSet( allKeys );
        int historyVersionObjectNum = allKeys.size() - currentKeys.size();
        Assert.assertEquals( historyVersionObjectNum, multiVersionKeys.size(),
                "multiVersionKeys=" + multiVersionKeys );
        Assert.assertEquals( currentKeys.size(), keyNames.size(),
                "putObjectList : " + putObjectList.toString() + "  ,objects="
                        + printVersionKeys( objects ) + " ,multiVersionKeys="
                        + multiVersionKeys );
    }

    private String printVersionKeys( List< S3VersionSummary > objects ) {
        String str = "";
        for ( S3VersionSummary obj : objects ) {
            str += obj.getKey();
            str += " ";
        }
        return str;
    }
}