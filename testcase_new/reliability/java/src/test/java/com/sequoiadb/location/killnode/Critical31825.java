package com.sequoiadb.location.killnode;

import java.util.ArrayList;

import com.sequoiadb.base.*;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.location.LocationUtils;
import com.sequoiadb.task.OperateTask;
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
 * @Description seqDB-31825:并发启动不同的节点为Critical模式
 * @Author TangTao
 * @Date 2023.05.29
 * @UpdateAuthor TangTao
 * @UpdateDate 2023.05.29
 */
@Test(groups = "location")
public class Critical31825 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private DBCollection dbcl = null;
    private GroupMgr groupMgr;
    private String csName = "cs_31825";
    private String clName = "cl_31825";
    private String primaryLocation = "guangzhou.nansha_31825";
    private String sameCityLocation = "guangzhou.panyu_31825";
    private String offsiteLocation = "shenzhen.nanshan_31825";
    private int successCount = 0;

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

        BasicBSONObject option1 = new BasicBSONObject();
        option1.put( "ReplSize", 0 );
        option1.put( "ConsistencyStrategy", 3 );
        option1.put( "Group", expandGroupName );
        dbcl = dbcs.createCollection( clName, option1 );
    }

    @Test
    public void test() throws ReliabilityException, InterruptedException {
        String groupName = SdbTestBase.expandGroupName;
        ReplicaGroup group = sdb.getReplicaGroup( groupName );
        ArrayList< BasicBSONObject > primaryLocationNodes = LocationUtils
                .getGroupLocationSlaveNodes( sdb, groupName, primaryLocation );
        ArrayList< BasicBSONObject > sameCityLocationNodes = LocationUtils
                .getGroupLocationNodes( sdb, groupName, sameCityLocation );
        ArrayList< BasicBSONObject > otherLocationNodes = LocationUtils
                .getGroupLocationNodes( sdb, groupName, offsiteLocation );

        // 并发设置不同节点为critical模式
        TaskMgr mgr = new TaskMgr();

        for ( BasicBSONObject sameCityLocationNode : sameCityLocationNodes ) {
            String nodeName = sameCityLocationNode.getString( "hostName" ) + ":"
                    + sameCityLocationNode.getString( "svcName" );
            mgr.addTask( new setCritical( groupName, nodeName ) );
        }
        for ( BasicBSONObject otherLocationNode : otherLocationNodes ) {
            String nodeName = otherLocationNode.getString( "hostName" ) + ":"
                    + otherLocationNode.getString( "svcName" );
            mgr.addTask( new setCritical( groupName, nodeName ) );
        }
        mgr.execute();
        Assert.assertTrue( successCount <= 4, "successCount:" + successCount );

        // 串行设置不同节点为critical模式
        String pnodeName = primaryLocationNodes.get( 0 ).getString( "hostName" )
                + ":" + primaryLocationNodes.get( 0 ).getString( "svcName" );
        group.startCriticalMode( new BasicBSONObject( "NodeName", pnodeName )
                .append( "MinKeepTime", 5 ).append( "MaxKeepTime", 10 ) );

        LocationUtils.checkGroupInCriticalMode( sdb, groupName );

        // 集群环境恢复
        group.stopCriticalMode();
        group.start();
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

    private class setCritical extends OperateTask {
        String groupName;
        String nodeName;

        private setCritical( String groupName, String nodeName ) {
            this.groupName = groupName;
            this.nodeName = nodeName;
        }

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                ReplicaGroup group = db.getReplicaGroup( groupName );
                group.startCriticalMode(
                        new BasicBSONObject( "NodeName", nodeName )
                                .append( "MinKeepTime", 5 )
                                .append( "MaxKeepTime", 10 ) );
                successCount++;
            }
        }
    }
}
