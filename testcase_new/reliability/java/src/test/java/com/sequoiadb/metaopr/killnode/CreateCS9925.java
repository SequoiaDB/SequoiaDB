package com.sequoiadb.metaopr.killnode;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import com.sequoiadb.metaopr.Utils;
import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.KillNode;
import com.sequoiadb.metaopr.commons.MyUtil;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @FileName seqDB-9925: 创建CS过程中数据主节点被kill -9
 * @Author linsuqiang
 * @Date 2017-03-31
 * @Version 1.00
 */

/*
 * 1、创建CS过程中多次kill -9数据主节点,如执行如下循环语句创建CS过程中kill -9 数据主节点： for(i=0;i<1000;i++) {
 * var csName = "cs"+i; var cl =
 * db.createCS(csName).createCL("cl",{Group:"group1"}); cl.insert({a:1}); }
 * 2、查看节点是否有被正常拉起，且数据正常 http://jira:8080/browse/SEQUOIADBMAINSTREAM-1934
 */

public class CreateCS9925 extends SdbTestBase {
    private GroupMgr groupMgr = null;
    private boolean runSuccess = false;
    private String csNameBase = "cs_9925";
    private String clNameBase = "cl_9925";
    private String clGroupName = null;
    private static final int CS_NUM = 100;

    @BeforeClass
    public void setUp() {
        Sequoiadb db = null;
        try {

            groupMgr = GroupMgr.getInstance();
            if ( !groupMgr.checkBusiness() ) {
                throw new SkipException( "checkBusiness failed" );
            }

            clGroupName = groupMgr.getAllDataGroupName().get( 0 );
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
                    priNode.hostName(), priNode.svcName(), 0 );
            TaskMgr mgr = new TaskMgr( faultTask );
            CreateCSTask cTask = new CreateCSTask();
            mgr.addTask( cTask );
            mgr.execute();
            Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

            if ( !groupMgr.checkBusinessWithLSN( 600 ) ) {
                Assert.fail( "checkBusinessWithLSN() occurs timeout" );
            }

            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            createCSAgain( db );
            operateOnCS( db );

            if ( !groupMgr.checkBusinessWithLSN( 600 ) ) {
                Assert.fail( "checkBusinessWithLSN() occurs timeout" );
            }
            MyUtil.checkListCS( db, csNameBase, CS_NUM );
            // if (!dataGroup.checkInspect(1)) { // Error: Not found any
            // collection
            // Assert.fail("data is different on " + dataGroup.getGroupName());
            // }
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
            dropCS( db );
        } catch ( BaseException e ) {
            Assert.fail(
                    e.getMessage() + "\r\n" + Utils.getKeyStack( e, this ) );
        } finally {
            if ( db != null ) {
                db.close();
            }
        }
    }

    // not only create cs. see details on the head of this file.
    private class CreateCSTask extends OperateTask {
        @Override
        public void exec() throws Exception {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( coordUrl, "", "" );
                for ( int i = 0; i < CS_NUM; i++ ) {
                    String csName = csNameBase + "_" + i;
                    String clName = clNameBase + "_" + i;
                    CollectionSpace cs = db.createCollectionSpace( csName );
                    DBCollection cl = cs.createCollection( clName,
                            ( BSONObject ) JSON.parse(
                                    "{ Group: '" + clGroupName + "' }" ) );
                    cl.insert( "{ a: 1 }" );
                }
            } catch ( BaseException e ) {
            } finally {
                if ( db != null ) {
                    db.close();
                }
            }
        }
    }

    private void createCSAgain( Sequoiadb db ) {
        for ( int i = 0; i < CS_NUM; i++ ) {
            String csName = csNameBase + "_" + i;
            if ( db.isCollectionSpaceExist( csName ) ) {
                db.getCollectionSpace( csName );
            } else {
                db.createCollectionSpace( csName );
            }
        }
    }

    private void operateOnCS( Sequoiadb db ) {
        List< BSONObject > recs = new ArrayList< BSONObject >();
        int recCnt = 3000;
        for ( int i = 0; i < recCnt; i++ ) {
            recs.add( ( BSONObject ) JSON.parse( "{ a: 1 }" ) );
        }

        for ( int i = 0; i < CS_NUM; i++ ) {
            String csName = csNameBase + "_" + i;
            CollectionSpace currCS = db.getCollectionSpace( csName );

            String clName = clNameBase + "_" + i;
            DBCollection cl = null;
            if ( currCS.isCollectionExist( clName ) ) {
                cl = currCS.getCollection( clName );
            } else {
                cl = currCS.createCollection( clName );
            }
            cl.insert( recs, DBCollection.FLG_INSERT_CONTONDUP );

            currCS.dropCollection( clName );
        }
    }

    private void dropCS( Sequoiadb db ) {
        for ( int i = 0; i < CS_NUM; i++ ) {
            String csName = csNameBase + "_" + i;
            db.dropCollectionSpace( csName );
        }
    }
}