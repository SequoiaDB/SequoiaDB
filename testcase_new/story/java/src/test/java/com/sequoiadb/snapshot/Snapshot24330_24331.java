package com.sequoiadb.snapshot;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * @Description: seqDB-24330 : 在 data 节点上检测简单死锁 seqDB-24331 在 data 节点上使用 forceSession() 解除死锁
 * @Author Yang Qincheng
 * @Date 2021.09.02
 */
public class Snapshot24330_24331 extends SdbTestBase {
    private Sequoiadb db;
    private Sequoiadb dataNode = null;
    private CollectionSpace cs;
    private final String clName1 = "cl_24331_A";
    private final String clName2 = "cl_24331_B";
    private final String clName3 = "cl_24331_C";
    private final static AtomicInteger count = new AtomicInteger(3);
    private final static Object syncObj = new Object();
    private final static int TIMEOUT = 60 * 1000; // 1 min, the default value of transactiontimeout is 1 min
    private final static int INTERVAL_TIME = 200;  // 200 ms

    @BeforeClass
    public void setUp(){
        db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cs = db.getCollectionSpace( csName );
        BSONObject option1 = new BasicBSONObject();
        BSONObject option2 = new BasicBSONObject();
        if ( !CommLib.isStandAlone( db ) ){
            ArrayList< String > groupList =  CommLib.getDataGroupNames( db );
            if ( groupList.size() < 1 ){
                Assert.fail( "At least two data groups are required" );
            }
            int num = 0;
            option1.put("Group", groupList.get( num ) );
            num = ++num < groupList.size() - 1 ? num : groupList.size() - 1;
            option2.put("Group", groupList.get( num ) );

            String nodeName = db.getReplicaGroup( groupList.get( num ) ).getMaster().getNodeName();
            dataNode = new Sequoiadb( nodeName, "", "" );
        }
        DBCollection cl1 = cs.createCollection( clName1, option1 );
        DBCollection cl2 = cs.createCollection( clName2, option2 );
        DBCollection cl3 = cs.createCollection( clName3, option2 );

        SnapshotUtil.insertData( cl1 );
        SnapshotUtil.insertData( cl2 );
        SnapshotUtil.insertData( cl3 );
    }

    @AfterClass
    public void tearDown(){
        try {
            cs.dropCollection( clName1 );
            cs.dropCollection( clName2 );
            cs.dropCollection( clName3 );
        }finally {
            db.close();
            if ( dataNode != null ){
                dataNode.close();
            }
        }
    }

    @Test
    public void test(){
        UpdateTrans trans1 = new UpdateTrans( clName1, clName2 );
        UpdateTrans trans2 = new UpdateTrans( clName2, clName3 );
        UpdateTrans trans3 = new UpdateTrans( clName3, clName2 );
        List<UpdateTrans> transList = new ArrayList<>();
        transList.add( trans2 );
        transList.add( trans3 );
        GetAndCheckSnap snap = new GetAndCheckSnap( transList );

        trans1.start();
        trans2.start();
        trans3.start();
        snap.start();

        Assert.assertTrue( trans1.isSuccess(),  trans1.getErrorMsg() );
        Assert.assertTrue( trans2.isSuccess(),  trans2.getErrorMsg() );
        Assert.assertTrue( trans3.isSuccess(),  trans3.getErrorMsg() );
        Assert.assertTrue( snap.isSuccess(),  snap.getErrorMsg() );
    }

    class UpdateTrans extends SdbThreadBase{
        private String clName1;
        private String clName2;
        private Sequoiadb db;
        private String modifier = "{$set: {a: 2}}";

        UpdateTrans( String clName1, String clName2 ) {
            this.clName1 = clName1;
            this.clName2 = clName2;
            this.db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        }

        void interruptTrans(){
            db.close();
        }

        @Override
        public void exec() throws Exception {
            try {
                DBCollection cl1 = db.getCollectionSpace( csName ).getCollection( clName1 );
                DBCollection cl2 = db.getCollectionSpace( csName ).getCollection( clName2 );

                db.beginTransaction();
                setTransactionID( db );

                // phase 1: get lock
                cl1.update( "", modifier, "" );

                // sync all transaction
                count.decrementAndGet();
                synchronized ( syncObj ){
                    if ( count.get() > 0 ){
                        syncObj.wait();
                    }else {
                        syncObj.notifyAll();
                    }
                }

                // phase 2: trigger lock wait
                cl2.update( "", modifier, "" );

                db.commit();
            }catch (BaseException e){
                // use db.close() / forceSession() to unlock deadLock, so the following errors need to be ignore:
                // SDB_NETWORK
                // SDB_NETWORK_CLOSE
                if ( e.getErrorCode() != SDBError.SDB_NETWORK.getErrorCode() &&
                     e.getErrorCode() != SDBError.SDB_NETWORK_CLOSE.getErrorCode() &&
                     e.getErrorCode() != SDBError.SDB_APP_FORCED.getErrorCode() ){
                    throw e;
                }
            }finally {
                db.close();
            }
        }
    }

    class GetAndCheckSnap extends SdbThreadBase{
        private List<UpdateTrans> transList;

        GetAndCheckSnap( List<UpdateTrans> transList ){
            this.transList = transList;
        }

        @Override
        public void exec() throws Exception {
            // wait for all UpdateTrans
            synchronized ( syncObj ){
                if ( count.get() > 0 ){
                    syncObj.wait();
                }
            }

            try {
                List<String> transIDList = new ArrayList<>();
                for ( UpdateTrans t: transList ){
                    transIDList.add( t.getTransactionID() );
                }
                long sessionID;

                if ( dataNode != null ){
                    // cluster mode

                    // 1. check SDB_SNAP_TRANSDEADLOCK
                    checkAndGetSessionID( db, transIDList, true, 2 );
                    sessionID = checkAndGetSessionID( dataNode, transIDList, true, 2 );

                    // 2. use forceSession() to unlock deadlocks
                    dataNode.forceSession( sessionID );
                    // wait for the transaction rollback to complete
                    Thread.sleep( 1000 );

                    // 3. check again
                    checkAndGetSessionID( db, transIDList, false,  0 );
                    checkAndGetSessionID( dataNode, transIDList, false, 0 );
                }else {
                    // standalone mode

                    // 1. check SDB_SNAP_TRANSDEADLOCK
                    sessionID = checkAndGetSessionID( db, transIDList, true, 2 );

                    // 2. use forceSession() to unlock deadlocks
                    db.forceSession( sessionID );
                    // wait for the transaction rollback to complete
                    Thread.sleep( 1000 );

                    // 3. check again
                    checkAndGetSessionID( db, transIDList, false,  0 );
                }
            }finally {
                // kill transactions to unlock deadlocks
                for ( UpdateTrans trans: transList ) {
                    trans.interruptTrans();
                }
            }
        }

        private long checkAndGetSessionID( Sequoiadb db, List<String> transIDList, boolean hadDeadlocks,
                                           int transNum ) throws Exception{
            long sessionID = -1;
            int totalTime = 0;
            BSONObject matcher = new BasicBSONObject( "TransactionID", new BasicBSONObject( "$in", transIDList ) );

            // DeadLock generation takes times, so we need to query snapshots several times
            while ( totalTime <= TIMEOUT ){
                Thread.sleep( INTERVAL_TIME );
                totalTime += INTERVAL_TIME;

                try( DBCursor cursor = db.getSnapshot( Sequoiadb.SDB_SNAP_TRANSDEADLOCK, matcher,null,null ) ){
                    if ( !hadDeadlocks ){
                        Assert.assertFalse( cursor.hasNext() );
                        return sessionID;
                    }

                    DeadlocksBean lastDeadLock = null;
                    int count = 0;
                    int deadlockID = -1;
                    while ( cursor.hasNext() ){
                        DeadlocksBean deadlock = new DeadlocksBean( cursor.getNext() );
                        Assert.assertTrue( deadlock.check() );
                        if ( deadlockID == -1 ){
                            deadlockID = deadlock.getDeadlockID();
                        }else {
                            // make sure all deadlockIDs are the same
                            Assert.assertEquals( deadlock.getDeadlockID(), deadlockID );
                        }
                        if ( lastDeadLock == null ){
                            lastDeadLock = deadlock;
                            sessionID = deadlock.getSession();
                        }else {
                            // make sure the results are orderly
                            Assert.assertTrue( lastDeadLock.compareTo(deadlock) );
                        }
                        count++;
                    }
                    if ( count > 0 ){
                        Assert.assertEquals( count, transNum );
                        break;
                    }
                }
            }
            if ( totalTime > TIMEOUT ){
                Assert.fail( "check SDB_SNAP_TRANSDEADLOCK timeout!" );
            }
            return sessionID;
        }
    }
}
