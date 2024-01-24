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
import org.bson.types.ObjectId;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.*;

/**
 * @FileName seqDB-3227: 文档写入加新建节点过程中备节点节点异常重启，该备节点为同步的源节点 seqDB-3236:
 *           文档写入加新建节点过程中备节点节点异常重启，该备节点为同步的目的节点
 * @Author linsuqiang
 * @Date 2017-03-27
 * @Version 1.00
 */

/*
 * 1.创建CS，CL 2.循环执行增删改操作 3.往副本组中新增节点 4.过程中购造节点异常重启(kill -9) 5.选主成功后，继续写入
 * 6.过程中故障恢复 7.验证结果 注：和单独测插入或删除不同，这个用例就是为了覆盖综合的场景 所以特地涉足增删改查和lob操作，没有固定的预期结果，
 * 只要节点间数据一致即可。 ReplSize = 2,随机断一个备节点时，该节点有可能是同步的源节点，也有可能是同步的目的节点。
 */

public class CRUDAndAddNode3227 extends SdbTestBase {
    private GroupMgr groupMgr = null;
    private boolean runSuccess = false;
    private String clName = "cl_3227";
    private String clGroupName = null;
    private String randomHost = null;
    private int randomPort = 0;
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
            insertData( cl ); // prepare data for sync

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
            NodeWrapper slvNode = dataGroup.getSlave();

            FaultMakeTask faultTask = KillNode.getFaultMakeTask(
                    slvNode.hostName(), slvNode.svcName(), 1 );
            TaskMgr mgr = new TaskMgr( faultTask );
            CRUDTask cTask = new CRUDTask(clName);
            aTask = new AddNodeTask(clGroupName, randomHost, randomPort);
            mgr.addTask( cTask );
            mgr.addTask( aTask );
            mgr.execute();
            Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

            if ( !groupMgr.checkBusinessWithLSN( 600 ) ) {
                Assert.fail( "checkBusiness occurs time out" );
            }

            db = new Sequoiadb( coordUrl, "", "" );
            checkLob( db );
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
                .parse( "{ Group: '" + clGroupName + "', ReplSize: 2 }" );
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

    private void checkLob( Sequoiadb db ) {
        DBCollection cl = db.getCollectionSpace( csName )
                .getCollection( clName );
        int lobSize = 1 * 1024;
        byte[] lobBytes = new byte[ lobSize ];
        new Random().nextBytes( lobBytes );

        DBLob wLob = cl.createLob();
        wLob.write( lobBytes );
        ObjectId oid = wLob.getID();
        wLob.close();

        DBLob rLob = cl.openLob( oid );
        byte[] rLobBytes = new byte[ lobSize ];
        rLob.read( rLobBytes );
        rLob.close();

        if ( !Arrays.equals( rLobBytes, lobBytes ) ) {
            Assert.fail( "lob is different" );
        }
    }
}
