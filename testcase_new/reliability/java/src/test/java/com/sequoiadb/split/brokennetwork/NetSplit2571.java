package com.sequoiadb.split.brokennetwork;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.CommLib;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.BrokenNetwork;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;
import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.Date;
import java.util.List;

/**
 * @FileName:SEQDB-2571 对hash分区组进行范围切分，切分时catalog主节点断网
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class NetSplit2571 extends SdbTestBase {
    private String clName = "testcaseCL2571";
    private String srcGroupName;
    private String destGroupName;
    private GroupMgr groupMgr = null;
    private int clTotalCount;
    private String connectUrl;
    private boolean clearFlag = false;
    private boolean splitComplete = false;
    private String brokenNetHost;

    @BeforeClass()
    public void setUp() {
        Sequoiadb sdb = null;
        try {
            groupMgr = GroupMgr.getInstance();

            if ( !groupMgr.checkBusiness( 20 ) ) {
                throw new SkipException( "checkBusiness return false" );
            }
            sdb = new Sequoiadb( coordUrl, "", "" );
            List< GroupWrapper > glist = groupMgr.getAllDataGroup();

            srcGroupName = glist.get( 0 ).getGroupName();
            destGroupName = glist.get( 1 ).getGroupName();
            System.out.println( "split srcRG:" + srcGroupName + " destRG:"
                    + destGroupName );

            CollectionSpace commCS = sdb.getCollectionSpace( csName );
            DBCollection cl = commCS.createCollection( clName,
                    ( BSONObject ) JSON.parse(
                            "{ShardingKey:{'sk':1},Partition:4096,ShardingType:'hash',Group:'"
                                    + srcGroupName + "'}" ) );
            insertData( cl, 0, 2000 );
            // 调整主机
            brokenNetHost = groupMgr.getGroupByName( Utils.CATA_RG_NAME )
                    .getMaster().hostName();
            Utils.reelect( brokenNetHost, srcGroupName, destGroupName );
            connectUrl = CommLib.getSafeCoordUrl( brokenNetHost );
            groupMgr.refresh();
            System.out.println( "brokenHost:" + brokenNetHost + " connectUrl:"
                    + connectUrl );
        } catch ( ReliabilityException e ) {
            Assert.fail( this.getClass().getName()
                    + " setUp error, error description:" + e.getMessage()
                    + "\r\n" + Utils.getStackString( e ) );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    public void insertData( DBCollection cl, int begin, int end ) {
        for ( int i = begin; i < end; i++ ) {
            BSONObject obj = ( BSONObject ) JSON.parse( "{sk:" + i + "}" );
            cl.insert( obj );
        }
        clTotalCount = clTotalCount + ( end - begin );
    }

    @Test
    public void test() {
        Sequoiadb db = null;
        try {
            // 建立并行任务
            FaultMakeTask faultTask = BrokenNetwork
                    .getFaultMakeTask( brokenNetHost, 2, 10 );
            TaskMgr mgr = new TaskMgr( faultTask );
            mgr.addTask( new Split() );
            mgr.addTask( new Insert() );
            mgr.execute();

            Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

            // 最长等待2分钟的环境恢复
            Assert.assertEquals( groupMgr.checkBusiness( 600 ), true,
                    "failed to restore business" );

            if ( splitComplete ) {
                // 再次插入数据
                db = new Sequoiadb( connectUrl, "", "" );
                db.setSessionAttr(
                        ( BSONObject ) JSON.parse( "{PreferedInstance:'M'}" ) );
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                insertData( cl, 8000, 10000 );

                // 百分比切分覆盖
                // GroupWrapper srcGroup =
                // groupMgr.getGroupByName(srcGroupName);
                // GroupWrapper destGroup =
                // groupMgr.getGroupByName(destGroupName);
                // GroupWrapper cataGroup =
                // groupMgr.getGroupByName("SYSCatalogGroup");
                // Assert.assertEquals(srcGroup.checkInspect(60), true);
                // Assert.assertEquals(destGroup.checkInspect(60), true);
                // Assert.assertEquals(cataGroup.checkInspect(60), true);

                // 等待切分任务的结束（由于cata断网，同步切分返回不一定在目标组建立了集合）
                Utils.waitSplit( db, cl.getFullName() );

                long destCount = checkGroupData( db, destGroupName );
                long srcCount = checkGroupData( db, srcGroupName );
                Assert.assertEquals( destCount + srcCount, clTotalCount );
                Assert.assertEquals( cl.getCount( "{sk:{$gte:0,$lt:10000}}" ),
                        clTotalCount );
            }
            clearFlag = true;
        } catch ( ReliabilityException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            if ( db != null ) {
                db.close();
            }
        }

    }

    private long checkGroupData( Sequoiadb sdb, String groupName ) {
        Sequoiadb dataNode = null;
        DBCursor cursor = null;
        try {
            dataNode = sdb.getReplicaGroup( groupName ).getMaster().connect();// 获得目标组主节点链接
            DBCollection cl = dataNode.getCollectionSpace( csName )
                    .getCollection( clName );
            long count = cl.getCount();
            // 组的数据量应该在clTotalCount/2条左右（切分范围2048-4096）
            Assert.assertEquals(
                    count > clTotalCount / 2 - ( clTotalCount / 2 * 0.3 )
                            && count < clTotalCount / 2
                                    + ( clTotalCount / 2 * 0.3 ),
                    true, "destGroup data count:" + count );
            return count;
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() + "\r\n" + Utils.getStackString( e ) );
        } finally {
            if ( cursor != null ) {
                cursor.close();
            }
            if ( dataNode != null ) {
                dataNode.close();
            }
        }
        return 0;
    }

    @AfterClass
    public void tearDown() {
        Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        try {
            if ( clearFlag ) {
                CollectionSpace commCS = db.getCollectionSpace( csName );
                commCS.dropCollection( clName );
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n" + Utils.getStackString( e ) );
        } finally {
            db.close();
        }
    }

    class Insert extends OperateTask {

        @Override
        public void exec() throws Exception {
            try {
                Sequoiadb db = new Sequoiadb( connectUrl, "", "" );
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                insertData( cl, 2000, 8000 );
                db.close();
            } catch ( BaseException e ) {
                System.out.println( "insert have exception:" + e.getMessage() );
            }
        }
    }

    class Split extends OperateTask {
        @Override
        public void exec() throws Exception {
            Sequoiadb sdb = null;
            try {
                sdb = new Sequoiadb( connectUrl, "", "" );
                sdb.setSessionAttr(
                        ( BSONObject ) JSON.parse( "{PreferedInstance:'M'}" ) );
                DBCollection cl = sdb.getCollectionSpace( csName )
                        .getCollection( clName );
                try {
                    cl.split( srcGroupName, destGroupName,
                            ( BSONObject ) JSON.parse( "{Partition:2048}" ), // 切分
                            ( BSONObject ) JSON.parse( "{Partition:4096}" ) );
                    splitComplete = true;
                } catch ( BaseException e ) {
                    System.out.println(
                            "split have exception:" + e.getMessage() );
                }
            } catch ( BaseException e ) {
                throw e;
            } finally {
                if ( sdb != null ) {
                    sdb.close();
                }
            }
        }
    }

}
