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
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.KillNode;
import com.sequoiadb.location.LocationUtils;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @Description seqDB-33466:异地备中心异常，启动运维模式
 * @Author liuli
 * @Date 2023.09.22
 * @UpdateAuthor liuli
 * @UpdateDate 2023.09.22
 * @version 1.10
 */
@Test(groups = "location")
public class Location33466 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private GroupMgr groupMgr;
    private CollectionSpace dbcs = null;
    private String csName = "cs_33466";
    private String clName = "cl_33466";
    private String primaryLocation = "guangzhou.nansha_33466";
    private String sameCityLocation = "guangzhou.panyu_33466";
    private String offsiteLocation = "shenzhan.nanshan_33466";
    private String expandGroupName;
    private int recordNum = 1000;

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

        expandGroupName = SdbTestBase.expandGroupNames.get( 0 );
        LocationUtils.setTwoCityAndThreeLocation( sdb, expandGroupName,
                primaryLocation, sameCityLocation, offsiteLocation );
        if ( !CommLib.isLSNConsistency( sdb, expandGroupName ) ) {
            Assert.fail( "LSN is not consistency" );
        }

        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }

        dbcs = sdb.createCollectionSpace( csName );
    }

    @Test
    public void test() throws ReliabilityException {
        // 异地备中心节点异常
        ReplicaGroup group = sdb.getReplicaGroup( expandGroupName );
        ArrayList< BasicBSONObject > offsiteLocationNodes = LocationUtils
                .getGroupLocationNodes( sdb, expandGroupName, offsiteLocation );

        TaskMgr mgr = new TaskMgr();
        for ( BasicBSONObject offsiteLocationNode : offsiteLocationNodes ) {
            FaultMakeTask faultTask = KillNode.getFaultMakeTask(
                    offsiteLocationNode.getString( "hostName" ),
                    offsiteLocationNode.getString( "svcName" ), 0 );
            mgr.addTask( faultTask );
        }

        mgr.execute();
        Assert.assertTrue( mgr.isAllSuccess(), mgr.getErrorMsg() );

        // 等待节点启动后再正常停止节点，模拟节点停止后不会恢复
        int timeout = 30;
        for ( BasicBSONObject offsiteLocationNode : offsiteLocationNodes ) {
            String nodeName = offsiteLocationNode.getString( "hostName" ) + ":"
                    + offsiteLocationNode.getString( "svcName" );
            LocationUtils.waitNodeStart( sdb, nodeName, timeout );
            Node node = group.getNode( nodeName );
            node.stop();
        }

        // 指定同城备中启动运维模式
        BasicBSONObject options = new BasicBSONObject();
        options.put( "MinKeepTime", 10 );
        options.put( "MaxKeepTime", 20 );
        options.put( "Location", offsiteLocation );
        group.startMaintenanceMode( options );

        // 创建集合并插入数据
        BasicBSONObject option = new BasicBSONObject();
        option.put( "Group", expandGroupName );
        option.put( "ReplSize", 0 );
        DBCollection dbcl = dbcs.createCollection( clName, option );

        // 集合插入数据
        List< BSONObject > batchRecords = CommLib.insertData( dbcl, recordNum );

        BasicBSONObject orderBy = new BasicBSONObject( "a", 1 );
        CommLib.checkRecords( dbcl, batchRecords, orderBy );

        // 解除运维模式，节点未恢复
        group.stopMaintenanceMode();
        group.start();
        Assert.assertTrue(
                groupMgr.checkBusiness( 600, true, SdbTestBase.coordUrl ),
                "failed to restore business" );
    }

    @AfterClass
    public void tearDown() throws ReliabilityException {
        sdb.getReplicaGroup( expandGroupName ).start();
        sdb.getReplicaGroup( expandGroupName ).stopMaintenanceMode();
        Assert.assertTrue(
                groupMgr.checkBusiness( 600, true, SdbTestBase.coordUrl ),
                "failed to restore business" );
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        if ( sdb != null ) {
            sdb.close();
        }
    }
}
