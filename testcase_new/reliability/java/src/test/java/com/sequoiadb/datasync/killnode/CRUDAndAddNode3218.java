package com.sequoiadb.datasync.killnode;

import com.sequoiadb.base.*;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.datasync.Utils;
import com.sequoiadb.datasync.CRUDTask;
import com.sequoiadb.datasync.AddNodeTask;
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

import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.Random;

/**
 * @FileName seqDB-3218: 文档写入加新建节点过程中主节点节点异常重启，该主节点为同步的源节点
 * @Author linsuqiang
 * @Date 2017-03-27
 * @Version 1.00
 */

/*
 * 1.创建CS，CL 2.循环执行增删改操作 3.往副本组中新增节点 4.过程中购造节点异常重启(kill -9) 5.选主成功后，继续写入
 * 6.过程中故障恢复 7.验证结果
 */

public class CRUDAndAddNode3218 extends SdbTestBase {
    private GroupMgr groupMgr = null;
    private TaskMgr taskMgr = null;
    private Sequoiadb db = null;
    private boolean runSuccess = false;
    private String clName = "cl_3218";
    private String clGroupName = null;
    private GroupWrapper clGroupWrapper = null;
    private String randomHost ;
    private int randomPort ;
    private AddNodeTask aTask = null ;

    @BeforeClass
    public void setUp() {
        try {
            // 检测集群是否可用
            groupMgr = GroupMgr.getInstance();
            if ( !groupMgr.checkBusiness() ) {
                throw new SkipException( "checkBusiness failed" );
            }
            // 准备用例的公用内容，如cl
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            clGroupName = groupMgr.getAllDataGroupName().get( 0 );
            Utils.makeReplicaLogFull( clGroupName );
            createCLOnGroup( db, clGroupName );

            Random rand = new Random();
            List< String > hosts = groupMgr.getAllHosts();
            randomHost = hosts.get( rand.nextInt( hosts.size() ) );
            randomPort = rand.nextInt( reservedPortEnd - reservedPortBegin )
                    + reservedPortBegin;
            DBCollection cl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
            insertData( cl );

            clGroupWrapper = groupMgr.getGroupByName( clGroupName );
            NodeWrapper priNode = clGroupWrapper.getMaster();
            // 设置任务
            FaultMakeTask faultTask = KillNode.getFaultMakeTask(
                    priNode.hostName(), priNode.svcName(), 1 );
            taskMgr = new TaskMgr( faultTask );
            taskMgr.addTask( new CRUDTask(clName) );
            aTask = new AddNodeTask(clGroupName, randomHost, randomPort) ;
            taskMgr.addTask( aTask );
            // 各个任务各自初始化
            taskMgr.init();
        } catch ( BaseException | ReliabilityException e ) {
            if ( db != null ) {
                db.close();
            }
            Assert.fail( this.getClass().getName()
                    + " setUp error, error description:" + e.getMessage()
                    + "\r\n" + Utils.getKeyStack( e, this ) );
        }
    }

    @Test
    public void test() {
        try {
            // 启动任务
            taskMgr.start();
            taskMgr.join();

            if ( !groupMgr.checkBusiness( 600, true ) ) {
                Assert.fail( "checkBusinessWithExNode occurs time out(1)" );
            }
            // 各个任务检查各自结果
            // Note: 有包含过去的Assert.assertTrue(mgr.isAllSuccess(),
            // mgr.getErrorMsg());
            taskMgr.check();
            // 公共的结果检查，以下为检查cl所在数据组节点间一致性
            if ( !groupMgr.checkBusinessWithLSN( 600 ) ) {
                Assert.fail( "checkBusinessWithExNode occurs time out(2)" );
            }
            if ( !clGroupWrapper.checkInspect( 1 ) ) {
                Assert.fail( "data is different on "
                        + clGroupWrapper.getGroupName() );
            }
            runSuccess = true;
        } catch ( ReliabilityException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( runSuccess ) {
                // 各个任务分别清理自己环境
                taskMgr.fini();
               
                aTask.removeNode();
                // 公用环境清理
                CollectionSpace commCS = db.getCollectionSpace( csName );
                commCS.dropCollection( clName );
            }
        } catch ( BaseException | ReliabilityException e ) {
            Assert.fail(
                    e.getMessage() + "\r\n" + Utils.getKeyStack( e, this ) );
        } finally {
            if ( db != null ) {
                db.close();
            }
        }
    }

    private DBCollection createCLOnGroup( Sequoiadb db, String clGroupName ) {
        CollectionSpace commCS = db.getCollectionSpace( csName );
        BSONObject option = ( BSONObject ) JSON
                .parse( "{ Group: '" + clGroupName + "', ReplSize: 1 }" );
        return commCS.createCollection( clName, option );
    }

        private void insertData( DBCollection cl ) {
            List< BSONObject > recs = new ArrayList< BSONObject >();
            int recNum = 100000;
            for ( int i = 0; i < recNum; i++ ) {
                BSONObject rec = ( BSONObject ) JSON.parse( "{ a: 1 }" );
                recs.add( rec );
            }
            cl.insert( recs, DBCollection.FLG_INSERT_CONTONDUP );
        }

 
    
}
