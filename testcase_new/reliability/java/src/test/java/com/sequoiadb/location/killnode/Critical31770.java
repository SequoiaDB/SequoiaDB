package com.sequoiadb.location.killnode;

import java.util.ArrayList;

import com.sequoiadb.base.*;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.location.LocationUtils;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.commlib.CommLib;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.ReliabilityException;

/**
 * @version 1.0
 * @Description seqDB-31770:主中心异常停止，备中心启动Critical模式后备中心Location增加节点
 * @Author TangTao
 * @Date 2023.05.26
 * @UpdateAuthor TangTao
 * @UpdateDate 2023.05.26
 */
@Test(groups = "location")
public class Critical31770 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private DBCollection dbcl1 = null;
    private DBCollection dbcl2 = null;
    private GroupMgr groupMgr;
    private String csName = "cs_31770";
    private String clName1 = "cl_31770_1";
    private String clName2 = "cl_31770_2";
    private String primaryLocation = "guangzhou.nansha_31770";
    private String sameCityLocation = "guangzhou.panyu_31770";
    private int recordNum = 10000;

    @BeforeClass
    public void setUp() throws ReliabilityException {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        groupMgr = GroupMgr.getInstance();
        if ( !groupMgr.checkBusiness( 120, true, SdbTestBase.coordUrl ) ) {
            throw new SkipException( "checkBusiness return false" );
        }
        LocationUtils.setTwoLocationInSameCity( sdb, expandGroupName,
                primaryLocation, sameCityLocation );
        sdb.getReplicaGroup( expandGroupName )
                .setActiveLocation( primaryLocation );

        if ( !CommLib.isLSNConsistency( sdb, SdbTestBase.expandGroupName ) ) {
            Assert.fail( "LSN is not consistency" );
        }

        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }

        CollectionSpace dbcs = sdb.createCollectionSpace( csName );

        BasicBSONObject option1 = new BasicBSONObject();
        option1.put( "ReplSize", -1 );
        option1.put( "Group", expandGroupName );
        dbcl1 = dbcs.createCollection( clName1, option1 );
    }

    @Test
    public void test() throws ReliabilityException, InterruptedException {
        String groupName = SdbTestBase.expandGroupName;
        ReplicaGroup group = sdb.getReplicaGroup( groupName );
        ArrayList< BasicBSONObject > primaryLocationNodes = LocationUtils
                .getGroupLocationNodes( sdb, groupName, primaryLocation );
        ArrayList< BasicBSONObject > primaryLocationSlaveNodes = LocationUtils
                .getGroupLocationSlaveNodes( sdb, groupName, primaryLocation );
        ArrayList< BasicBSONObject > sameCityLocationNodes = LocationUtils
                .getGroupLocationNodes( sdb, groupName, sameCityLocation );

        // 主中心异常停止，然后stop节点模拟故障无法启动
        LocationUtils.stopNodeAbnormal( sdb, groupName, primaryLocationNodes );

        // 同城备中心启动Critical模式
        BasicBSONObject options1 = new BasicBSONObject();
        options1.put( "MinKeepTime", 10 );
        options1.put( "MaxKeepTime", 20 );
        options1.put( "Location", sameCityLocation );
        group.startCriticalMode( options1 );
        LocationUtils.checkGroupInCriticalMode( sdb, groupName );

        // 部分节点设置location为critical模式的location
        for ( BasicBSONObject curNode : primaryLocationSlaveNodes ) {
            String nodeName = curNode.getString( "hostName" ) + ":"
                    + curNode.getString( "svcName" );
            Node node = group.getNode( nodeName );
            node.setLocation( sameCityLocation );
        }

        // 等待复制组状态更新，sameCityLocation内6节点3个异常，不满足多数派无法选主
        try {
            int timeout = 30;
            while ( timeout-- > 0 ) {
                group.getMaster();
                Thread.sleep( 1000 );
            }
            Assert.fail( "Expected to failed, group do not had primary node." );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_RTN_NO_PRIMARY_FOUND
                    .getErrorCode() )
                throw ( e );
        }

        BasicBSONObject groupInfo = ( BasicBSONObject ) group.getDetail();
        Assert.assertFalse( groupInfo.containsField( "PrimaryNode" ) );

        // 创建集合插入数据，期望失败
        try {
            BasicBSONObject option2 = new BasicBSONObject();
            option2.put( "ReplSize", -1 );
            option2.put( "Group", groupName );
            dbcl2 = sdb.getCollectionSpace( csName ).createCollection( clName2,
                    option2 );
            CommLib.insertData( dbcl2, recordNum );

            Assert.fail( "Expected to failed, group do not had primary node." );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_CLS_NOT_PRIMARY
                    .getErrorCode() )
                throw ( e );
        }

        // 集群环境恢复
        group.start();
        group.stopCriticalMode();
        Assert.assertTrue(
                groupMgr.checkBusiness( 600, true, SdbTestBase.coordUrl ),
                "failed to restore business" );
    }

    @AfterClass
    public void tearDown() throws ReliabilityException {
        sdb.getReplicaGroup( SdbTestBase.expandGroupNames.get( 0 ) ).start();
        Assert.assertTrue(
                groupMgr.checkBusiness( 600, true, SdbTestBase.coordUrl ),
                "failed to restore business" );
        LocationUtils.cleanLocation( sdb,
                SdbTestBase.expandGroupNames.get( 0 ) );
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        if ( sdb != null ) {
            sdb.close();
        }
    }
}
