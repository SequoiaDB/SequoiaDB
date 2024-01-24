package com.sequoiadb.location.killnode;

import java.util.ArrayList;
import java.util.List;

import com.sequoiadb.base.*;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.location.LocationUtils;
import com.sequoiadb.task.OperateTask;
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
 * @Description seqDB-31771:Critical模式外的节点启停
 * @Author TangTao
 * @Date 2023.06.02
 * @UpdateAuthor TangTao
 * @UpdateDate 2023.06.02
 */
@Test(groups = "location")
public class Critical31771 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private DBCollection dbcl = null;
    private GroupMgr groupMgr;
    private String csName = "cs_31771";
    private String clName = "cl_31771";
    private String primaryLocation = "guangzhou.nansha_31771";
    private String sameCityLocation = "guangzhou.panyu_31771";
    private int recordNum = 10000;
    private int[] replSizes = { 0, -1, 1, 2, 3 };

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

    }

    @Test
    public void test() throws ReliabilityException {
        String groupName = SdbTestBase.expandGroupName;
        ReplicaGroup group = sdb.getReplicaGroup( groupName );
        ArrayList< BasicBSONObject > primaryLocationNodes = LocationUtils
                .getGroupLocationNodes( sdb, groupName, primaryLocation );

        // 主中心异常停止，然后stop节点模拟故障无法启动
        LocationUtils.stopNodeAbnormal( sdb, groupName, primaryLocationNodes );

        // 同城备中心启动Critical模式
        BasicBSONObject options1 = new BasicBSONObject();
        options1.put( "MinKeepTime", 5 );
        options1.put( "MaxKeepTime", 10 );
        options1.put( "Location", sameCityLocation );
        group.startCriticalMode( options1 );

        // 并发执行创建集合插入数据、启停主中心节点
        TaskMgr mgr = new TaskMgr();
        for ( int i : replSizes ) {
            mgr.addTask( new Insert( i ) );
        }
        mgr.addTask( new killNode( primaryLocationNodes.get( 0 ), 1 ) );
        mgr.addTask( new killNode( primaryLocationNodes.get( 1 ), 2 ) );
        mgr.execute();
        Assert.assertTrue( mgr.isAllSuccess(), mgr.getErrorMsg() );

        LocationUtils.checkGroupInCriticalMode( sdb, groupName );

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

    private class Insert extends OperateTask {
        private int replSize = 0;
        private String tempCLName = "tempCL_31771_";

        private Insert( int replsize ) {
            this.replSize = replsize;
            this.tempCLName += replsize;
        }

        @Override
        public void exec() throws Exception {
            Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            // 创建集合、插入数据并校验
            BasicBSONObject option = new BasicBSONObject();
            option.put( "Group", expandGroupName );
            option.put( "ReplSize", replSize );
            DBCollection curcl = db.getCollectionSpace( csName )
                    .createCollection( tempCLName, option );

            List< BSONObject > batchRecords = CommLib.insertData( curcl,
                    recordNum );
            BasicBSONObject orderBy = new BasicBSONObject( "a", 1 );
            CommLib.checkRecords( curcl, batchRecords, orderBy );

        }
    }

    private class killNode extends OperateTask {
        private int type = 0;
        BasicBSONObject node = null;

        private killNode( BasicBSONObject nodes, int type ) {
            this.type = type;
            this.node = nodes;
        }

        @Override
        public void exec() throws Exception {
            Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            ReplicaGroup group = db
                    .getReplicaGroup( SdbTestBase.expandGroupName );
            group.start();
            if ( type == 1 ) {
                String nodeName = node.getString( "hostName" ) + ":"
                        + node.getString( "svcName" );
                Node node = group.getNode( nodeName );
                node.stop();
            }

            if ( type == 2 ) {
                ArrayList< BasicBSONObject > nodes = new ArrayList< BasicBSONObject >();
                nodes.add( node );
                LocationUtils.stopNodeAbnormal( db, SdbTestBase.expandGroupName,
                        nodes );
            }
        }
    }

}
