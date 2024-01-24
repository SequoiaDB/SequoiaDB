package com.sequoiadb.location.killnode;

import java.util.ArrayList;
import java.util.List;

import com.sequoiadb.fault.KillNode;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;
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
import com.sequoiadb.location.LocationUtils;

/**
 * @Description seqDB-33728:节点优先，异地节点不参与同步一致性计算
 *              seqDB-33729:位置多数派优先，异地节点不参与同步一致性计算
 *              seqDB-33730:主位置多数派优先，异地节点不参与同步一致性计算
 * @Author liuli
 * @Date 2023.10.12
 * @UpdateAuthor liuli
 * @UpdateDate 2023.10.12
 * @version 1.10
 */
@Test(groups = "location")
public class Location33728_33729_33730 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private GroupMgr groupMgr;
    private DBCollection dbcl1 = null;
    private DBCollection dbcl2 = null;
    private DBCollection dbcl3 = null;
    private String csName = "cs_33728";
    private String clName1 = "cl_33728_1";
    private String clName2 = "cl_33728_2";
    private String clName3 = "cl_33728_3";
    private String primaryLocation = "guangzhou.nansha_33728";
    private String sameCityLocation = "guangzhou.panyu_33728";
    private String offsiteLocation = "shenzhan.nanshan_33728";
    private int recordNum = 50000;
    private String expandGroupName;

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
        ReplicaGroup group = sdb.getReplicaGroup( expandGroupName );
        group.setActiveLocation( primaryLocation );

        if ( !CommLib.isLSNConsistency( sdb, SdbTestBase.expandGroupName ) ) {
            Assert.fail( "LSN is not consistency" );
        }

        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }

        sdb.updateConfig(
                new BasicBSONObject( "remotelocationconsistency", false ) );
        CollectionSpace dbcs = sdb.createCollectionSpace( csName );

        // 创建集合，ReplSize为6，ConsistencyStrategy为1
        BasicBSONObject option1 = new BasicBSONObject();
        option1.put( "ReplSize", 6 );
        option1.put( "ConsistencyStrategy", 1 );
        option1.put( "Group", expandGroupName );
        dbcl1 = dbcs.createCollection( clName1, option1 );

        // 创建集合，ReplSize为7，ConsistencyStrategy为2
        BasicBSONObject option2 = new BasicBSONObject();
        option2.put( "ReplSize", 7 );
        option2.put( "ConsistencyStrategy", 2 );
        option2.put( "Group", expandGroupName );
        dbcl2 = dbcs.createCollection( clName2, option2 );

        // 创建集合，ReplSize为0，ConsistencyStrategy为3
        BasicBSONObject option3 = new BasicBSONObject();
        option3.put( "ReplSize", 0 );
        option3.put( "ConsistencyStrategy", 3 );
        option3.put( "Group", expandGroupName );
        dbcl3 = dbcs.createCollection( clName3, option3 );
    }

    @Test
    public void test() throws ReliabilityException {
        // 集合插入数据过程中强杀异地灾备中心所有节点
        ArrayList< BasicBSONObject > offsiteLocationNodes = LocationUtils
                .getGroupLocationNodes( sdb, expandGroupName, offsiteLocation );

        TaskMgr mgr = new TaskMgr();
        for ( BasicBSONObject offsiteLocationNode : offsiteLocationNodes ) {
            FaultMakeTask faultTask = KillNode.getFaultMakeTask(
                    offsiteLocationNode.getString( "hostName" ),
                    offsiteLocationNode.getString( "svcName" ), 0 );
            mgr.addTask( faultTask );
        }

        mgr.addTask( new Insert( csName, clName1 ) );
        mgr.addTask( new Insert( csName, clName2 ) );
        mgr.addTask( new Insert( csName, clName3 ) );
        mgr.execute();
        Assert.assertTrue( mgr.isAllSuccess(), mgr.getErrorMsg() );

        Assert.assertTrue(
                groupMgr.checkBusiness( 600, true, SdbTestBase.coordUrl ),
                "failed to restore business" );

        dbcl1.truncate();
        dbcl2.truncate();
        dbcl3.truncate();

        // 停止异地灾备中心所有节点
        ReplicaGroup group = sdb.getReplicaGroup( expandGroupName );
        for ( BasicBSONObject offsiteLocationNode : offsiteLocationNodes ) {
            String nodeName = offsiteLocationNode.getString( "hostName" ) + ":"
                    + offsiteLocationNode.getString( "svcName" );
            Node node = group.getNode( nodeName );
            node.stop();
        }

        List< BSONObject > batchRecords1 = CommLib.insertData( dbcl1,
                recordNum );
        List< BSONObject > batchRecords2 = CommLib.insertData( dbcl2,
                recordNum );
        List< BSONObject > batchRecords3 = CommLib.insertData( dbcl3,
                recordNum );

        // 校验数据
        BasicBSONObject orderBy = new BasicBSONObject( "a", 1 );
        CommLib.checkRecords( dbcl1, batchRecords1, orderBy );
        CommLib.checkRecords( dbcl2, batchRecords2, orderBy );
        CommLib.checkRecords( dbcl3, batchRecords3, orderBy );

        // 恢复环境
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
        sdb.deleteConfig( new BasicBSONObject( "remotelocationconsistency", 1 ),
                new BasicBSONObject() );
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        if ( sdb != null ) {
            sdb.close();
        }
    }

    private class Insert extends OperateTask {
        private String csName;
        private String clName;

        private Insert( String csName, String clName ) {
            this.csName = csName;
            this.clName = clName;
        }

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection dbcl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                List< BSONObject > batchRecords = CommLib.insertData( dbcl,
                        recordNum );

                BasicBSONObject orderBy = new BasicBSONObject( "a", 1 );
                CommLib.checkRecords( dbcl, batchRecords, orderBy );
            }
        }
    }
}
