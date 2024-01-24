package com.sequoiadb.location.killnode;

import java.util.ArrayList;
import java.util.List;

import com.sequoiadb.base.*;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.location.LocationUtils;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.commlib.CommLib;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.ReliabilityException;

import static com.sequoiadb.location.LocationUtils.getGroupCriticalInfo;

/**
 * @Descreption seqDB-31768:主中心异常停止，备中心节点启动Critical模式转为Location启动Critical模式
 * @Author huanghaimei
 * @CreateDate 2023/5/31
 * @UpdateUser huanghaimei
 * @UpdateDate 2023/5/31
 * @UpdateRemark
 * @Version
 */
@Test(groups = "location")
public class Critical31768 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private DBCollection dbcl = null;
    private GroupMgr groupMgr;
    private String csName = "cs_31768";
    private String clName = "cl_31768";
    private String primaryLocation = "guangzhou.nansha_31768";
    private String sameCityLocation = "guangzhou.panyu_31768";
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

        // 主中心设置ActiveLocation
        LocationUtils.setTwoLocationInSameCity( sdb, expandGroupName,
                primaryLocation, sameCityLocation );
        sdb.getReplicaGroup( expandGroupName )
                .setActiveLocation( primaryLocation );

        // 检查cl主备节点LSN一致
        CommLib.isLSNConsistency( sdb, SdbTestBase.expandGroupName );

        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }

        sdb.createCollectionSpace( csName );
    }

    @Test
    public void test() throws ReliabilityException {
        String groupName = SdbTestBase.expandGroupName;
        ReplicaGroup group = sdb.getReplicaGroup( groupName );
        // 获取group中指定Location下的所有节点
        ArrayList< BasicBSONObject > primaryLocationNodes = LocationUtils
                .getGroupLocationNodes( sdb, groupName, primaryLocation );
        ArrayList< BasicBSONObject > sameCityLocationNodes = LocationUtils
                .getGroupLocationNodes( sdb, groupName, sameCityLocation );

        // 停止主位置节点
        LocationUtils.stopNodeAbnormal( sdb, groupName, primaryLocationNodes );

        // 同城备中心单节点Critical模式
        String nodeName1 = sameCityLocationNodes.get( 0 )
                .getString( "hostName" ) + ":"
                + sameCityLocationNodes.get( 0 ).getString( "svcName" );
        BasicBSONObject options1 = new BasicBSONObject();
        options1.put( "MinKeepTime", 5 );
        options1.put( "MaxKeepTime", 10 );
        options1.put( "NodeName", nodeName1 );
        group.startCriticalMode( options1 );
        LocationUtils.checkGroupCriticalModeStatus( sdb, groupName,
                sameCityLocationNodes.get( 0 ).getInt( "nodeID" ) );

        // 同城备中心Location启动Critical模式
        BasicBSONObject options2 = new BasicBSONObject();
        options2.put( "MinKeepTime", 5 );
        options2.put( "MaxKeepTime", 10 );
        options2.put( "Location", sameCityLocation );
        group.startCriticalMode( options2 );

        BasicBSONObject info = getGroupCriticalInfo( sdb, groupName );
        String location = info.getString( "Location" );

        Assert.assertEquals( location, sameCityLocation, "location不相等！" );

        // 创建集合、插入数据并校验
        BasicBSONObject option = new BasicBSONObject();
        option.put( "Group", expandGroupName );
        dbcl = sdb.getCollectionSpace( csName ).createCollection( clName,
                option );

        List< BSONObject > batchRecords1 = CommLib.insertData( dbcl,
                recordNum );
        BasicBSONObject orderBy = new BasicBSONObject( "a", 1 );
        CommLib.checkRecords( dbcl, batchRecords1, orderBy );

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
