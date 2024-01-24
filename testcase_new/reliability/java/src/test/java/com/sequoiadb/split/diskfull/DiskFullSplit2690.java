package com.sequoiadb.split.diskfull;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.DiskFull;
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
 * @FileName:SEQDB-2690 对hash分区组进行范围切分，切分时目标组备节点所在服务器磁盘耗尽
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class DiskFullSplit2690 extends SdbTestBase {
    private String clName = "testcaseCL2690";
    private String csName = "testcaseCL2690_cs";
    private String srcGroupName;
    private String destGroupName;
    private Sequoiadb commSdb = null;
    private GroupMgr groupMgr = null;
    private int totalCount;
    private boolean clearFlag = false;
    private String fillUpDiskHost;

    @BeforeClass()
    public void setUp() {
        try {
            commSdb = new Sequoiadb( coordUrl, "", "" );
            groupMgr = GroupMgr.getInstance();

            if ( !groupMgr.checkBusiness( 20 ) ) {
                throw new SkipException( "checkBusiness return false" );
            }
            List< GroupWrapper > glist = groupMgr.getAllDataGroup();

            srcGroupName = glist.get( 0 ).getGroupName();
            destGroupName = glist.get( 1 ).getGroupName();
            System.out.println( "split srcRG:" + srcGroupName + " destRG:"
                    + destGroupName );

            CollectionSpace commCS = commSdb.createCollectionSpace( csName );
            DBCollection cl = commCS.createCollection( clName,
                    ( BSONObject ) JSON.parse(
                            "{ShardingKey:{'sk':1},Partition:4096,ShardingType:'hash',Group:'"
                                    + srcGroupName + "'}" ) );
            insertData( cl, 0, 300 );// 写入待切分的记录（~300M）

            // 调整主机
            fillUpDiskHost = groupMgr.getGroupByName( destGroupName ).getSlave()
                    .hostName();
            Utils.reelect( fillUpDiskHost, Utils.CATA_RG_NAME, srcGroupName );
            groupMgr.refresh();
            System.out.println( "fillUpDiskHost:" + fillUpDiskHost );
        } catch ( ReliabilityException e ) {
            if ( commSdb != null ) {
                commSdb.close();
            }
            Assert.fail( this.getClass().getName()
                    + " setUp error, error description:" + e.getMessage()
                    + "\r\n" + Utils.getStackString( e ) );
        }
    }

    public void insertData( DBCollection cl, int begin, int end ) {
        String padStr = Utils.getString( 1024 * 1024 );
        for ( int i = begin; i < end; i++ ) {
            cl.insert( "{sk:" + i + ",pad:'" + padStr + "'}" );
        }
        totalCount = totalCount + end - begin;
    }

    @Test
    public void test() {
        try {
            // 得到源和目标组的GroupWrapper对象
            GroupWrapper srcGroup = groupMgr.getGroupByName( srcGroupName );
            GroupWrapper destGroup = groupMgr.getGroupByName( destGroupName );

            // 建立并行任务
            FaultMakeTask faultTask = DiskFull.getFaultMakeTask( fillUpDiskHost,
                    SdbTestBase.reservedDir, 0, 10 );
            TaskMgr mgr = new TaskMgr( faultTask );
            mgr.addTask( new Split() );
            mgr.execute();
            Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

            commSdb.setSessionAttr(
                    ( BSONObject ) JSON.parse( "{PreferedInstance:'M'}" ) );
            DBCollection cl = commSdb.getCollectionSpace( csName )
                    .getCollection( clName );
            insertData( cl, 300, 310 );

            Assert.assertEquals( destGroup.checkInspect( 240 ), true );
            Assert.assertEquals( srcGroup.checkInspect( 60 ), true );

            long destCount = checkGroupData( commSdb, destGroupName );
            long srcCount = checkGroupData( commSdb, srcGroupName );
            Assert.assertEquals( destCount + srcCount, totalCount );
            clearFlag = true;
        } catch ( ReliabilityException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        }

    }

    private long checkGroupData( Sequoiadb sdb, String destGroupName ) {
        Sequoiadb destDataNode = null;
        DBCursor cursor = null;
        try {
            destDataNode = sdb.getReplicaGroup( destGroupName ).getMaster()
                    .connect();// 获得源主节点链接
            DBCollection destCL = destDataNode.getCollectionSpace( csName )
                    .getCollection( clName );
            long recCount = destCL.getCount();

            // 数据量应在totalCount / 2条左右（切分范围2048-4096）
            Assert.assertEquals(
                    recCount > totalCount / 2 - ( totalCount / 2 * 0.3 )
                            && recCount < totalCount / 2
                                    + ( totalCount / 2 * 0.3 ),
                    true, "srcGroup count:" + recCount );
            return recCount;
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
        try {
            if ( clearFlag ) {
                commSdb.dropCollectionSpace( csName );
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n" + Utils.getStackString( e ) );
        } finally {
            if ( commSdb != null ) {
                commSdb.close();
            }
        }
    }

    class Split extends OperateTask {
        @Override
        public void exec() throws Exception {
            Sequoiadb sdb = null;
            try {
                sdb = new Sequoiadb( coordUrl, "", "" );
                sdb.setSessionAttr(
                        ( BSONObject ) JSON.parse( "{PreferedInstance:'M'}" ) );
                DBCollection cl = sdb.getCollectionSpace( csName )
                        .getCollection( clName );
                cl.split( srcGroupName, destGroupName,
                        ( BSONObject ) JSON.parse( "{Partition:2048}" ),
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
