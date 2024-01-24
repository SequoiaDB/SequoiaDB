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

import java.util.Date;
import java.util.List;
import java.util.Random;

/**
 * @FileName seqDB-3220: 创建多个唯一索引后，写入文档过程中主节点节点异常重启，该主节点为同步的源节点
 * @Author linsuqiang
 * @Date 2017-03-28
 * @Version 1.00
 */

/*
 * 1.创建CS，CL，在CL上创建多个唯一索引 2.循环执行增删改操作 3.往副本组中新增节点 4.过程中购造节点异常重启(kill -9)
 * 5.选主成功后，继续写入 6.过程中故障恢复 7.验证结果 注：和单独测插入或删除不同，这个用例就是为了覆盖综合的场景
 * 所以特地涉足增删改查和lob操作，没有固定的预期结果， 只要节点间数据一致即可。
 */

public class CRUDWithIndex3220 extends SdbTestBase {
    private GroupMgr groupMgr = null;
    private boolean runSuccess = false;
    private String clName = "cl_3220";
    private String clGroupName = null;
    private String randomHost = null;
    private int randomPort;
    private AddNodeTask aTask = null ;

    @BeforeClass
    public void setUp() {
        Sequoiadb db = null;
        try {

            groupMgr = GroupMgr.getInstance();
            if ( !groupMgr.checkBusiness() ) {
                throw new SkipException( "checkBusiness failed" );
            }

            db = new Sequoiadb( coordUrl, "", "" );
            clGroupName = groupMgr.getAllDataGroupName().get( 0 );
            DBCollection cl = createCL( db );
            createIndexes( cl );

            // node info, which will be used at AddNodeTask and teardown
            Random ran = new Random();
            List< String > hosts = groupMgr.getAllHosts();
            randomHost = hosts.get( ran.nextInt( hosts.size() ) );
            randomPort = ran.nextInt( reservedPortEnd - reservedPortBegin )
                    + reservedPortBegin;

            Utils.makeReplicaLogFull( clGroupName );
        } catch ( ReliabilityException e ) {
            Assert.fail( this.getClass().getName()
                    + " setUp error, error description:" + e.getMessage()
                    + "\r\n" + Utils.getKeyStack( e, this ) );
        } finally {
            if ( db != null ) {
                db.close();
            }
        }
    }

    @Test
    public void test() {
        Sequoiadb db = null;
        try {
            GroupWrapper dataGroup = groupMgr.getGroupByName( clGroupName );
            NodeWrapper priNode = dataGroup.getMaster();

            FaultMakeTask faultTask = KillNode.getFaultMakeTask(
                    priNode.hostName(), priNode.svcName(), 1 );
            TaskMgr mgr = new TaskMgr( faultTask );
            CRUDTask cTask = new CRUDTask(clName);
            aTask = new AddNodeTask(clGroupName, randomHost, randomPort);
            mgr.addTask( cTask );
            mgr.addTask( aTask );
            mgr.execute();
            Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

            if ( !groupMgr.checkBusinessWithLSN( 60 ) ) {
                Assert.fail( "checkBusiness occurs time out" );
            }

            db = new Sequoiadb( coordUrl, "", "" );
            Utils.testLob( db, clName );
            if ( !dataGroup.checkInspect( 1 ) ) {
                Assert.fail(
                        "data is different on " + dataGroup.getGroupName() );
            }
            runSuccess = true;
        } catch ( ReliabilityException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        } finally {
            if ( db != null ) {
                db.close();
            }
        }
    }

    @AfterClass
    public void tearDown() {
        if ( !runSuccess ) {
            throw new SkipException( "to save environment" );
        }
        Sequoiadb db = null;
        try {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            CollectionSpace commCS = db.getCollectionSpace( csName );
            commCS.dropCollection( clName );
            aTask.removeNode();
        } catch ( BaseException e ) {
            Assert.fail(
                    e.getMessage() + "\r\n" + Utils.getKeyStack( e, this ) );
        } finally {
            if ( db != null ) {
                db.close();
            }
        }
    }

    private DBCollection createCL( Sequoiadb db ) {
        CollectionSpace commCS = db.getCollectionSpace( csName );
        BSONObject option = ( BSONObject ) JSON
                .parse( "{ Group: '" + clGroupName + "', ReplSize: 1 }" );
        return commCS.createCollection( clName, option );
    }

    private void createIndexes( DBCollection cl ) {
        for ( int i = 0; i < 10; i++ ) {
            String idxName = "idx_" + i;
            BSONObject key = ( BSONObject ) JSON.parse( "{ a" + i + ": 1 }" );
            cl.createIndex( idxName, key, true, true, 8 );
        }
    }
}
