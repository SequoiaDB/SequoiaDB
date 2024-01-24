package com.sequoiadb.split.killnode;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.KillNode;
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
 * @FileName:SEQDB-2771 对range分区组进行百分比切分，切分时cata组主节点异常重启
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class KillNodeSplit2771 extends SdbTestBase {
    private String clName = "testcaseCL2771";
    private String srcGroupName;
    private String destGroupName;
    private GroupMgr groupMgr = null;
    private int totalCount;
    private Sequoiadb commSdb;
    private boolean clearFlag = false;
    private boolean splitComplete = false;

    @BeforeClass()
    public void setUp() {
        try {

            groupMgr = GroupMgr.getInstance();

            // CheckBusiness(true),检测当前集群环境，若存在异常返回false，
            if ( !groupMgr.checkBusiness( 20 ) ) {
                throw new SkipException( "checkBusiness return false" );
            }

            // 确定切分的源和目标组
            List< GroupWrapper > glist = groupMgr.getAllDataGroup();
            srcGroupName = glist.get( 0 ).getGroupName();
            destGroupName = glist.get( 1 ).getGroupName();
            System.out.println( "split srcRG:" + srcGroupName + " destRG:"
                    + destGroupName );

            commSdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            CollectionSpace commCS = commSdb.getCollectionSpace( csName );
            DBCollection cl = commCS.createCollection( clName,
                    ( BSONObject ) JSON.parse(
                            "{ShardingKey:{'sk':1},ShardingType:'range',Group:'"
                                    + srcGroupName + "'}" ) );
            // 准备切分的数据
            insertData( cl, 0, 5000 );
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
            BSONObject obj = ( BSONObject ) JSON.parse( "{sk:" + i + "}" );
            cl.insert( obj );
        }
        totalCount = totalCount + end - begin;
    }

    @Test
    public void test() throws Exception {
        try {
            // 获取源组主节点的主机名端口号
            GroupWrapper srcGroup = groupMgr.getGroupByName( srcGroupName );
            GroupWrapper destGroup = groupMgr.getGroupByName( destGroupName );
            GroupWrapper cataGroup = groupMgr
                    .getGroupByName( Utils.CATA_RG_NAME );
            NodeWrapper cataMasterNode = cataGroup.getMaster();
            System.out.println( "KillNode:" + cataMasterNode.hostName() + ":"
                    + cataMasterNode.svcName() );

            // 建立并行任务
            FaultMakeTask faultTask = KillNode.getFaultMakeTask(
                    cataMasterNode.hostName(), cataMasterNode.svcName(), 0 );
            TaskMgr mgr = new TaskMgr( faultTask );
            mgr.addTask( new Split() );
            mgr.execute();

            // TaskMgr检查线程异常
            Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

            // 最长等待2分钟的集群环境恢复
            Assert.assertEquals( groupMgr.checkBusiness( 600 ), true,
                    "failed to restore business" );

            // 再次插入数据
            commSdb.setSessionAttr(
                    ( BSONObject ) JSON.parse( "{PreferedInstance:'M'}" ) );
            DBCollection cl = commSdb.getCollectionSpace( csName )
                    .getCollection( clName );
            insertData( cl, 5000, 5100 );

            if ( splitComplete ) {
                // 切分任务已执行完后，再执行源和目标数据量比对
                Assert.assertEquals( destGroup.checkInspect( 60 ), true );
                Assert.assertEquals( srcGroup.checkInspect( 60 ), true );
                Assert.assertEquals( cataGroup.checkInspect( 60 ), true );

                Utils.waitSplit( commSdb, cl.getFullName() );
                int bound = Utils.getBound( commSdb, csName + "." + clName,
                        srcGroupName, destGroupName );
                long destCount = checkGroupData( commSdb, destGroupName );
                Assert.assertEquals( destCount, 5100 - bound );
                long srcCount = checkGroupData( commSdb, srcGroupName );
                Assert.assertEquals( srcCount, bound );
                Assert.assertEquals( cl.getCount( "{sk:{$gte:0,$lt:5100}}" ),
                        5100 );

            } else {
                // 切分任务建立失败，数据全部在源组上
                Assert.assertEquals( srcGroup.checkInspect( 60 ), true );
                Assert.assertEquals( cataGroup.checkInspect( 60 ), true );

                long srcCount = cl.getCount();
                Assert.assertEquals( srcCount, totalCount );
            }
            clearFlag = true;
        } catch ( ReliabilityException e ) {
            Assert.fail( e.getMessage() + "\r\n" + Utils.getStackString( e ) );
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
        try {
            if ( clearFlag ) {
                CollectionSpace commCS = commSdb.getCollectionSpace( csName );
                commCS.dropCollection( clName );
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
                sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
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
