package com.sequoiadb.location.killnode;

import java.util.ArrayList;
import java.util.List;

import com.sequoiadb.commlib.*;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.KillNode;
import com.sequoiadb.location.LocationUtils;
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

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;

/**
 * @Description seqDB-31337:集合设置ReplSize为3，位置多数派优先，插入数据过程中亲和性Location异常中一个节点异常
 * @Author TangTao
 * @Date 2023.05.04
 * @UpdateAuthor TangTao
 * @UpdateDate 2023.05.04
 * @version 1.0
 */
@Test(groups = "location")
public class Location31337 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private GroupMgr groupMgr;
    private DBCollection dbcl = null;
    private String csName = "cs_31337";
    private String clName = "cl_31337";
    private String primaryLocation = "guangzhou.nansha_31337";
    private String sameCityLocation = "guangzhou.panyu_31337";
    private String offsiteLocation = "shenzhen.nanshan_31337";
    private List< BSONObject > batchRecords;
    private int recordNum = 200000;

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
        if ( !CommLib.isLSNConsistency( sdb, SdbTestBase.expandGroupName ) ) {
            Assert.fail( "LSN is not consistency" );
        }

        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }

        CollectionSpace dbcs = sdb.createCollectionSpace( csName );

        BasicBSONObject option1 = new BasicBSONObject();
        option1.put( "ReplSize", 3 );
        option1.put( "ConsistencyStrategy", 2 );
        option1.put( "Group", SdbTestBase.expandGroupNames.get( 0 ) );
        dbcl = dbcs.createCollection( clName, option1 );
    }

    @Test
    public void test() throws ReliabilityException {
        String groupName = SdbTestBase.expandGroupNames.get( 0 );
        ArrayList< BasicBSONObject > primaryLocationSlaveNodes = LocationUtils
                .getGroupLocationSlaveNodes( sdb, groupName, primaryLocation );
        ArrayList< BasicBSONObject > sameCityLocationNodes = LocationUtils
                .getGroupLocationNodes( sdb, groupName, sameCityLocation );

        TaskMgr mgr = new TaskMgr();
        BasicBSONObject sameCityLocationNode = sameCityLocationNodes.get( 0 );
        sameCityLocationNodes.remove( 0 );
        FaultMakeTask faultTask = KillNode.getFaultMakeTask(
                sameCityLocationNode.getString( "hostName" ),
                sameCityLocationNode.getString( "svcName" ), 0 );
        mgr.addTask( faultTask );

        mgr.addTask( new Insert( primaryLocationSlaveNodes,
                sameCityLocationNodes ) );
        mgr.execute();
        Assert.assertTrue( mgr.isAllSuccess(), mgr.getErrorMsg() );

        // 集群环境恢复后校验数据
        Assert.assertTrue(
                groupMgr.checkBusiness( 600, true, SdbTestBase.coordUrl ),
                "failed to restore business" );
        BasicBSONObject orderBy = new BasicBSONObject( "a", 1 );
        CommLib.checkRecords( dbcl, batchRecords, orderBy );
    }

    @AfterClass
    public void tearDown() {
        LocationUtils.cleanLocation( sdb, SdbTestBase.expandGroupName );
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        if ( sdb != null ) {
            sdb.close();
        }
    }

    private class Insert extends OperateTask {
        private ArrayList< BasicBSONObject > primaryNodes;
        private ArrayList< BasicBSONObject > sameCityNodes;

        private Insert( ArrayList< BasicBSONObject > primary,
                ArrayList< BasicBSONObject > sameCity ) {
            this.primaryNodes = primary;
            this.sameCityNodes = sameCity;
        }

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection dbcl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                batchRecords = CommLib.insertData( dbcl, recordNum );
                // 校验数据已经同步到具有亲和性的Location
                LocationUtils.checkRecordSync( csName, clName, recordNum,
                        sameCityNodes );
                LocationUtils.checkRecordSync( csName, clName, recordNum,
                        primaryNodes );
            }
        }
    }
}
