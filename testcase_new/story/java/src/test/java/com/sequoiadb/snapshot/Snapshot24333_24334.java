package com.sequoiadb.snapshot;

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
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.*;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * @Description: seqDB-24333 : 在 data、coord 节点上检测复杂死锁 seqDB-24334 使用 forceSession() 解除复杂死锁
 * @Author Yang Qincheng
 * @Date 2021.09.03
 */
public class Snapshot24333_24334 extends SdbTestBase {
    private Sequoiadb db;
    private Sequoiadb dataNode1;
    private Sequoiadb dataNode2;
    private Sequoiadb dataNode3;
    private DBCollection cl;
    private final String clName = "cl_24334";
    private final String shardingKey = "a";
    private String groupName1;
    private String groupName2;
    private String groupName3;
    private final static AtomicInteger count = new AtomicInteger( 6 );
    private final static Object syncObj = new Object();
    private final static int TIMEOUT = 60 * 1000; // 1 min, the default value of transactiontimeout is 1 min
    private final static int INTERVAL_TIME = 200;  // 200 ms

    @BeforeClass
    public void setUp(){
        db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( db ) ) {
            throw new SkipException( "skip StandAlone" );
        }
        ArrayList< String > groupList =  CommLib.getDataGroupNames( db );
        if ( groupList.size() < 3 ){
            Assert.fail( "At least three data groups are required" );
        }

        int num = 0;
        groupName1 = groupList.get( num++ );
        groupName2 = groupList.get( num++ );
        groupName3 = groupList.get( num );
        BSONObject option = new BasicBSONObject();
        option.put( "Group", groupName1 );
        option.put( "ShardingKey", new BasicBSONObject( shardingKey, 1 ) );
        option.put( "ShardingType", "range" );
        cl = db.getCollectionSpace( csName ).createCollection( clName, option );

        String nodeName1 = db.getReplicaGroup( groupName1 ).getMaster().getNodeName();
        String nodeName2 = db.getReplicaGroup( groupName2 ).getMaster().getNodeName();
        String nodeName3 = db.getReplicaGroup( groupName3 ).getMaster().getNodeName();
        dataNode1 = new Sequoiadb( nodeName1, "", "" );
        dataNode2 = new Sequoiadb( nodeName2, "", "" );
        dataNode3 = new Sequoiadb( nodeName3, "", "" );
    }

    @AfterClass
    public void tearDown(){
        try {
            db.getCollectionSpace( csName ).dropCollection( clName );
        }finally {
            db.close();
            dataNode1.close();
            dataNode2.close();
            dataNode3.close();
        }
    }

    @Test(enabled = false)
    public void test(){
        // prepare data
        for ( int i = 1; i < 7; i++ ){
            cl.insert( new BasicBSONObject( shardingKey, i ) );
        }
        // groupName1: {a: 1},{a: 2}
        // groupName2: {a: 3},{a: 4}
        // groupName3: {a: 5},{a: 6}
        cl.split( groupName1, groupName2, new BasicBSONObject(shardingKey, 3), new BasicBSONObject( shardingKey, 5 ) );
        cl.split( groupName1, groupName3, new BasicBSONObject(shardingKey, 5), new BasicBSONObject( shardingKey, 7 ) );

        BSONObject modifier = new BasicBSONObject( "$set", new BasicBSONObject( "b", "B" ) );
        BSONObject modifierLarger = new BasicBSONObject( "$set", new BasicBSONObject( "b", "BBBBBBBBBBBBBB" ) );
        // matcherB is used to construct the wait relationship between transactions
        int[] matcherB1 = { 2, 5 }; // t1 wait for t2,t4
        int[] matcherB2 = { 3, 6 }; // t2 wait for t3,t5
        int[] matcherB3 = { 4 };    // t3 wait for t6
        int[] matcherB4 = { 6 };    // t4 wait for t5
        int[] matcherB5 = { 4, 5 }; // t5 wait for t4,t6
        int[] matcherB6 = { 2 };    // t6 wait for t2

        UpdateTrans trans1 = new UpdateTrans( 1, matcherB1, modifier );
        // the amount of data for t2 updates needs to be larger than other transactions
        UpdateTrans trans2 = new UpdateTrans( 2, matcherB2, modifierLarger );
        UpdateTrans trans3 = new UpdateTrans( 3, matcherB3, modifier );
        UpdateTrans trans4 = new UpdateTrans( 5, matcherB4, modifier );
        UpdateTrans trans5 = new UpdateTrans( 6, matcherB5, modifier );
        UpdateTrans trans6 = new UpdateTrans( 4, matcherB6, modifier );
        List<UpdateTrans> transList = new ArrayList<>();
        transList.add( trans1 );
        transList.add( trans2 );
        transList.add( trans3 );
        transList.add( trans4 );
        transList.add( trans5 );
        transList.add( trans6 );
        GetAndCheckSnap snap = new GetAndCheckSnap( transList );

        trans1.start();
        trans2.start();
        trans3.start();
        trans4.start();
        trans5.start();
        trans6.start();
        snap.start();

        Assert.assertTrue( trans1.isSuccess(),  trans1.getErrorMsg() );
        Assert.assertTrue( trans2.isSuccess(),  trans2.getErrorMsg() );
        Assert.assertTrue( trans3.isSuccess(),  trans3.getErrorMsg() );
        Assert.assertTrue( trans4.isSuccess(),  trans1.getErrorMsg() );
        Assert.assertTrue( trans5.isSuccess(),  trans2.getErrorMsg() );
        Assert.assertTrue( trans6.isSuccess(),  trans3.getErrorMsg() );
        Assert.assertTrue( snap.isSuccess(),  snap.getErrorMsg() );
    }

    class UpdateTrans extends SdbThreadBase {
        private BSONObject matcherA;
        private BSONObject matcherB;
        private BSONObject modifier;
        private Sequoiadb db;

        UpdateTrans( int matcherA, int[] matcherB, BSONObject modifier ) {
            this.matcherA = new BasicBSONObject( shardingKey, new BasicBSONObject( "$et", matcherA ) );
            this.matcherB = new BasicBSONObject( shardingKey, new BasicBSONObject( "$in", matcherB ) );
            this.modifier = modifier;
            this.db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        }

        void interruptTrans(){
            db.close();
        }

        @Override
        public void exec() throws Exception {
            try {
                DBCollection cl = db.getCollectionSpace( csName ).getCollection( clName );
                db.beginTransaction();
                setTransactionID( db );

                // phase 1: get lock
                cl.update( matcherA, modifier, null );

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
                cl.update( matcherB, modifier, null );

                db.commit();
            }catch ( BaseException e ){
                // use db.close() / forceSession() to unlock deadLock, so the following errors need to be ignored:
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

        GetAndCheckSnap(List<UpdateTrans> transList){
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

                // 1. get and check SDB_SNAP_TRANSDEADLOCK
                long sessionID1 = checkAndGetSessionID( db, transIDList,true, 5 );
                checkAndGetSessionID( dataNode1, transIDList, false, 0 );
                checkAndGetSessionID( dataNode2, transIDList, false, 0 );
                checkAndGetSessionID( dataNode3, transIDList, true, 2 );

                // 2. use forceSession() unlock deadlocks
                db.forceSession( sessionID1 );
                // wait for the transaction rollback to complete
                Thread.sleep( 1000 );

                // 3. get and check SDB_SNAP_TRANSDEADLOCK
                long sessionID2 = checkAndGetSessionID( db, transIDList, true, 3 );

                // 4. use forceSession() unlock deadlocks
                db.forceSession( sessionID2 );
                Thread.sleep( 1000 );

                // 5. get and check SDB_SNAP_TRANSDEADLOCK
                checkAndGetSessionID( db, transIDList, false, 0 );
            }finally {
                // kill transactions to unlock deadlocks
                for ( UpdateTrans trans: transList ) {
                    trans.interruptTrans();
                }
            }
        }

        private long checkAndGetSessionID( Sequoiadb db, List<String> transIDList, boolean hadDeadlocks,
                                           int transNum ) throws Exception {
            int totalTime = 0;
            long sessionID = -1;
            BSONObject matcher = new BasicBSONObject( "TransactionID", new BasicBSONObject( "$in", transIDList ) );

            // DeadLock generation takes times, so we need to query snapshots several times
            while ( totalTime <= TIMEOUT ){
                Thread.sleep( INTERVAL_TIME );
                totalTime += INTERVAL_TIME;

                try ( DBCursor cursor = db.getSnapshot( Sequoiadb.SDB_SNAP_TRANSDEADLOCK, matcher,null,null ) ){
                    if ( !hadDeadlocks ){
                        Assert.assertFalse( cursor.hasNext() );
                        return sessionID;
                    }

                    if ( !cursor.hasNext() ){
                        continue;
                    }

                    DeadlocksBean lastDeadLock = null;
                    List<DeadlocksBean> deadlockList = new ArrayList<>();
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
                        }else {
                            // make sure the results are orderly
                            Assert.assertTrue( lastDeadLock.compareTo( deadlock ) );
                        }
                        deadlockList.add( deadlock );
                    }
                    Assert.assertEquals( deadlockList.size(), transNum );
                    sessionID = deadlockList.get( 0 ).getSession();
                    break;
                }
            }
            if ( totalTime > TIMEOUT ){
                Assert.fail( "check SDB_SNAP_TRANSDEADLOCK timeout!" );
            }
            return sessionID;
        }
    }
}
