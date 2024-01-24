package com.sequoiadb.location.killnode;

import java.util.ArrayList;

import com.sequoiadb.base.*;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.fault.KillNode;
import com.sequoiadb.location.LocationUtils;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.TaskMgr;
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
 * @Description seqDB-31817:Critical模式中节点操作
 * @Author TangTao
 * @Date 2023.05.26
 * @UpdateAuthor TangTao
 * @UpdateDate 2023.05.26
 */
@Test(groups = "location")
public class Critical31817 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private DBCollection dbcl = null;
    private GroupMgr groupMgr;
    private String csName = "cs_31817";
    private String clName = "cl_31817";
    private String primaryLocation = "guangzhou.nansha_31817";
    private String sameCityLocation = "guangzhou.panyu_31817";
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

        BasicBSONObject option = new BasicBSONObject();
        option.put( "Group", expandGroupName );
        dbcl = sdb.getCollectionSpace( csName ).createCollection( clName,
                option );
    }

    @Test
    public void test() throws ReliabilityException, InterruptedException {
        String groupName = SdbTestBase.expandGroupName;
        ReplicaGroup group = sdb.getReplicaGroup( groupName );
        ArrayList< BasicBSONObject > primaryLocationNodes = LocationUtils
                .getGroupLocationNodes( sdb, groupName, primaryLocation );
        ArrayList< BasicBSONObject > sameCityLocationNodes = LocationUtils
                .getGroupLocationNodes( sdb, groupName, sameCityLocation );

        if ( !CommLib.isLSNConsistency( sdb, SdbTestBase.expandGroupName ) ) {
            Assert.fail( "LSN is not consistency" );
        }
        // 1、同城备中心开启Critical模式
        BasicBSONObject options1 = new BasicBSONObject();
        options1.put( "MinKeepTime", 5 );
        options1.put( "MaxKeepTime", 10 );
        options1.put( "Location", sameCityLocation );
        group.startCriticalMode( options1 );
        LocationUtils.checkGroupInCriticalMode( sdb, groupName );

        // 2、停止所有节点
        for ( BasicBSONObject primaryLocationNode : primaryLocationNodes ) {
            String nodeName = primaryLocationNode.getString( "hostName" ) + ":"
                    + primaryLocationNode.getString( "svcName" );
            Node node = group.getNode( nodeName );
            node.stop();
        }
        for ( BasicBSONObject sameCityLocationNode : sameCityLocationNodes ) {
            String nodeName = sameCityLocationNode.getString( "hostName" ) + ":"
                    + sameCityLocationNode.getString( "svcName" );
            Node node = group.getNode( nodeName );
            node.stop();
        }

        // 3、启动所有节点
        group.start();
        if ( LocationUtils.isPrimaryNodeInLocation( sdb, groupName,
                primaryLocationNodes, 30 ) ) {
            // 主节点在主中心，自动退出critical模式，重新启动critical模式
            LocationUtils.checkGroupStopCriticalMode( sdb, groupName );

            group.startCriticalMode( options1 );
            LocationUtils.checkGroupInCriticalMode( sdb, groupName );
        } else {
            // 主节点在备中心保持critical模式
            LocationUtils.checkGroupInCriticalMode( sdb, groupName );
        }

        // 4、强杀所有节点
        TaskMgr mgr = new TaskMgr();
        for ( BasicBSONObject curNode : primaryLocationNodes ) {
            FaultMakeTask faultTask = KillNode.getFaultMakeTask(
                    curNode.getString( "hostName" ),
                    curNode.getString( "svcName" ), 0 );
            mgr.addTask( faultTask );
        }
        for ( BasicBSONObject curNode : sameCityLocationNodes ) {
            FaultMakeTask faultTask = KillNode.getFaultMakeTask(
                    curNode.getString( "hostName" ),
                    curNode.getString( "svcName" ), 0 );
            mgr.addTask( faultTask );
        }
        mgr.execute();
        Assert.assertTrue(
                groupMgr.checkBusiness( 600, true, SdbTestBase.coordUrl ),
                "failed to restore business" );
        if ( !CommLib.isLSNConsistency( sdb, SdbTestBase.expandGroupName ) ) {
            Assert.fail( "LSN is not consistency" );
        }

        if ( LocationUtils.isPrimaryNodeInLocation( sdb, groupName,
                primaryLocationNodes, 30 ) ) {
            // 主节点在主中心，自动退出critical模式，重新启动critical模式
            LocationUtils.checkGroupStopCriticalMode( sdb, groupName );

            group.startCriticalMode( options1 );
            LocationUtils.checkGroupInCriticalMode( sdb, groupName );
        } else {
            // 主节点在备中心保持critical模式
            LocationUtils.checkGroupInCriticalMode( sdb, groupName );
        }

        // 5、停止备中心所有节点
        for ( BasicBSONObject sameCityLocationNode : sameCityLocationNodes ) {
            String nodeName = sameCityLocationNode.getString( "hostName" ) + ":"
                    + sameCityLocationNode.getString( "svcName" );
            Node node = group.getNode( nodeName );
            node.stop();
        }

        LocationUtils.checkPrimaryNodeInLocation( sdb, groupName,
                primaryLocationNodes, 30 );

        // 6、启动备中心所有节点，备中心已自动退出critical模式
        group.start();
        Assert.assertTrue(
                groupMgr.checkBusiness( 600, true, SdbTestBase.coordUrl ),
                "failed to restore business" );
        LocationUtils.checkGroupStopCriticalMode( sdb, groupName );

        // 7、同城备中心开启Critical模式
        group.startCriticalMode( options1 );

        // 8、强杀备中心所有节点，选主后自动退出critical模式
        TaskMgr mgr2 = new TaskMgr();
        for ( BasicBSONObject curNode : sameCityLocationNodes ) {
            FaultMakeTask faultTask = KillNode.getFaultMakeTask(
                    curNode.getString( "hostName" ),
                    curNode.getString( "svcName" ), 0 );
            mgr2.addTask( faultTask );
        }
        mgr2.execute();

        Assert.assertTrue(
                groupMgr.checkBusiness( 600, true, SdbTestBase.coordUrl ),
                "failed to restore business" );
        LocationUtils.checkGroupStopCriticalMode( sdb, groupName );

        // 9、同城备中心开启Critical模式
        if ( !CommLib.isLSNConsistency( sdb, SdbTestBase.expandGroupName ) ) {
            Assert.fail( "LSN is not consistency" );
        }
        group.startCriticalMode( options1 );

        // 集群环境恢复后校验数据
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
