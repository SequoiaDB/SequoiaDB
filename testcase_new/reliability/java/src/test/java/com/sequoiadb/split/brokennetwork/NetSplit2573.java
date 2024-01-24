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
 * @author huangqiaohui
 * @version 1.00
 * @FileName:SEQDB-2573 对hash分区组进行百分比切分，切分时源主节点断网
 *                      备注：断网时，若在lob.close()发生异常，可能产生一个不可用的lob
 */

public class NetSplit2573 extends SdbTestBase {
    private String clName = "testcaseCL2573";
    private String srcGroupName;
    private String destGroupName;
    private GroupMgr groupMgr = null;
    private int recordCount;
    private int lobCount;
    private String connectUrl;
    private boolean clearFlag = false;
    private boolean splitComplete = false;
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
                            "{ShardingKey:{'sk':1},Partition:4096,ReplSize:2,ShardingType:'hash',Group:'"
                                    + srcGroupName + "'}" ) );
            // 准备切分的数据
            insertData( cl, 0, 1000 );
            // 调整主机
            brokenNetHost = groupMgr.getGroupByName( srcGroupName ).getMaster()
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
            DBLob lob = cl.createLob();
            String id = lob.getID().toString();
            lob.write( id.getBytes() );
            lob.close();
        }
        recordCount = recordCount + end - begin;
        lobCount = lobCount + end - begin;
    }

    private FaultMakeTask faultTask = null;

    @Test
    public void test() {
        Sequoiadb db = null;
        try {
            // 建立并行任务
            faultTask = BrokenNetwork.getFaultMakeTask( brokenNetHost, 2, 10,
                    15 );
            TaskMgr mgr = new TaskMgr( faultTask );
            mgr.addTask( new Split() );
            mgr.addTask( new Insert() );
            mgr.execute();

            // TaskMgr检查线程异常
            Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

            // 最长等待2分钟的集群环境恢复
            Assert.assertEquals( groupMgr.checkBusiness( 600 ), true,
                    "failed to restore business" );

            if ( splitComplete ) {
                // 再次插入数据
                db = new Sequoiadb( connectUrl, "", "" );
                db.setSessionAttr(
                        ( BSONObject ) JSON.parse( "{PreferedInstance:'M'}" ) );
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                insertData( cl, 1000000, 1001000 );

                // 源和目标数据量比对,SEQUOIADBMAINSTREAM-1087
                long destCount = checkGroupData( db, destGroupName );
                long srcCount = checkGroupData( db, srcGroupName );
                if ( ( srcCount + destCount ) < recordCount ) {
                    Assert.fail();
                }
                destCount = checkGroupLob( db, destGroupName );
                srcCount = checkGroupLob( db, srcGroupName );
                if ( srcCount + destCount != lobCount
                        && srcCount + destCount != lobCount + 1 ) {
                    Assert.fail( "srcCount:" + srcCount + " destCount:"
                            + destCount + " lobCount:" + lobCount );
                }

                // 已在范围切分覆盖
                // GroupWrapper srcGroup =
                // groupMgr.getGroupByName(srcGroupName);
                // GroupWrapper destGroup =
                // groupMgr.getGroupByName(destGroupName);
                // Assert.assertEquals(srcGroup.checkInspect(60), true);
                // Assert.assertEquals(destGroup.checkInspect(60), true);
            }
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
                    lobCount > recordCount / 2 - ( recordCount / 2 * 0.3 )
                            && lobCount < recordCount / 2
                                    + ( recordCount / 2 * 0.3 ),
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

    private long checkGroupData( Sequoiadb sdb, String groupName ) {
        Sequoiadb dataNode = null;
        DBCursor cursor = null;
        try {
            dataNode = sdb.getReplicaGroup( groupName ).getMaster().connect();// 获得目标组主节点链接
            DBCollection cl = dataNode.getCollectionSpace( csName )
                    .getCollection( clName );
            long count = cl.getCount();
            // 组的数据量应该在totalCount / 2条左右（切分范围2048-4096）
            Assert.assertEquals(
                    count > recordCount / 2 - ( recordCount / 2 * 0.3 )
                            && count < recordCount / 2
                                    + ( recordCount / 2 * 0.3 ),
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
            insertDataForThread( cl );
            db.close();
        }

        private void insertDataForThread( DBCollection cl ) {
            int record = 0;
            int lob = 0;
            for ( int i = 1000; i < 20000; i++ ) {
                BSONObject obj = ( BSONObject ) JSON.parse( "{sk:" + i + "}" );
                try {
                    cl.insert( obj );
                } catch ( BaseException e ) {
                    if ( faultTask != null && faultTask.isMakeSuccess() ) {
                        System.out.println( "insertThread insert record:{sk:"
                                + i + "} :" + e.getMessage()
                                + Utils.getStackString( e ) );
                        break;
                    }
                    throw e;
                }
                record++;
                String id = null;
                try {
                    DBLob dblob = cl.createLob();
                    id = dblob.getID().toString();
                    dblob.write( id.getBytes() );
                    dblob.close();
                } catch ( BaseException e ) {
                    if ( faultTask != null && faultTask.isMakeSuccess() ) {
                        System.out.println( "insertThread create lobNum:" + i
                                + ",lobID:" + id + " " + e.getMessage() + "\r\n"
                                + Utils.getStackString( e ) );
                        break;
                    }
                    throw e;
                }
                lob++;
            }
            recordCount += record;
            lobCount += lob;

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
                    cl.split( srcGroupName, destGroupName, 50 );
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
