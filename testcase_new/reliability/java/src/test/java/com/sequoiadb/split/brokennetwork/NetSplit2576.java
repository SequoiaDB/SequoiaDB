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
 * @FileName:SEQDB-2576 指定hash分区，百分比切分，切分时目标组备节点断网
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class NetSplit2576 extends SdbTestBase {
    private String clName = "testcaseCL2576";
    private String srcGroupName;
    private String destGroupName;
    private GroupMgr groupMgr = null;
    private int totalCount;
    private String connectUrl;
    private boolean clearFlag = false;
    private String brokenNetHost;

    @BeforeClass()
    public void setUp() {
        Sequoiadb commSdb = null;
        try {
            groupMgr = GroupMgr.getInstance();

            if ( !groupMgr.checkBusiness( 20 ) ) {
                throw new SkipException( "checkBusiness return false" );
            }
            commSdb = new Sequoiadb( coordUrl, "", "" );
            List< GroupWrapper > glist = groupMgr.getAllDataGroup();

            srcGroupName = glist.get( 0 ).getGroupName();
            destGroupName = glist.get( 1 ).getGroupName();
            System.out.println( "split srcRG:" + srcGroupName + " destRG:"
                    + destGroupName );

            CollectionSpace commCS = commSdb.getCollectionSpace( csName );
            DBCollection cl = commCS.createCollection( clName,
                    ( BSONObject ) JSON.parse(
                            "{ShardingKey:{'sk':1},Partition:4096,ShardingType:'hash',Group:'"
                                    + srcGroupName + "'}" ) );
            insertData( cl, 0, 1000 );// 写入待切分的记录（1000普通记录）

            // 调整主机
            brokenNetHost = groupMgr.getGroupByName( destGroupName ).getSlave()
                    .hostName();
            Utils.reelect( brokenNetHost, Utils.CATA_RG_NAME, srcGroupName );
            connectUrl = CommLib.getSafeCoordUrl( brokenNetHost );
            groupMgr.refresh();
            System.out.println( "brokenHost:" + brokenNetHost + " connectUrl:"
                    + connectUrl );
        } catch ( ReliabilityException e ) {
            Assert.fail( this.getClass().getName()
                    + " setUp error, error description:" + e.getMessage()
                    + "\r\n" + Utils.getStackString( e ) );
        } finally {
            if ( commSdb != null ) {
                commSdb.close();
            }
        }
    }

    public void insertData( DBCollection cl, int begin, int end ) {
        for ( int i = begin; i < end; i++ ) {
            DBLob lob = cl.createLob();
            String id = lob.getID().toString();
            lob.write( id.getBytes() );
            lob.close();
        }
        totalCount = totalCount + ( end - begin );
    }

    @Test
    public void test() {
        Sequoiadb db = null;
        try {
            // 建立并行任务
            FaultMakeTask faultTask = BrokenNetwork
                    .getFaultMakeTask( brokenNetHost, 1, 10, 30 );
            TaskMgr mgr = new TaskMgr( faultTask );
            mgr.addTask( new Split() );
            mgr.addTask( new Insert() );
            mgr.execute();

            Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

            // 最长等待2分钟的环境恢复
            Assert.assertEquals( groupMgr.checkBusiness( 600 ), true,
                    "failed to restore business" );

            db = new Sequoiadb( connectUrl, "", "" );
            db.setSessionAttr(
                    ( BSONObject ) JSON.parse( "{PreferedInstance:'M'}" ) );
            DBCollection cl = db.getCollectionSpace( csName )
                    .getCollection( clName );
            insertData( cl, 0, 1000 );

            // 结果校验
            GroupWrapper srcGroup = groupMgr.getGroupByName( srcGroupName );
            GroupWrapper destGroup = groupMgr.getGroupByName( destGroupName );
            Assert.assertEquals( srcGroup.checkInspect( 60 ), true );
            Assert.assertEquals( destGroup.checkInspect( 60 ), true );
            long srcCount = checkGroupLob( db, destGroupName );
            long destCount = checkGroupLob( db, srcGroupName );
            Assert.assertEquals( srcCount + destCount, totalCount );

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

    private long checkGroupLob( Sequoiadb sdb, String destGroupName ) {
        Sequoiadb destDataNode = null;
        DBCursor cursor = null;
        try {
            destDataNode = sdb.getReplicaGroup( destGroupName ).getMaster()
                    .connect();// 获得源主节点链接
            DBCollection destCL = destDataNode.getCollectionSpace( csName )
                    .getCollection( clName );

            cursor = destCL.listLobs();
            int lobCount = 0;
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
            return lobCount;
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
        return 0;
    }

    @AfterClass
    public void tearDown() {
        Sequoiadb commSdb = new Sequoiadb( coordUrl, "", "" );
        try {
            if ( clearFlag ) {
                CollectionSpace commCS = commSdb.getCollectionSpace( csName );
                commCS.dropCollection( clName );
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n" + Utils.getStackString( e ) );
        } finally {
            commSdb.close();
        }
    }

    class Insert extends OperateTask {
        @Override
        public void exec() throws Exception {
            Sequoiadb db = new Sequoiadb( connectUrl, "", "" );
            DBCollection cl = db.getCollectionSpace( csName )
                    .getCollection( clName );
            insertData( cl, 0, 1000 );
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
                cl.split( srcGroupName, destGroupName, 50 );
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
