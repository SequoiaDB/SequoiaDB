package com.sequoiadb.location.killnode;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.*;
import com.sequoiadb.commlib.CommLib;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.fault.KillNode;
import com.sequoiadb.location.LocationUtils;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @Description seqDB-31799:灾备中心故障，主中心启动Critical模式后节点故障
 * @Author liuli
 * @Date 2023.05.30
 * @UpdateAuthor liuli
 * @UpdateDate 2023.05.30
 * @version 1.10
 */
@Test(groups = "location")
public class Critical31799 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private DBCollection dbcl = null;
    private GroupMgr groupMgr;
    private String csName = "cs_31799";
    private String clName = "cl_31799";
    private String primaryLocation = "guangzhou.nansha_31799";
    private String sameCityLocation = "guangzhou.panyu_31799";
    private String offsiteLocation = "shenzhan.nanshan_31799";
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
        LocationUtils.setTwoCityAndThreeLocation( sdb, expandGroupName,
                primaryLocation, sameCityLocation, offsiteLocation );
        sdb.getReplicaGroup( groupName ).setActiveLocation( primaryLocation );

        CommLib.isLSNConsistency( sdb, groupName );

        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        CollectionSpace dbcs = sdb.createCollectionSpace( csName );

        BasicBSONObject option = new BasicBSONObject();
        option.put( "Group", groupName );
        dbcl = dbcs.createCollection( clName, option );
    }

    @Test
    public void test() throws ReliabilityException {
        ReplicaGroup group = sdb.getReplicaGroup( groupName );
        ArrayList< BasicBSONObject > slaveLocationNodes = new ArrayList<>();
        ArrayList< BasicBSONObject > offsiteLocationNodes = LocationUtils
                .getGroupLocationNodes( sdb, groupName, offsiteLocation );
        ArrayList< BasicBSONObject > primaryLocationNodes = LocationUtils
                .getGroupLocationNodes( sdb, groupName, primaryLocation );
        ArrayList< BasicBSONObject > sameslaveLocationNodes = LocationUtils
                .getGroupLocationNodes( sdb, groupName, sameCityLocation );
        slaveLocationNodes.addAll( offsiteLocationNodes );
        slaveLocationNodes.addAll( sameslaveLocationNodes );

        // 灾备中心节点全部异常停止
        TaskMgr mgr = new TaskMgr();
        for ( BasicBSONObject cityLocationNode : slaveLocationNodes ) {
            FaultMakeTask faultTask = KillNode.getFaultMakeTask(
                    cityLocationNode.getString( "hostName" ),
                    cityLocationNode.getString( "svcName" ), 0 );
            mgr.addTask( faultTask );
        }

        mgr.execute();
        Assert.assertTrue( mgr.isAllSuccess(), mgr.getErrorMsg() );

        // 等待节点启动后再正常停止节点，模拟节点停止后不会恢复
        int timeout = 30;
        for ( BasicBSONObject cityLocationNode : slaveLocationNodes ) {
            String nodeName = cityLocationNode.getString( "hostName" ) + ":"
                    + cityLocationNode.getString( "svcName" );
            LocationUtils.waitNodeStart( sdb, nodeName, timeout );
            Node node = group.getNode( nodeName );
            node.stop();
        }

        // 主中心启动Critical模式
        BasicBSONObject options = new BasicBSONObject();
        options.put( "MinKeepTime", 20 );
        options.put( "MaxKeepTime", 40 );
        options.put( "Location", primaryLocation );
        group.startCriticalMode( options );

        // 主中心一个备节点停止
        ArrayList< BasicBSONObject > primaryLocationSlaveNodes = LocationUtils
                .getGroupLocationSlaveNodes( sdb, groupName, primaryLocation );
        String primaryLocationSlaveNodeName = primaryLocationSlaveNodes.get( 0 )
                .getString( "hostName" ) + ":"
                + primaryLocationSlaveNodes.get( 0 ).getString( "svcName" );
        mgr.clear();
        FaultMakeTask faultTask1 = KillNode.getFaultMakeTask(
                primaryLocationSlaveNodes.get( 0 ).getString( "hostName" ),
                primaryLocationSlaveNodes.get( 0 ).getString( "svcName" ), 0 );
        mgr.addTask( faultTask1 );
        mgr.execute();
        Assert.assertTrue( mgr.isAllSuccess(), mgr.getErrorMsg() );
        LocationUtils.waitNodeStart( sdb, primaryLocationSlaveNodeName,
                timeout );
        Node primaryLocationSlaveNode = group
                .getNode( primaryLocationSlaveNodeName );
        primaryLocationSlaveNode.stop();

        // 插入数据并校验
        List< BSONObject > batchRecords1 = CommLib.insertData( dbcl,
                recordNum );
        BasicBSONObject orderBy = new BasicBSONObject( "a", 1 );
        CommLib.checkRecords( dbcl, batchRecords1, orderBy );

        // 异常节点恢复正常
        primaryLocationSlaveNode.start();

        // 主节点异常停止
        Node primaryLocationMasterNode = group.getMaster();
        String primaryLocationMasterNodeName = primaryLocationMasterNode
                .getNodeName();
        mgr.clear();
        FaultMakeTask faultTask2 = KillNode.getFaultMakeTask(
                primaryLocationMasterNode.getHostName(),
                String.valueOf( primaryLocationMasterNode.getPort() ), 0 );
        mgr.addTask( faultTask2 );
        mgr.execute();
        Assert.assertTrue( mgr.isAllSuccess(), mgr.getErrorMsg() );
        LocationUtils.waitNodeStart( sdb, primaryLocationMasterNodeName,
                timeout );
        primaryLocationMasterNode.stop();

        // 等待选出主节点
        waitElect( sdb, groupName, primaryLocationMasterNode.getNodeName(),
                timeout );
        LocationUtils.isLSNConsistencyNormalNode( sdb, groupName );

        // 插入数据并校验
        dbcl.truncate();
        List< BSONObject > batchRecords2 = CommLib.insertData( dbcl,
                recordNum );
        CommLib.checkRecords( dbcl, batchRecords2, orderBy );

        // 主中心节点全部异常停止
        primaryLocationMasterNode.start();

        mgr.clear();
        for ( BasicBSONObject primaryLocationNode : primaryLocationNodes ) {
            FaultMakeTask faultTask = KillNode.getFaultMakeTask(
                    primaryLocationNode.getString( "hostName" ),
                    primaryLocationNode.getString( "svcName" ), 0 );
            mgr.addTask( faultTask );
        }

        mgr.execute();
        Assert.assertTrue( mgr.isAllSuccess(), mgr.getErrorMsg() );

        // 等待选出主节点
        waitElect( sdb, groupName, primaryLocationMasterNode.getNodeName(),
                timeout );
        LocationUtils.isLSNConsistencyNormalNode( sdb, groupName );

        // group保持Critical模式
        LocationUtils.checkGroupStartCriticalMode( sdb, groupName );

        // 等待集群恢复
        group.start();
        Assert.assertTrue(
                groupMgr.checkBusiness( 600, true, SdbTestBase.coordUrl ),
                "failed to restore business" );

        // 停止Critical模式
        group.stopCriticalMode();
        Assert.assertTrue(
                groupMgr.checkBusiness( 600, true, SdbTestBase.coordUrl ),
                "failed to restore business" );
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

    // 等待选出主节点
    private void waitElect( Sequoiadb db, String groupName,
            String originalMasterNodeName, int timeOut ) {
        boolean existPrimaryNode = false;
        ReplicaGroup group = db.getReplicaGroup( groupName );
        int doTime = 0;
        while ( doTime < timeOut ) {
            try {
                Node masterNode = group.getMaster();
                String masterNodeName = masterNode.getNodeName();
                if ( !masterNodeName.equals( originalMasterNodeName ) ) {
                    existPrimaryNode = true;
                }
            } catch ( BaseException e ) {
                System.out.println( "e -- " + e );
                if ( e.getErrorCode() != SDBError.SDB_RTN_NO_PRIMARY_FOUND
                        .getErrorCode() ) {
                    throw e;
                }
            }

            if ( existPrimaryNode ) {
                break;
            }
            try {
                Thread.sleep( 1000 );
            } catch ( InterruptedException e ) {
                throw new RuntimeException( e );
            }
            doTime++;
        }

        if ( doTime >= timeOut ) {
            Assert.fail( "there is no primary node in group, groupName : "
                    + groupName );
        }
    }
}
