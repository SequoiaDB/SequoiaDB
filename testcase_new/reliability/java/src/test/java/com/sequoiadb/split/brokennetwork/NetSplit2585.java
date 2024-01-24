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
import org.bson.types.BasicBSONList;
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
 * @FileName:SEQDB-2585 对range分区组进行百分比切分，切分时源主节点断网
 */

public class NetSplit2585 extends SdbTestBase {
    private String clName = "testcaseCL2585";
    private String srcGroupName;
    private String destGroupName;
    private GroupMgr groupMgr = null;
    private String connectUrl;
    private boolean clearFlag = false;
    private boolean splitComplete = false;
    private int exceptionRecNum;
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
                            "{ShardingKey:{'sk':1},ShardingType:'range',ReplSize:2,Group:'"
                                    + srcGroupName + "'}" ) );
            // 准备切分的数据
            insertData( cl, 0, 5000 );

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
            sdb.close();
        }
    }

    public void insertData( DBCollection cl, int begin, int end ) {
        for ( int i = begin; i < end; i++ ) {
            BSONObject obj = ( BSONObject ) JSON.parse( "{sk:" + i + "}" );
            cl.insert( obj );
        }
    }

    private FaultMakeTask faultTask = null;

    @Test
    public void test() {
        Sequoiadb db = null;
        try {
            // 建立并行任务
            faultTask = BrokenNetwork.getFaultMakeTask( brokenNetHost, 5, 15,
                    25 );

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
                insertData( cl, 50000, 51000 );

                // 源和目标数据量比对
                int bound = getBound( db );
                // exceptionRecNum + 5000 + 1000 - bound
                // :插入线程插入的数据+setUp插入的数据+所有线程结束后插入的数据-切分至源组的数据 = 目标组数据量
                long count = checkGroupData( db, destGroupName,
                        "{sk:{$gte:" + bound + "}}" );
                if ( count != exceptionRecNum + 5000 + 1000 - bound
                        && count != exceptionRecNum + 5000 + 1000 - bound
                                + 1 ) {
                    Assert.fail( "count:" + count + " exptionRecNum:"
                            + exceptionRecNum + " bound:" + bound );
                }
                Assert.assertEquals( checkGroupData( db, srcGroupName,
                        "{sk:{$lt:" + bound + "}}" ), bound );

                // 组间一致性校验，尝试至多30次，每次间隔1秒
                GroupWrapper srcGroup = groupMgr.getGroupByName( srcGroupName );
                GroupWrapper destGroup = groupMgr
                        .getGroupByName( destGroupName );
                Assert.assertEquals( srcGroup.checkInspect( 60 ), true );
                Assert.assertEquals( destGroup.checkInspect( 60 ), true );
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

    private int getBound( Sequoiadb commSdb ) {
        DBCursor cursor = null;
        BSONObject lowBound = null;
        BSONObject upBound = null;
        try {
            cursor = commSdb.getSnapshot( Sequoiadb.SDB_SNAP_CATALOG,
                    "{Name:\"" + csName + "." + clName + "\"}", null, null );
            BasicBSONList list = null;
            if ( cursor.hasNext() ) {
                list = ( BasicBSONList ) cursor.getNext().get( "CataInfo" );
            } else {
                Assert.fail( clName + " collection catalog not found" );
            }
            for ( int i = 0; i < list.size(); i++ ) {
                String groupName = ( String ) ( ( BSONObject ) list.get( i ) )
                        .get( "GroupName" );
                if ( groupName.equals( destGroupName ) ) {// 目标组编目信息检查
                    lowBound = ( BSONObject ) ( ( BSONObject ) list.get( i ) )
                            .get( "LowBound" );

                }
                if ( groupName.equals( srcGroupName ) ) {// 源组编目信息检查
                    upBound = ( BSONObject ) ( ( BSONObject ) list.get( i ) )
                            .get( "UpBound" );

                }
            }
            if ( !upBound.equals( lowBound ) ) {
                Assert.fail( "get lowbound upbound fail:" + list );
            }
            return ( int ) upBound.get( "sk" );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n" + Utils.getStackString( e ) );
        } finally {
            if ( cursor != null ) {
                cursor.close();
            }
        }
        return ( int ) upBound.get( "UpBound" );

    }

    private long checkGroupData( Sequoiadb sdb, String groupName,
            String macher ) {
        Sequoiadb dataNode = null;
        DBCursor cursor = null;
        try {
            dataNode = sdb.getReplicaGroup( groupName ).getMaster().connect();
            DBCollection cl = dataNode.getCollectionSpace( csName )
                    .getCollection( clName );
            long macherCount = cl.getCount( macher );
            long count = cl.getCount();
            Assert.assertEquals( macherCount == count, true, destGroupName
                    + " count:" + count + " macherCount:" + macherCount );
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
            for ( int i = 5000; i < 50000; i++ ) {
                BSONObject obj = ( BSONObject ) JSON.parse( "{sk:" + i + "}" );
                try {
                    cl.insert( obj );
                } catch ( BaseException e ) {
                    if ( faultTask != null && faultTask.isMakeSuccess() ) {
                        System.out.println( "insertThread insert record:{sk:"
                                + i + "} :" + e.getMessage()
                                + Utils.getStackString( e ) );
                        exceptionRecNum = i - 5000;
                        return;
                    }
                    throw e;
                }
            }
            exceptionRecNum = 45000;
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
