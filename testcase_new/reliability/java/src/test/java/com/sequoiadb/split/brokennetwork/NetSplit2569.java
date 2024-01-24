package com.sequoiadb.split.brokennetwork;

import java.util.List;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.DBLob;
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

/**
 * @FileName:SEQDB-2569 对hash分区组进行范围切分，切分时目标组主节点断网,切分数据类型为lob对象
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class NetSplit2569 extends SdbTestBase {
    private String clName = "testcaseCL2569";
    private String srcGroupName;
    private String dstGroupName;
    private GroupMgr groupMgr = null;
    private int writeLobCount;
    private String connectUrl;
    private String brokenNetHost;
    private boolean clearFlag = false;
    private boolean splitComplete = false;

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
            dstGroupName = glist.get( 1 ).getGroupName();
            System.out.println(
                    "split srcRG:" + srcGroupName + " destRG:" + dstGroupName );

            CollectionSpace commCS = sdb.getCollectionSpace( csName );
            DBCollection cl = commCS.createCollection( clName,
                    ( BSONObject ) JSON.parse(
                            "{ShardingKey:{'sk':1},Partition:4096,ShardingType:'hash',Group:'"
                                    + srcGroupName + "'}" ) );
            insertData( cl, 0, 1000 );// 写入待切分的记录（1000普通记录，1000lob）

            // 调整主机
            brokenNetHost = groupMgr.getGroupByName( dstGroupName ).getMaster()
                    .hostName();
            Utils.reelect( brokenNetHost, srcGroupName, Utils.CATA_RG_NAME );
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
            DBLob lob = cl.createLob();
            String id = lob.getID().toString();
            lob.write( id.getBytes() );
            lob.close();
            writeLobCount++;
        }
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
                db = new Sequoiadb( connectUrl, "", "" );
                db.setSessionAttr(
                        ( BSONObject ) JSON.parse( "{PreferedInstance:'M'}" ) );
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                insertData( cl, 3000, 4000 );

                // 百分比切分覆盖
                // GroupWrapper srcGroup =
                // groupMgr.getGroupByName(srcGroupName);
                // GroupWrapper destGroup =
                // groupMgr.getGroupByName(destGroupName);
                // Assert.assertEquals(srcGroup.checkInspect(60), true);
                // Assert.assertEquals(destGroup.checkInspect(60), true);

                long dstLobCount = checkGroupLob( db, dstGroupName );
                long srcLobCount = checkGroupLob( db, srcGroupName );
                DBCursor cursor = cl.listLobs();
                int dbLobCount = 0;
                while ( cursor.hasNext() ) {
                    cursor.getNext();
                    dbLobCount++;
                }
                Assert.assertEquals( dstLobCount + srcLobCount, dbLobCount );
            }
            clearFlag = true;
        } catch ( ReliabilityException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        } finally {
            if ( db != null ) {
                db.close();
            }
        }

    }

    private long checkGroupLob( Sequoiadb sdb, String groupName ) {
        Sequoiadb dataNode = null;
        DBCursor cursor = null;
        int lobCount = 0;
        try {
            dataNode = sdb.getReplicaGroup( groupName ).getMaster().connect();// 获得源主节点链接
            DBCollection destCL = dataNode.getCollectionSpace( csName )
                    .getCollection( clName );
            cursor = destCL.listLobs();
            while ( cursor.hasNext() ) {
                cursor.getNext();
                lobCount++;
            }
            // 数据量应在totalCount / 2条左右（切分范围2048-4096）
            Assert.assertEquals(
                    lobCount > writeLobCount / 2 - ( writeLobCount / 2 * 0.3 )
                            && lobCount < writeLobCount / 2
                                    + ( writeLobCount / 2 * 0.3 ),
                    true, groupName + " = " + lobCount + ", totalCount = "
                            + writeLobCount );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n" + Utils.getStackString( e ) );
        } finally {
            if ( cursor != null ) {
                cursor.close();
            }
            if ( dataNode != null ) {
                dataNode.close();
            }
        }
        return lobCount;
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
            try {
                insertData( cl, 1000, 3000 );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -134 ) {
                    throw e;
                }
            } finally {
                if ( db != null ) {
                    db.close();
                }
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
                    cl.split( srcGroupName, dstGroupName,
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
