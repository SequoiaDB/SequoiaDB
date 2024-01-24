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
import java.util.Arrays;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * @Description: seqDB-24326:在存在死锁的 data、coord 节点上查询事务锁等待信息
 * @Author Yang Qincheng
 * @Date 2021.08.27
 */
public class Snapshot24326 extends SdbTestBase {
    private Sequoiadb db;
    private Sequoiadb dataNode = null;
    private CollectionSpace cs;
    private final String clName1 = "cl_24326_A";
    private final String clName2 = "cl_24326_B";
    private final String clName3 = "cl_24326_C";
    private final static AtomicInteger count = new AtomicInteger( 3 );
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
        GetAndCheckSnap snap = new GetAndCheckSnap( trans1, trans2, trans3 );

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
                // use db.close() to unlock deadLock, so the SDB_NETWORK errors should be ignored
                if ( e.getErrorCode() != SDBError.SDB_NETWORK.getErrorCode() ){
                    throw e;
                }
            }finally {
                db.close();
            }
        }
    }

    class GetAndCheckSnap extends SdbThreadBase{
        UpdateTrans trans1;
        UpdateTrans trans2;
        UpdateTrans trans3;

        GetAndCheckSnap( UpdateTrans trans1, UpdateTrans trans2, UpdateTrans trans3 ){
            this.trans1 = trans1;
            this.trans2 = trans2;
            this.trans3 = trans3;
        }

        @Override
        public void exec() throws Exception {
            // wait for all UpdateTrans
            synchronized ( syncObj ){
                if ( count.get() > 0 ){
                    syncObj.wait();
                }
            }

            try{
                checkSnapshot( db, 3, trans1, trans2, trans3 );

                // dataNode is null means standalone mode
                if ( dataNode != null ){
                    // if cluster mode, we should check data node
                    checkSnapshot(dataNode, 3, trans1, trans2, trans3);
                }
            }finally {
                // kill a transaction to unlock deadlocks
                trans2.interruptTrans();
            }
        }

        private void checkSnapshot( Sequoiadb db, int expectSize, UpdateTrans trans1, UpdateTrans trans2,
                                  UpdateTrans trans3 ) throws Exception{
            BSONObject result = null;
            int totalTime = 0;

            // DeadLock generation takes times, so we need to query snapshots several times
            while ( totalTime <= TIMEOUT ){
                Thread.sleep( INTERVAL_TIME );
                totalTime += INTERVAL_TIME;

                try( DBCursor cursor = db.getSnapshot( Sequoiadb.SDB_SNAP_TRANSWAITS, "","","" ) ){
                    if ( !cursor.hasNext() ){
                        continue;
                    }

                    int size = 0;
                    while (cursor.hasNext()){
                        result = cursor.getNext();
                        String waiterTransID = (String)result.get("WaiterTransID");
                        String holderTransID = (String)result.get("HolderTransID");
                        // check value
                        if ( waiterTransID.equals( trans1.getTransactionID() )){
                            Assert.assertEquals(holderTransID, trans2.getTransactionID());
                        }
                        if ( waiterTransID.equals( trans2.getTransactionID() )){
                            Assert.assertEquals(holderTransID, trans3.getTransactionID());
                        }
                        if ( waiterTransID.equals( trans3.getTransactionID() )){
                            Assert.assertEquals(holderTransID, trans2.getTransactionID());
                        }
                        size++;
                    }
                    // check size
                    Assert.assertEquals(size, expectSize);
                    // check fields of SDB_SNAP_TRANSWAITS
                    Assert.assertNotNull(result.get("NodeName"));
                    Assert.assertNotNull(result.get("GroupID"));
                    Assert.assertNotNull(result.get("NodeID"));
                    Assert.assertNotNull(result.get("WaitTime"));
                    Assert.assertNotNull(result.get("WaiterTransID"));
                    Assert.assertNotNull(result.get("HolderTransID"));
                    Assert.assertNotNull(result.get("WaiterTransCost"));
                    Assert.assertNotNull(result.get("HolderTransCost"));
                    Assert.assertNotNull(result.get("WaiterSessionID"));
                    Assert.assertNotNull(result.get("HolderSessionID"));
                    Assert.assertNotNull(result.get("WaiterRelatedID"));
                    Assert.assertNotNull(result.get("HolderRelatedID"));
                    Assert.assertNotNull(result.get("WaiterRelatedSessionID"));
                    Assert.assertNotNull(result.get("HolderRelatedSessionID"));
                    Assert.assertNotNull(result.get("WaiterRelatedGroupID"));
                    Assert.assertNotNull(result.get("HolderRelatedGroupID"));
                    Assert.assertNotNull(result.get("WaiterRelatedNodeID"));
                    Assert.assertNotNull(result.get("HolderRelatedNodeID"));
                    break;
                }
            }
            if ( totalTime > TIMEOUT ){
                Assert.fail( "check SDB_SNAP_TRANSWAITS timeout!" );
            }
        }
    }
}