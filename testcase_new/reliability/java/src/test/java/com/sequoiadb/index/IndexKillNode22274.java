package com.sequoiadb.index;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
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
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @Description seqDB-22274:唯一索引，事务回滚+非事务insert+数据主节点异常重启
 * @Author XiaoNi Huang 2020.06.10
 */

public class IndexKillNode22274 extends SdbTestBase {
    private boolean runSuccess = false;
    private String groupName;
    private GroupMgr groupMgr;
    private String clName = "index22274";
    private int recsNum1 = 50000;
    private int recsNum2 = 100000;

    @BeforeClass()
    private void setUp() throws ReliabilityException {
        groupMgr = GroupMgr.getInstance();
        if ( !groupMgr.checkBusiness() ) {
            throw new SkipException( "checkBusiness return false" );
        }

        groupName = groupMgr.getAllDataGroup().get( 0 ).getGroupName();
        System.out.println( "cl: " + SdbTestBase.csName + "." + clName
                + ", groupName: " + groupName );

        // create collection and unique index
        try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" )) {
            if ( !db.isCollectionSpaceExist( SdbTestBase.csName ) ) {
                db.createCollectionSpace( SdbTestBase.csName );
            }
            CollectionSpace cs = db.getCollectionSpace( SdbTestBase.csName );
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
            DBCollection cl = cs.createCollection( clName,
                    new BasicBSONObject( "Group", groupName ) );
            cl.createIndex( "a", new BasicBSONObject( "a", 1 ), true, false );
        }
    }

    @Test
    private void test() throws ReliabilityException {
        Sequoiadb db1 = null;
        Sequoiadb db2 = null;
        try {
            db1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            db2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

            // db1 beginTrans, insert, remove
            DBCollection cl1 = db1.getCollectionSpace( SdbTestBase.csName )
                    .getCollection( clName );
            db1.beginTransaction();
            Insert( cl1, recsNum1, 0, false );
            cl1.delete( new BasicBSONObject() );

            // db2 insert, duplicate index key
            DBCollection cl2 = db2.getCollectionSpace( SdbTestBase.csName )
                    .getCollection( clName );
            Insert( cl2, recsNum1, 0, true );

            // db1 rollbakTrans + db2 insert + kill master node
            GroupWrapper dataGroup = groupMgr.getGroupByName( groupName );
            NodeWrapper mstNode = dataGroup.getMaster();
            FaultMakeTask faultTask = KillNode.getFaultMakeTask( mstNode, 5 );
            TaskMgr mgr = new TaskMgr( faultTask );
            mgr.addTask( new TransRollbackThread( db1 ) );
            mgr.addTask( new InsertThread( db2 ) );
            mgr.execute();
            Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

            // check cluster
            Assert.assertEquals( groupMgr.checkBusiness( 600 ), true,
                    "failed to restore business" );

            // check results
            Assert.assertTrue( dataGroup.checkInspect( 1 ),
                    "data is different on " + dataGroup.getGroupName() );

            Assert.assertEquals(
                    cl1.getCount( new BasicBSONObject( "a",
                            new BasicBSONObject( "$lt", recsNum1 ) ) ),
                    recsNum1 );

            for ( int i = 0; i < recsNum1; i++ ) {
                Assert.assertEquals(
                        cl1.getCount( new BasicBSONObject( "a", i ) ), 1 );
            }

            runSuccess = true;
        } finally {
            if ( db1 != null ) {
                db1.close();
            }
            if ( db2 != null ) {
                db2.close();
            }
        }
    }

    @AfterClass
    private void tearDown() {
        if ( runSuccess ) {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                CollectionSpace cs = db
                        .getCollectionSpace( SdbTestBase.csName );
                cs.dropCollection( clName );
            }
        }
    }

    private class TransRollbackThread extends OperateTask {
        private Sequoiadb db;

        public TransRollbackThread( Sequoiadb db ) {
            this.db = db;
        }

        @Override
        public void exec() throws Exception {
            Thread.sleep( new Random().nextInt( 10 * 1000 ) );
            db.rollback();
        }
    }

    private class InsertThread extends OperateTask {
        private Sequoiadb db;

        public InsertThread( Sequoiadb db ) {
            this.db = db;
        }

        @Override
        public void exec() throws Exception {
            Thread.sleep( new Random().nextInt( 8 * 1000 ) );
            try {
                DBCollection cl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                Insert( cl, recsNum2, recsNum1, false );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -134 && e.getErrorCode() != -104
                        && e.getErrorCode() != -79 && e.getErrorCode() != -81
                        && e.getErrorCode() != -222 ) {
                    throw e;
                }
            }
        }
    }

    /**
     * 
     * @param cl
     * @param recsNum
     *            records number
     * @param startNum
     *            start records number
     * @param _idDef
     *            _id definition
     */
    private static void Insert( DBCollection cl, int recsNum, int startNum,
            boolean _idDef ) {
        List< BSONObject > recs = new ArrayList< BSONObject >();
        for ( int i = 0; i < recsNum; i++ ) {
            BSONObject obj = new BasicBSONObject();
            if ( _idDef ) {
                obj.put( "_id", i );
            }
            obj.put( "a", i + startNum );
            recs.add( obj );
        }
        cl.insert( recs );
    }

}
