package com.sequoiadb.split.diskfull;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.*;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.DiskFull;
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
 * @FileName:SEQDB-2698 对range分区组进行百分比切分，切分时cata组主节点所在服务器磁盘耗尽
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class DiskFullSplit2698 extends SdbTestBase {
    private String clName = "testcaseCL2698";
    private String csName = "testcaseCL2698_cs";
    private String srcGroupName;
    private String destGroupName;
    private Sequoiadb commSdb = null;
    private GroupMgr groupMgr = null;
    private int totalCount;
    private boolean clearFlag = false;
    private String fillUpDiskHost;
    private DiskFull diskFull;
    private boolean splitComplete = true;

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
                            "{ShardingKey:{'sk':1},ShardingType:'range',Group:'"
                                    + srcGroupName + "'}" ) );
            insertData( cl, 0, 5000 );// 写入待切分的记录

            // 调整主机
            NodeWrapper cataMaster = groupMgr
                    .getGroupByName( Utils.CATA_RG_NAME ).getMaster();
            fillUpDiskHost = cataMaster.hostName();
            Utils.reelect( fillUpDiskHost, destGroupName, srcGroupName );
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
        for ( int i = begin; i < end; i++ ) {
            cl.insert( "{sk:" + i + "}" );
        }
        totalCount = totalCount + end - begin;
    }

    // private void fillUpCatalogSYSCL(String name, String padStr, NodeWrapper
    // master) {
    // Sequoiadb db = null;
    // try {
    // System.out.println("strlen:" + padStr.length());
    // db = new Sequoiadb(master.hostName() + ":" + master.svcName(), "", "");
    // DBCollection cl =
    // db.getCollectionSpace("SYSCAT").getCollection("SYSCOLLECTIONS");
    // int i = 0;
    // try {
    // while (true) {
    // cl.insert("{Name:'" + name + i + "',pad:'" + padStr + i +
    // "',deleteFlag:1}");
    // i++;
    // }
    // }
    // catch (BaseException e) {
    // System.out.println("fillUpCataSYSCL:" + e.getErrorCode());
    // if (e.getErrorCode() != -11) {
    // throw e;
    // }
    // }
    // }
    // finally {
    // if (db != null) {
    // db.close();
    // }
    // }
    // }

    @Test
    public void test() {
        try {
            // 得到源和目标组的GroupWrapper对象
            GroupWrapper srcGroup = groupMgr.getGroupByName( srcGroupName );
            GroupWrapper destGroup = groupMgr.getGroupByName( destGroupName );
            GroupWrapper cataGroup = groupMgr
                    .getGroupByName( Utils.CATA_RG_NAME );
            NodeWrapper cataMaster = cataGroup.getMaster();

            diskFull = new DiskFull( cataMaster.hostName(),
                    SdbTestBase.reservedDir );
            diskFull.init();
            diskFull.make();

            // // 分别以每条记录1m，512K,1k,512B,256B的大小填充SYSCAT.SYSCOLLECTIONS至-11错误
            // fillUpCatalogSYSCL("pad_M", Utils.getString(1024 * 1024),
            // cataMaster);
            // fillUpCatalogSYSCL("pad_HM", Utils.getString(512 * 1024),
            // cataMaster);
            // fillUpCatalogSYSCL("pad_K", Utils.getString(1024), cataMaster);
            // fillUpCatalogSYSCL("pad_HK", Utils.getString(512), cataMaster);
            // fillUpCatalogSYSCL("pad_HHK", Utils.getString(256), cataMaster);
            DBCollection sysCL = cataMaster.connect()
                    .getCollectionSpace( "SYSCAT" )
                    .getCollection( "SYSCOLLECTIONS" );
            CommLib.fillUpCL( sysCL, 256 );

            // 启动Split的线程，及填充SYSCAT.SYSCOLLECTIONS的线程（每条记录~128字节）
            TaskMgr mgr = new TaskMgr();
            mgr.addTask( new Split() );
            mgr.addTask( new fillUp() );
            mgr.execute();

            // 检测线程执行结果
            Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

            commSdb.setSessionAttr(
                    ( BSONObject ) JSON.parse( "{PreferedInstance:'M'}" ) );
            DBCollection cl = commSdb.getCollectionSpace( csName )
                    .getCollection( clName );
            insertData( cl, 5000, 6000 );

            Assert.assertEquals( srcGroup.checkInspect( 60 ), true );
            if ( splitComplete ) {
                Assert.assertEquals( destGroup.checkInspect( 240 ), true );

                checkGroupData( commSdb, destGroupName );
                checkGroupData( commSdb, srcGroupName );
            }
            Assert.assertEquals( cl.getCount( "{sk:{$gte:0,$lt:6000}}" ),
                    totalCount );
            clearFlag = true;
        } catch ( ReliabilityException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        }

    }

    private void checkGroupData( Sequoiadb sdb, String destGroupName ) {
        Sequoiadb destDataNode = null;
        DBCursor cursor = null;
        try {
            destDataNode = sdb.getReplicaGroup( destGroupName ).getMaster()
                    .connect();// 获得源主节点链接
            DBCollection destCL = destDataNode.getCollectionSpace( csName )
                    .getCollection( clName );
            long recCount = destCL.getCount();

            // 数据量应在totalCount / 2条左右（切分范围2048-4096）
            Assert.assertEquals( recCount, totalCount / 2,
                    "srcGroup count:" + recCount );
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

    class fillUp extends OperateTask {
        @Override
        public void exec() throws Exception {
            Sequoiadb db = null;
            try {
                NodeWrapper cataMaster = GroupMgr.getInstance()
                        .getGroupByName( Utils.CATA_RG_NAME ).getMaster();
                db = cataMaster.connect();
                DBCollection cl = db.getCollectionSpace( "SYSCAT" )
                        .getCollection( "SYSCOLLECTIONS" );
                // 分别以每条记录128字节的大小填充SYSCAT.SYSCOLLECTIONS至-11错误
                // fillUpCatalogSYSCL("pad_HHHK", Utils.getString(128),
                // cataMaster);
                CommLib.fillUpCL( cl, 128 );

                // 使故障持续10秒
                Thread.sleep( 10000 );

                // 清除填充至SYSCAT.SYSCOLLECTIONS的数据

                cl.delete( "{deleteFlag:1}" );
            } finally {
                db.close();
                diskFull.restore();
                diskFull.fini();
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
                        ( BSONObject ) JSON.parse( "{sk:0}" ),
                        ( BSONObject ) JSON.parse( "{sk:3000}" ) );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -11 ) {
                    throw e;
                }
                splitComplete = false;
            } finally {
                if ( sdb != null ) {
                    sdb.close();
                }
            }
        }
    }

}
