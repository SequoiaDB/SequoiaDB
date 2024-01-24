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
 * @FileName:SEQDB-2580 对range分区组进行范围切分，切分时源备节点断网
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class NetSplit2580 extends SdbTestBase {
    private String clName = "testcaseCL2580";
    private String srcGroupName;
    private String destGroupName;
    private GroupMgr groupMgr = null;
    private String connectUrl;
    private boolean clearFlag = false;
    private String brokenNetHost;

    @BeforeClass()
    public void setUp() {
        Sequoiadb sdb = null;
        try {
            groupMgr = GroupMgr.getInstance();

            // CheckBusiness(true),检测当前集群环境，若存在异常返回false，
            if ( !groupMgr.checkBusiness( 20 ) ) {
                throw new SkipException( "checkBusiness return false" );
            }
            sdb = new Sequoiadb( coordUrl, "", "" );
            // 确定切分的源和目标组
            List< GroupWrapper > glist = groupMgr.getAllDataGroup();
            srcGroupName = glist.get( 0 ).getGroupName();
            destGroupName = glist.get( 1 ).getGroupName();
            System.out.println( "split srcRG:" + srcGroupName + " destRG:"
                    + destGroupName );

            CollectionSpace commCS = sdb.getCollectionSpace( csName );
            DBCollection cl = commCS.createCollection( clName,
                    ( BSONObject ) JSON.parse(
                            "{ShardingKey:{'sk':1},ShardingType:'range',Group:'"
                                    + srcGroupName + "'}" ) );
            // 准备切分的数据
            insertData( cl, 0, 5000 );

            // 调整主机
            brokenNetHost = groupMgr.getGroupByName( srcGroupName ).getSlave()
                    .hostName();
            Utils.reelect( brokenNetHost, Utils.CATA_RG_NAME, destGroupName );
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
    }

    @Test
    public void test() {
        Sequoiadb db = null;
        try {
            // 建立并行任务
            FaultMakeTask faultTask = BrokenNetwork
                    .getFaultMakeTask( brokenNetHost, 0, 10 );
            TaskMgr mgr = new TaskMgr( faultTask );
            mgr.addTask( new Split() );
            mgr.addTask( new Insert() );
            mgr.execute();

            // TaskMgr检查线程异常
            Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

            // 最长等待2分钟的集群环境恢复
            Assert.assertEquals( groupMgr.checkBusiness( 600 ), true,
                    "failed to restore business" );

            // 再次插入数据
            db = new Sequoiadb( connectUrl, "", "" );
            db.setSessionAttr(
                    ( BSONObject ) JSON.parse( "{PreferedInstance:'M'}" ) );
            DBCollection cl = db.getCollectionSpace( csName )
                    .getCollection( clName );
            insertData( cl, 9000, 10000 );

            // 源和目标数据量比对
            checkGroupData( db, destGroupName, "{sk:{$gte:2500,$lt:7500}}",
                    5000 );
            checkGroupData( db, srcGroupName,
                    "{$or:[{sk:{$gte:7500}},{sk:{$lt:2500}}]}", 5000 );
            Assert.assertEquals( cl.getCount( "{sk:{$gte:0,$lt:10000}}" ),
                    10000 );

            // 百分比切分覆盖
            // GroupWrapper srcGroup = groupMgr.getGroupByName(srcGroupName);
            // GroupWrapper destGroup = groupMgr.getGroupByName(destGroupName);
            // Assert.assertEquals(srcGroup.checkInspect(60), true);
            // Assert.assertEquals(destGroup.checkInspect(60), true);

            clearFlag = true;
        } catch ( ReliabilityException e ) {
            Assert.fail( e.getMessage() + "\r\n" + Utils.getStackString( e ) );
        } finally {
            if ( db != null ) {
                db.close();
            }
        }

    }

    private void checkGroupData( Sequoiadb sdb, String groupName, String macher,
            int expectCount ) {
        Sequoiadb dataNode = null;
        DBCursor cursor = null;
        try {
            dataNode = sdb.getReplicaGroup( groupName ).getMaster().connect();
            DBCollection cl = dataNode.getCollectionSpace( csName )
                    .getCollection( clName );
            long macherCount = cl.getCount( macher );
            long count = cl.getCount();
            Assert.assertEquals( macherCount == count && count == expectCount,
                    true, destGroupName + " count:" + count + " macherCount:"
                            + macherCount );
            ;
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
    }

    @AfterClass
    public void tearDown() {
        Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        try {
            if ( clearFlag ) {
                CollectionSpace commCS = sdb.getCollectionSpace( csName );
                commCS.dropCollection( clName );
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n" + Utils.getStackString( e ) );
        } finally {
            sdb.close();
        }
    }

    class Insert extends OperateTask {
        @Override
        public void exec() throws Exception {
            Sequoiadb db = new Sequoiadb( connectUrl, "", "" );
            DBCollection cl = db.getCollectionSpace( csName )
                    .getCollection( clName );
            insertData( cl, 5000, 9000 );
            db.close();
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
                cl.split( srcGroupName, destGroupName,
                        ( BSONObject ) JSON.parse( "{sk:2500}" ), // 切分
                        ( BSONObject ) JSON.parse( "{sk:7500}" ) );
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
