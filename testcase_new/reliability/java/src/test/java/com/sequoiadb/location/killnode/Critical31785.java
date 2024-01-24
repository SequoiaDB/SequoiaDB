package com.sequoiadb.location.killnode;

import java.util.ArrayList;
import java.util.List;

import com.sequoiadb.base.*;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.fault.KillNode;
import com.sequoiadb.location.LocationUtils;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.TaskMgr;
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

/**
 * @version 1.0
 * @Description seqDB-31785:主中心异常，指定Enforced为true强制切主
 * @Author TangTao
 * @Date 2023.05.26
 * @UpdateAuthor TangTao
 * @UpdateDate 2023.05.26
 */
@Test(groups = "location")
public class Critical31785 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private DBCollection dbcl1 = null;
    private DBCollection dbcl2 = null;
    private DBCollection dbcl3 = null;
    private GroupMgr groupMgr;
    private String csName = "cs_31785";
    private String clName1 = "cl_31785_1";
    private String clName2 = "cl_31785_2";
    private String clName3 = "cl_31785_3";
    private String primaryLocation = "guangzhou.nansha_31785";
    private String sameCityLocation = "guangzhou.panyu_31785";
    private int recordNum = 100000;

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
        ArrayList< BasicBSONObject > sameCityLocationNodes = LocationUtils
                .getGroupLocationNodes( sdb, groupName, sameCityLocation );

        // 停止备中心所有节点
        for ( BasicBSONObject sameCityLocationNode : sameCityLocationNodes ) {
            String nodeName = sameCityLocationNode.getString( "hostName" ) + ":"
                    + sameCityLocationNode.getString( "svcName" );
            Node node = group.getNode( nodeName );
            node.stop();
        }
        CommLib.insertData( dbcl1, recordNum * 2 );

        group.start();

        // 主中心异常停止，然后stop节点模拟故障无法启动
        LocationUtils.stopNodeAbnormal( sdb, groupName, primaryLocationNodes );

        // 同城备中心启动Critical模式
        BasicBSONObject options1 = new BasicBSONObject();
        options1.put( "MinKeepTime", 1 );
        options1.put( "MaxKeepTime", 10 );
        options1.put( "Location", sameCityLocation );
        options1.put( "Enforced", true );
        group.startCriticalMode( options1 );

        LocationUtils.checkPrimaryNodeInLocation( sdb, groupName,
                sameCityLocationNodes, 30 );

        // 创建集合并插入数据
        BasicBSONObject option2 = new BasicBSONObject();
        option2.put( "ReplSize", -1 );
        option2.put( "Group", expandGroupName );
        dbcl2 = sdb.getCollectionSpace( csName ).createCollection( clName2,
                option2 );

        List< BSONObject > batchRecords2 = CommLib.insertData( dbcl2, 1000 );
        BasicBSONObject orderBy = new BasicBSONObject( "a", 1 );
        CommLib.checkRecords( dbcl2, batchRecords2, orderBy );

        // 集群环境恢复
        group.start();
        group.stopCriticalMode();
        Assert.assertTrue(
                groupMgr.checkBusiness( 600, true, SdbTestBase.coordUrl ),
                "failed to restore business" );

        // 创建集合并插入数据
        BasicBSONObject option3 = new BasicBSONObject();
        option3.put( "ReplSize", -1 );
        option3.put( "Group", expandGroupName );
        dbcl3 = sdb.getCollectionSpace( csName ).createCollection( clName3,
                option3 );

        List< BSONObject > batchRecords3 = CommLib.insertData( dbcl3, 1000 );
        CommLib.checkRecords( dbcl3, batchRecords3, orderBy );
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
