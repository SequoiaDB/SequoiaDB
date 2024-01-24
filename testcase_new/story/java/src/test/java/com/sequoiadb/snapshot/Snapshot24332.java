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
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * @Description: seqDB-24332:通过事务超时回滚解除死锁
 * @Author Yang Qincheng
 * @Date 2021.09.03
 */
public class Snapshot24332 extends SdbTestBase {
    private Sequoiadb db;
    private CollectionSpace cs;
    private final String clName1 = "cl_24332_A";
    private final String clName2 = "cl_24332_B";
    private final String clName3 = "cl_24332_C";
    private final static AtomicInteger count = new AtomicInteger( 3 );
    private final static AtomicBoolean isTimeout = new AtomicBoolean( false );
    private final static Object syncObj = new Object();
    private final static int TRANS_TIMEOUT_1 = 10; // 10 s
    private final static int TRANS_TIMEOUT_2 = 5;  // 5 s
    private final static int TIMEOUT = TRANS_TIMEOUT_1 * 1000;  // 10 s
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
        }
    }

    @Test
    public void test(){
        UpdateTrans trans1 = new UpdateTrans( clName1, clName2, TRANS_TIMEOUT_1 );
        UpdateTrans trans2 = new UpdateTrans( clName2, clName3, TRANS_TIMEOUT_2 );
        UpdateTrans trans3 = new UpdateTrans( clName3, clName2, TRANS_TIMEOUT_1 );
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

    class UpdateTrans extends SdbThreadBase {
        private String clName1;
        private String clName2;
        private Sequoiadb db;
        private String modifier = "{$set: {a: 2}}";

        UpdateTrans( String clName1, String clName2, int transTimeOut ) {
            this.clName1 = clName1;
            this.clName2 = clName2;
            this.db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            db.setSessionAttr( new BasicBSONObject( "TransTimeout", transTimeOut ) );
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
            }catch ( BaseException e ){
                if ( e.getErrorCode() == SDBError.SDB_TIMEOUT.getErrorCode() ){
                    isTimeout.set( true );
                }else if ( e.getErrorCode() != SDBError.SDB_NETWORK.getErrorCode() ){
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

            List<String> transIDList = new ArrayList<>();
            for ( UpdateTrans t: transList ){
                transIDList.add( t.getTransactionID() );
            }

            // 1. get and check SDB_SNAP_TRANSDEADLOCK
            checkResult( db, transIDList, true, 2 );

            // 2. wait for transactions timeout to rollback
            int totalTime = 0;
            while ( !isTimeout.get() && totalTime <= TIMEOUT ){
                totalTime +=INTERVAL_TIME;
                Thread.sleep( INTERVAL_TIME );
            }
            if ( totalTime > TIMEOUT ){
                Assert.fail( "Transaction did not time out for a long time!" );
            }

            // 3. get and check SDB_SNAP_TRANSDEADLOCK
            checkResult( db, transIDList, false, 0 );
        }

        private void checkResult( Sequoiadb db, List<String> transIDList, boolean hadDeadlocks, int transNum ) throws Exception{
            int totalTime = 0;
            BSONObject matcher = new BasicBSONObject( "TransactionID", new BasicBSONObject( "$in", transIDList ) );

            while ( totalTime <= TIMEOUT ){
                Thread.sleep( INTERVAL_TIME );
                totalTime += INTERVAL_TIME;

                try( DBCursor cursor = db.getSnapshot( Sequoiadb.SDB_SNAP_TRANSDEADLOCK, matcher,null,null ) ){
                    if ( !hadDeadlocks ){
                        Assert.assertFalse(cursor.hasNext());
                        break;
                    }

                    if ( !cursor.hasNext() ){
                        continue;
                    }
                    int deadlockID = -1;
                    int count = 0;
                    while (cursor.hasNext()){
                        DeadlocksBean deadlock = new DeadlocksBean( cursor.getNext() );
                        Assert.assertTrue( deadlock.check() );
                        if (deadlockID == -1){
                            deadlockID = deadlock.getDeadlockID();
                        }else {
                            Assert.assertEquals( deadlock.getDeadlockID(), deadlockID);
                        }
                        count++;
                    }
                    Assert.assertEquals(count, transNum);
                    break;
                }

            }
            if ( totalTime > TIMEOUT ){
                Assert.fail( "check SDB_SNAP_TRANSWAITS timeout!" );
            }
        }
    }
}
