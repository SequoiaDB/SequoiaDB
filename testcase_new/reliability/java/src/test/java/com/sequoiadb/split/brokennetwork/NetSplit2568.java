package com.sequoiadb.split.brokennetwork;

import com.sequoiadb.base.*;
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
 * @FileName:SEQDB-2568 指定hash分区，按范围切分数据，切分数据类型为普通记录+log对象,切分时源备节点断网
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class NetSplit2568 extends SdbTestBase {
    private String clName = "testcaseCL2568";
    private String srcGroupName;
    private String destGroupName;
    private GroupMgr groupMgr = null;
    private int totalCount;
    private String connectUrl;
    private String brokenNetHost;
    private boolean clearFlag = false;

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
                            "{ShardingKey:{'sk':1},Partition:4096,ShardingType:'hash',Group:'"
                                    + srcGroupName + "'}" ) );
            // 准备切分的数据
            insertData( cl, 0, 800 );

            // 切主,断网主机上的目标组，编目组
            brokenNetHost = groupMgr.getGroupByName( srcGroupName ).getSlave()
                    .hostName();
            Utils.reelect( brokenNetHost, destGroupName, Utils.CATA_RG_NAME );
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
            DBLob lob = cl.createLob();
            String id = lob.getID().toString();
            lob.write( id.getBytes() );
            lob.close();
        }
        totalCount = totalCount + end - begin;
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
            insertData( cl, 5000, 6000 );

            // 源和目标数据量比对
            long destCount = checkGroupData( db, destGroupName );
            long srcCount = checkGroupData( db, srcGroupName );
            Assert.assertEquals( srcCount + destCount, totalCount );
            destCount = checkGroupLob( db, destGroupName );
            srcCount = checkGroupLob( db, srcGroupName );
            Assert.assertEquals( srcCount + destCount, totalCount );
            Assert.assertEquals( cl.getCount( "{sk:{$gte:0,$lt:6000}}" ),
                    totalCount );
            // 组间一致性校验，尝试至多60次
            GroupWrapper srcGroup = groupMgr.getGroupByName( srcGroupName );
            GroupWrapper destGroup = groupMgr.getGroupByName( destGroupName );
            Assert.assertEquals( srcGroup.checkInspect( 60 ), true );
            Assert.assertEquals( destGroup.checkInspect( 60 ), true );

            clearFlag = true;
        } catch ( ReliabilityException e ) {
            Assert.fail( e.getMessage() + "\r\n" + Utils.getStackString( e ) );
        } finally {
            if ( db != null ) {
                db.close();
            }
        }

    }

    private long checkGroupLob( Sequoiadb sdb, String destGroupName ) {
        Sequoiadb destDataNode = null;
        DBCursor cursor = null;
        int lobCount = 0;
        try {
            destDataNode = sdb.getReplicaGroup( destGroupName ).getMaster()
                    .connect();// 获得源主节点链接
            DBCollection destCL = destDataNode.getCollectionSpace( csName )
                    .getCollection( clName );

            cursor = destCL.listLobs();

            while ( cursor.hasNext() ) {
                cursor.getNext();
                lobCount++;
            }
            // 数据量应在totalCount / 2条左右（切分范围2048-4096）
            Assert.assertEquals(
                    lobCount > totalCount / 2 - ( totalCount / 2 * 0.3 )
                            && lobCount < totalCount / 2
                                    + ( totalCount / 2 * 0.3 ),
                    true, "srcGroup count:" + lobCount );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n" + Utils.getStackString( e ) );
        } finally {
            if ( cursor != null ) {
                cursor.close();
            }
            if ( destDataNode != null ) {
                destDataNode.close();
            }
        }
        return lobCount;
    }

    private long checkGroupData( Sequoiadb sdb, String groupName ) {
        Sequoiadb dataNode = null;
        DBCursor cursor = null;
        long count = 0;
        try {
            dataNode = sdb.getReplicaGroup( groupName ).getMaster().connect();// 获得目标组主节点链接
            DBCollection cl = dataNode.getCollectionSpace( csName )
                    .getCollection( clName );
            count = cl.getCount();
            // 组的数据量应该在totalCount / 2条左右（切分范围2048-4096）
            Assert.assertEquals(
                    count > totalCount / 2 - ( totalCount / 2 * 0.3 )
                            && count < totalCount / 2
                                    + ( totalCount / 2 * 0.3 ),
                    true, "destGroup data count:" + count );
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
        return count;
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
            insertData( cl, 800, 5000 );
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
                        ( BSONObject ) JSON.parse( "{Partition:2048}" ), // 切分
                        ( BSONObject ) JSON.parse( "{Partition:4096}" ) );
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
