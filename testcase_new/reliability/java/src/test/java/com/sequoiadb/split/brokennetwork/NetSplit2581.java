package com.sequoiadb.split.brokennetwork;

import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

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

/**
 * @Description: seqDB-2581:对range分区组进行范围切分，切分时目标组主节点断网
 * @Author huangqiaohui
 * @Date
 */

public class NetSplit2581 extends SdbTestBase {
    private GroupMgr groupMgr;
    private String srcGroupName;
    private String dstGroupName;
    private String connectUrl;
    private String brokenNetHost;

    private String clName = "split2581";
    private int initRecsNum = 10000;
    private int dstRecsStart = 5000;
    private int dstRecsEnd = 35000;
    private int totalRecsNum = dstRecsEnd + 1000;
    private int faultSuccRecsNum = 0;

    private boolean isSplitSucc = false;
    private boolean clearFlag = false;

    @BeforeClass()
    public void setUp() {
        Sequoiadb sdb = null;
        try {
            groupMgr = GroupMgr.getInstance();
            if ( !groupMgr.checkBusiness( 20 ) ) {
                throw new SkipException( "checkBusiness return false" );
            }
            sdb = new Sequoiadb( coordUrl, "", "" );
            List< GroupWrapper > dataGroups = groupMgr.getAllDataGroup();

            srcGroupName = dataGroups.get( 0 ).getGroupName();
            dstGroupName = dataGroups.get( 1 ).getGroupName();
            System.out.println( "split srcGroupName:" + srcGroupName
                    + " dstGroupName:" + dstGroupName );

            CollectionSpace commCS = sdb.getCollectionSpace( csName );
            DBCollection cl = commCS.createCollection( clName,
                    ( BSONObject ) JSON.parse(
                            "{ShardingKey:{'sk':1},ShardingType:'range',Group:'"
                                    + srcGroupName + "'}" ) );

            // 预置数据，包含一部分目标组切分范围内的数据
            insertData( cl, 0, initRecsNum );

            // 调整主机
            brokenNetHost = groupMgr.getGroupByName( dstGroupName ).getMaster()
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
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    public void insertData( DBCollection cl, int begin, int end ) {
        for ( int i = begin; i < end; i++ ) {
            BSONObject obj = ( BSONObject ) JSON.parse( "{sk:" + i + "}" );
            cl.insert( obj );
            if ( i < dstRecsEnd ) {
                faultSuccRecsNum = i;
            }
        }
    }

    @Test()
    public void test() throws Exception {
        Sequoiadb db = null;
        try {
            // 创建并发任务
            FaultMakeTask faultTask = BrokenNetwork
                    .getFaultMakeTask( brokenNetHost, 2, 10, 15 );
            TaskMgr mgr = new TaskMgr( faultTask );
            mgr.addTask( new Split() );
            mgr.addTask( new Insert() );
            mgr.execute();
            Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

            // 最长等待2分钟的环境恢复
            Assert.assertEquals( groupMgr.checkBusiness( 600 ), true,
                    "failed to restore business" );
            System.out.println( "faultSuccRecsNum: " + faultSuccRecsNum
                    + ", expSuccRecsNum: " + ( dstRecsEnd - 1 ) );

            // 故障恢复后再次插入数据，从故障时插入失败的记录开始插入
            db = new Sequoiadb( connectUrl, "", "" );
            db.setSessionAttr(
                    ( BSONObject ) JSON.parse( "{PreferedInstance:'M'}" ) );
            DBCollection cl = db.getCollectionSpace( csName )
                    .getCollection( clName );
            // 再次插入数据时避免故障时{sk:faultSuccRecsNum+1}可能插入失败程序报错，但实际已经插入成功的情况
            long cnt = cl.getCount(
                    new BasicBSONObject( "sk", faultSuccRecsNum + 1 ) );
            if ( cnt == 1 ) {
                faultSuccRecsNum++;
            }
            insertData( cl, faultSuccRecsNum + 1, totalRecsNum );

            // 校验结果
            if ( isSplitSucc ) {
                Utils.waitSplit( db, cl.getFullName() );
                checkGroupData(
                        db, dstGroupName, "{sk:{$gte:" + dstRecsStart + ",$lt:"
                                + dstRecsEnd + "}}",
                        dstRecsEnd - dstRecsStart );
                checkGroupData( db, srcGroupName,
                        "{$or:[{sk:{$lt:" + dstRecsStart + "}},{sk:{$gte:"
                                + dstRecsEnd + "}}]}",
                        totalRecsNum - ( dstRecsEnd - dstRecsStart ) );
            }
            Assert.assertEquals( cl.getCount(), totalRecsNum );
            clearFlag = true;
        } catch ( ReliabilityException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        } finally {
            if ( db != null )
                db.close();
        }

    }

    private void checkGroupData( Sequoiadb sdb, String groupName, String macher,
            int expectCount ) {
        Sequoiadb nodeDB = null;
        DBCursor cursor = null;
        try {
            nodeDB = sdb.getReplicaGroup( groupName ).getMaster().connect();
            DBCollection cl = nodeDB.getCollectionSpace( csName )
                    .getCollection( clName );
            long macherCount = cl.getCount( macher );
            long count = cl.getCount();
            Assert.assertEquals( macherCount, count );
            Assert.assertEquals( count, expectCount );
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() + "\r\n" + Utils.getStackString( e ) );
        } finally {
            if ( cursor != null ) {
                cursor.close();
            }
            if ( nodeDB != null ) {
                nodeDB.close();
            }
        }
    }

    @AfterClass
    public void tearDown() {
        Sequoiadb sdb = null;
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            if ( clearFlag ) {
                CollectionSpace cs = sdb.getCollectionSpace( csName );
                cs.dropCollection( clName );
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n" + Utils.getStackString( e ) );
        } finally {
            if ( sdb != null )
                sdb.close();
        }
    }

    class Insert extends OperateTask {
        @Override
        public void exec() throws Exception {
            Sequoiadb sdb = null;
            try {
                sdb = new Sequoiadb( connectUrl, "", "" );
                DBCollection cl = sdb.getCollectionSpace( csName )
                        .getCollection( clName );
                insertData( cl, initRecsNum, dstRecsEnd );
            } catch ( BaseException e ) {
                System.out.println( "insert have exception:" + e.getMessage() );
            } finally {
                if ( sdb != null )
                    sdb.close();
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
                            ( BSONObject ) JSON
                                    .parse( "{sk:" + dstRecsStart + "}" ),
                            ( BSONObject ) JSON
                                    .parse( "{sk:" + dstRecsEnd + "}" ) );
                    isSplitSucc = true;
                } catch ( BaseException e ) {
                    System.out.println(
                            "split have exception:" + e.getMessage() );
                }
            } catch ( BaseException e ) {
                throw e;
            } finally {
                if ( sdb != null )
                    sdb.close();
            }
        }
    }

}
