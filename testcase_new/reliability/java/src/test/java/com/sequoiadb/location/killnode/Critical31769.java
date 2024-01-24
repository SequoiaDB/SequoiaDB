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
 * @Description seqDB-31769:主中心异常停止，备中心启动Critical模式后再次出现节点故障
 * @Author liuli
 * @Date 2023.05.26
 * @UpdateAuthor liuli
 * @UpdateDate 2023.05.26
 * @version 1.10
 */
@Test(groups = "location")
public class Critical31769 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private DBCollection dbcl = null;
    private GroupMgr groupMgr;
    private String csName = "cs_31769";
    private String clName = "cl_31769";
    private String primaryLocation = "guangzhou.nansha_31769";
    private String sameCityLocation = "guangzhou.panyu_31769";
    private int recordNum = 10000;
    private String groupName = null;

    @BeforeClass
    public void setUp() throws ReliabilityException {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        groupName = SdbTestBase.expandGroupNames.get( 0 );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        groupMgr = GroupMgr.getInstance();
        if ( !groupMgr.checkBusiness( 120, true, SdbTestBase.coordUrl ) ) {
            throw new SkipException( "checkBusiness return false" );
        }
        LocationUtils.setTwoLocationInSameCity( sdb, groupName, primaryLocation,
                sameCityLocation );
        sdb.getReplicaGroup( groupName ).setActiveLocation( primaryLocation );

        CommLib.isLSNConsistency( sdb, groupName );

        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        sdb.createCollectionSpace( csName );

    }

    @Test
    public void test() throws ReliabilityException {
        ReplicaGroup group = sdb.getReplicaGroup( groupName );
        ArrayList< BasicBSONObject > primaryLocationNodes = LocationUtils
                .getGroupLocationNodes( sdb, groupName, primaryLocation );
        ArrayList< BasicBSONObject > sameCityLocationNodes = LocationUtils
                .getGroupLocationNodes( sdb, groupName, sameCityLocation );

        // 停止主位置节点
        TaskMgr mgr = new TaskMgr();
        for ( BasicBSONObject primaryLocationNode : primaryLocationNodes ) {
            FaultMakeTask faultTask = KillNode.getFaultMakeTask(
                    primaryLocationNode.getString( "hostName" ),
                    primaryLocationNode.getString( "svcName" ), 0 );
            mgr.addTask( faultTask );
        }

        mgr.execute();
        Assert.assertTrue( mgr.isAllSuccess(), mgr.getErrorMsg() );

        // 等待节点启动后再正常停止节点，模拟节点停止后不会恢复
        int timeout = 30;
        for ( BasicBSONObject primaryLocationNode : primaryLocationNodes ) {
            String nodeName = primaryLocationNode.getString( "hostName" ) + ":"
                    + primaryLocationNode.getString( "svcName" );
            LocationUtils.waitNodeStart( sdb, nodeName, timeout );
            Node node = group.getNode( nodeName );
            node.stop();
        }

        // 同城备中心Location启动Critical模式
        BasicBSONObject options1 = new BasicBSONObject();
        options1.put( "MinKeepTime", 5 );
        options1.put( "MaxKeepTime", 10 );
        options1.put( "Location", sameCityLocation );
        group.startCriticalMode( options1 );

        // 创建集合插入数据并校验
        BasicBSONObject option = new BasicBSONObject();
        option.put( "Group", groupName );
        dbcl = sdb.getCollectionSpace( csName ).createCollection( clName,
                option );

        List< BSONObject > batchRecords1 = CommLib.insertData( dbcl,
                recordNum );
        BasicBSONObject orderBy = new BasicBSONObject( "a", 1 );
        CommLib.checkRecords( dbcl, batchRecords1, orderBy );

        // 备中心出现故障，只剩余一个节点正常
        int sameCityNodeNum = sameCityLocationNodes.size();
        mgr.clear();
        for ( int i = 0; i < sameCityNodeNum - 1; i++ ) {
            FaultMakeTask faultTask = KillNode.getFaultMakeTask(
                    sameCityLocationNodes.get( i ).getString( "hostName" ),
                    sameCityLocationNodes.get( i ).getString( "svcName" ), 0 );
            mgr.addTask( faultTask );
        }
        mgr.execute();
        Assert.assertTrue( mgr.isAllSuccess(), mgr.getErrorMsg() );

        for ( int i = 0; i < sameCityNodeNum - 1; i++ ) {
            String nodeName = sameCityLocationNodes.get( i )
                    .getString( "hostName" ) + ":"
                    + sameCityLocationNodes.get( i ).getString( "svcName" );
            LocationUtils.waitNodeStart( sdb, nodeName, timeout );
            Node node = group.getNode( nodeName );
            node.stop();
        }

        // 剩余一个节点启动Critical模式
        String nodeName2 = sameCityLocationNodes.get( sameCityNodeNum - 1 )
                .getString( "hostName" ) + ":"
                + sameCityLocationNodes.get( sameCityNodeNum - 1 )
                        .getString( "svcName" );
        BasicBSONObject options2 = new BasicBSONObject();
        options2.put( "MinKeepTime", 5 );
        options2.put( "MaxKeepTime", 10 );
        options2.put( "NodeName", nodeName2 );
        group.startCriticalMode( options2 );

        // 集合插入数据并校验
        dbcl.truncate();
        List< BSONObject > batchRecords2 = CommLib.insertData( dbcl,
                recordNum );
        CommLib.checkRecords( dbcl, batchRecords2, orderBy );

        // 集群环境恢复后校验数据
        group.start();
        group.stopCriticalMode();
        Assert.assertTrue(
                groupMgr.checkBusiness( 600, true, SdbTestBase.coordUrl ),
                "failed to restore business" );

        // 集合插入数据并校验
        dbcl.truncate();
        List< BSONObject > batchRecords3 = CommLib.insertData( dbcl,
                recordNum );
        CommLib.checkRecords( dbcl, batchRecords3, orderBy );

        // group不指定节点重新选主
        group.reelect();

        // 校验主节点在ActiveLocation中
        Node masterNode = group.getMaster();
        String masterNodeName = masterNode.getHostName() + ":"
                + masterNode.getPort();

        ArrayList< String > activeLocationNodes = new ArrayList<>();
        for ( BasicBSONObject primaryLocationNode : primaryLocationNodes ) {
            activeLocationNodes.add( primaryLocationNode.getString( "hostName" )
                    + ":" + primaryLocationNode.getString( "svcName" ) );
        }
        if ( !activeLocationNodes.contains( masterNodeName ) ) {
            Assert.fail( "masterNode is not in activeLocation , masterNode : "
                    + masterNodeName + " activeLocationNodes : "
                    + activeLocationNodes );
        }
        System.out.println( "masterNode : " + masterNodeName
                + " activeLocationNodes : " + activeLocationNodes );

    }

    @AfterClass
    public void tearDown() throws ReliabilityException {
        sdb.getReplicaGroup( expandGroupNames.get( 0 ) ).start();
        Assert.assertTrue(
                groupMgr.checkBusiness( 600, true, SdbTestBase.coordUrl ),
                "failed to restore business" );
        LocationUtils.cleanLocation( sdb, expandGroupNames.get( 0 ) );
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        if ( sdb != null ) {
            sdb.close();
        }
    }

}
