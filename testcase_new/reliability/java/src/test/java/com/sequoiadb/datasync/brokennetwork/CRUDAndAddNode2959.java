package com.sequoiadb.datasync.brokennetwork;

import com.sequoiadb.base.*;
import com.sequoiadb.commlib.CommLib;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.datasync.Utils;
import com.sequoiadb.datasync.CRUDTask;
import com.sequoiadb.datasync.AddNodeTask;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.BrokenNetwork;
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
 * @FileName seqDB-2948: 新增节点没开始同步前断网
 * @Author linsuqiang
 * @Date 2017-03-29
 * @Version 1.00
 */

/*
 * 1.创建CS，CL 2.循环执行增删改（文档）和增删LOB 3.往副本组中新增节点 4.新增节点没开始同步前构造断网故障(例如：ifdown)
 * 5.继续写入部分LOB 6.过程中故障恢复(例如：ifup) 7.确认结果 注：和单独测插入或删除不同，这个用例就是为了覆盖综合的场景
 * 所以特地涉足增删改查和lob操作，没有固定的预期结果， 只要节点间数据一致即可。
 */

public class CRUDAndAddNode2959 extends SdbTestBase {
    private GroupMgr groupMgr = null;
    private boolean runSuccess = false;
    private String clName = "cl_2948";
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

            // node info, which will be used at AddNodeTask and teardown
            Random ran = new Random();
            List< String > hosts = groupMgr.getAllHosts();
            randomHost = hosts.get( ran.nextInt( hosts.size() ) );
            randomPort = ran.nextInt( reservedPortEnd - reservedPortBegin )
                    + reservedPortBegin;

            clGroupName = groupMgr.getAllDataGroupName().get( 0 );
            GroupWrapper cataGroup = groupMgr
                    .getGroupByName( "SYSCatalogGroup" );
            String cataPriHost = cataGroup.getMaster().hostName();
            if ( cataPriHost.equals( randomHost )
                    && !cataGroup.changePrimary() ) {
                throw new SkipException(
                        cataGroup.getGroupName() + " reelect fail" );
            }

            db = new Sequoiadb( coordUrl, "", "" );
            DBCollection cl = createCL( db );
            insertData( cl ); // prepare data for sync

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
            String dataPriHost = dataGroup.getSlave().hostName();
            if ( dataPriHost.equals( randomHost )
                    && !dataGroup.changePrimary() ) {
                throw new SkipException(
                        dataGroup.getGroupName() + " reelect fail" );
            }

            FaultMakeTask faultTask = BrokenNetwork
                    .getFaultMakeTask( randomHost, 1, 10 );
            TaskMgr mgr = new TaskMgr( faultTask );
            String safeUrl = CommLib.getSafeCoordUrl( randomHost );
            CRUDTask cTask = new CRUDTask(safeUrl, clName );
            aTask = new AddNodeTask( clGroupName, randomHost,
                    randomPort );
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
