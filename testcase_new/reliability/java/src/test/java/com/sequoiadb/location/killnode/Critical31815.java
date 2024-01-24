package com.sequoiadb.location.killnode;

import java.util.ArrayList;

import com.sequoiadb.base.*;
import com.sequoiadb.commlib.GroupMgr;
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
 * @Description seqDB-31815:集群正常data启动Critical模式
 * @Author TangTao
 * @Date 2023.06.05
 * @UpdateAuthor TangTao
 * @UpdateDate 2023.06.05
 */
@Test(groups = "location")
public class Critical31815 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private DBCollection dbcl = null;
    private GroupMgr groupMgr;
    private String csName = "cs_31815";
    private String clName = "cl_31815";
    private String primaryLocation = "guangzhou.nansha_31815";
    private String sameCityLocation = "guangzhou.panyu_31815";
    private String offsiteLocation = "shenzhen.nanshan_31815";
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
        LocationUtils.setTwoCityAndThreeLocation( sdb, expandGroupName,
                primaryLocation, sameCityLocation, offsiteLocation );
        sdb.getReplicaGroup( expandGroupName )
                .setActiveLocation( primaryLocation );

        if ( !CommLib.isLSNConsistency( sdb, SdbTestBase.expandGroupName ) ) {
            Assert.fail( "LSN is not consistency" );
        }

        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }

        CollectionSpace dbcs = sdb.createCollectionSpace( csName );
    }

    @Test
    public void test() throws ReliabilityException, InterruptedException {
        String groupName = SdbTestBase.expandGroupName;
        ReplicaGroup group = sdb.getReplicaGroup( groupName );
        ArrayList< BasicBSONObject > sameCityLocationNodes = LocationUtils
                .getGroupLocationNodes( sdb, groupName, sameCityLocation );
        ArrayList< BasicBSONObject > otherLocationNodes = LocationUtils
                .getGroupLocationNodes( sdb, groupName, offsiteLocation );

        // data主节点开启critical模式
        Node primaryNode = group.getMaster();
        String masterNodeName = primaryNode.getNodeName();
        group.startCriticalMode(
                new BasicBSONObject( "NodeName", masterNodeName )
                        .append( "MinKeepTime", 1 )
                        .append( "MaxKeepTime", 5 ) );
        BasicBSONObject criticalInfo = LocationUtils.getGroupCriticalInfo( sdb,
                groupName );
        Assert.assertEquals( criticalInfo.getInt( "NodeID" ),
                primaryNode.getNodeId(), "criticalInfo is not equal" );
        group.stopCriticalMode();

        // data备节点开启critical模式
        String slaveNodeName = sameCityLocationNodes.get( 0 )
                .getString( "hostName" ) + ":"
                + sameCityLocationNodes.get( 0 ).getString( "svcName" );
        group.startCriticalMode(
                new BasicBSONObject( "NodeName", slaveNodeName )
                        .append( "MinKeepTime", 1 )
                        .append( "MaxKeepTime", 5 ) );

        criticalInfo = LocationUtils.getGroupCriticalInfo( sdb, groupName );
        Assert.assertEquals( criticalInfo.getInt( "NodeID" ),
                sameCityLocationNodes.get( 0 ).getInt( "nodeID" ),
                "criticalInfo is not equal" );
        Assert.assertEquals( group.getMaster().getNodeName(), slaveNodeName,
                "master node is not the same" );
        group.stopCriticalMode();

        // location开启critical模式
        group.startCriticalMode(
                new BasicBSONObject( "Location", offsiteLocation )
                        .append( "MinKeepTime", 1 )
                        .append( "MaxKeepTime", 5 ) );
        criticalInfo = LocationUtils.getGroupCriticalInfo( sdb, groupName );
        Assert.assertEquals( criticalInfo.getString( "Location" ),
                offsiteLocation, "criticalInfo is not equal" );
        group.stopCriticalMode();

        // data主节点所在location开启critical模式
        group.reelect( new BasicBSONObject( "NodeID",
                sameCityLocationNodes.get( 0 ).getInt( "nodeID" ) ) );
        group.startCriticalMode(
                new BasicBSONObject( "Location", sameCityLocation )
                        .append( "MinKeepTime", 1 )
                        .append( "MaxKeepTime", 5 ) );
        criticalInfo = LocationUtils.getGroupCriticalInfo( sdb, groupName );
        Assert.assertEquals( criticalInfo.getString( "Location" ),
                sameCityLocation, "criticalInfo is not equal" );
        group.stopCriticalMode();

        // data备节点location开启critical模式
        group.startCriticalMode(
                new BasicBSONObject( "Location", offsiteLocation )
                        .append( "MinKeepTime", 1 )
                        .append( "MaxKeepTime", 5 ) );
        criticalInfo = LocationUtils.getGroupCriticalInfo( sdb, groupName );
        Assert.assertEquals( criticalInfo.getString( "Location" ),
                offsiteLocation, "criticalInfo is not equal" );

        group.startCriticalMode(
                new BasicBSONObject( "Location", primaryLocation )
                        .append( "MinKeepTime", 1 )
                        .append( "MaxKeepTime", 5 ) );
        criticalInfo = LocationUtils.getGroupCriticalInfo( sdb, groupName );
        Assert.assertEquals( criticalInfo.getString( "Location" ),
                primaryLocation, "criticalInfo is not equal" );

        // 集群环境恢复
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
