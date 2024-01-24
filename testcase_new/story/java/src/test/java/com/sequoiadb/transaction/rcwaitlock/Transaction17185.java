package com.sequoiadb.transaction.rcwaitlock;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.concurrent.CountDownLatch;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @testcase seqDB-17185:并发读写不同的记录
 * @date 2019-1-23
 * @author yinzhen
 *
 */
@Test(groups = { "rcwaitlock", "rs" })
public class Transaction17185 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl17185";
    private DBCollection cl = null;
    private Sequoiadb db1 = null;
    private Sequoiadb db2 = null;
    private Sequoiadb db3 = null;
    private DBCollection cl1 = null;
    private DBCollection cl2 = null;
    private DBCollection cl3 = null;
    private CountDownLatch latch = null;
    private List< BSONObject > expList = new ArrayList<>();

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db3 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
    }

    @AfterClass
    public void tearDown() {
        if ( !db1.isClosed() ) {
            db1.close();
        }
        if ( !db2.isClosed() ) {
            db2.close();
        }
        if ( !db3.isClosed() ) {
            db3.close();
        }
        CollectionSpace cs = sdb.getCollectionSpace( csName );
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }
        if ( !sdb.isClosed() ) {
            sdb.close();
        }
    }

    @DataProvider(name = "index")
    public Object[][] createIndex() {
        return new Object[][] { { "{'a':-1}" }, { "{'a':1}" } };
    }

    @Test(dataProvider = "index")
    public void test( String indexKey ) {
        try {
            latch = new CountDownLatch( 7 );
            cl = sdb.getCollectionSpace( csName ).createCollection( clName );
            cl.createIndex( "textIndex17185", indexKey, false, false );
            insertData();

            // 事务1插入记录
            InsertThread insertThread = new InsertThread();
            insertThread.start();

            // 事务2更新记录
            UpdateThread updateThread = new UpdateThread();
            updateThread.start();

            // 事务3删除记录
            DeleteThread deleteThread = new DeleteThread();
            deleteThread.start();

            // 事务4读记录走索引扫描
            List< BSONObject > expRecords1 = getExpRecords();
            List< BSONObject > expRecords2 = new ArrayList<>( expRecords1 );
            Collections.reverse( expRecords2 );

            IdxScanThread positiveThread = new IdxScanThread( "{a:1}",
                    "{'':'textIndex17185'}", expRecords1 );
            positiveThread.start();

            IdxScanThread reverseThread = new IdxScanThread( "{a:-1}",
                    "{'':'textIndex17185'}", expRecords2 );
            reverseThread.start();

            // 事务5读记录走表扫描
            TableScanThread positiveThread2 = new TableScanThread( "{a:1}",
                    "{'':null}", expRecords1 );
            positiveThread2.start();

            TableScanThread reverseThread2 = new TableScanThread( "{a:-1}",
                    "{'':null}", expRecords2 );
            reverseThread2.start();

            Assert.assertTrue( insertThread.isSuccess(),
                    insertThread.getErrorMsg() );
            Assert.assertTrue( updateThread.isSuccess(),
                    updateThread.getErrorMsg() );
            Assert.assertTrue( deleteThread.isSuccess(),
                    deleteThread.getErrorMsg() );
            Assert.assertTrue( positiveThread.isSuccess(),
                    positiveThread.getErrorMsg() );
            Assert.assertTrue( reverseThread.isSuccess(),
                    reverseThread.getErrorMsg() );

            Assert.assertTrue( TransUtils.isTransWaitLock( sdb,
                    ( String ) positiveThread2.getExecResult() ) );
            Assert.assertTrue( TransUtils.isTransWaitLock( sdb,
                    ( String ) reverseThread2.getExecResult() ) );

            db1.commit();
            db2.commit();
            db3.commit();

            Assert.assertTrue( positiveThread2.isSuccess(),
                    positiveThread2.getErrorMsg() );
            Assert.assertTrue( reverseThread2.isSuccess(),
                    reverseThread2.getErrorMsg() );

            // 非事务表扫描
            DBCursor cursor = cl.query( "", "", "{_id:1}", "{'':null}" );
            List< BSONObject > actList = TransUtils.getReadActList( cursor );
            getExpList();
            Assert.assertEquals( actList, expList );

            // 非事务索引扫描
            cursor = cl.query( "", "", "{_id:1}", "{'':'textIndex17185'}" );
            actList = TransUtils.getReadActList( cursor );
            Assert.assertEquals( actList, expList );

            latch.await();
        } catch ( InterruptedException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            db1.commit();
            db2.commit();
            db3.commit();
            CollectionSpace cs = sdb.getCollectionSpace( csName );
            cs.dropCollection( clName );
        }
    }

    private void insertData() {
        List< BSONObject > records = new ArrayList<>();
        for ( int i = 1; i <= 400; i++ ) {
            BSONObject record = ( BSONObject ) JSON
                    .parse( "{_id:" + i + ",a:" + i + ", b:" + i + "}" );
            records.add( record );
        }
        Collections.shuffle( records );
        cl.insert( records );
    }

    private void getExpList() {
        List< BSONObject > records = new ArrayList<>();
        for ( int i = 1; i < 101; i++ ) {
            BSONObject record = ( BSONObject ) JSON.parse(
                    "{_id:" + i + ",a:" + ( i - 10 ) + ", b:" + i + "}" );
            records.add( record );
        }
        expList.clear();
        expList.addAll( records );
        records.clear();
        for ( int i = 201; i <= 500; i++ ) {
            BSONObject record = ( BSONObject ) JSON
                    .parse( "{_id:" + i + ",a:" + i + ", b:" + i + "}" );
            records.add( record );
        }
        expList.addAll( records );
    }

    class InsertThread extends SdbThreadBase {
        @Override
        public void exec() throws Exception {
            try {
                TransUtils.beginTransaction( db1 );
                cl1 = db1.getCollectionSpace( csName ).getCollection( clName );
                List< BSONObject > records = new ArrayList<>();
                for ( int i = 401; i <= 500; i++ ) {
                    BSONObject record = ( BSONObject ) JSON.parse(
                            "{_id:" + i + ",a:" + i + ", b:" + i + "}" );
                    records.add( record );
                }
                Collections.shuffle( records );
                cl1.insert( records );
            } catch ( Exception e ) {
                e.printStackTrace();
                throw e;
            } finally {
                latch.countDown();
            }
        }
    }

    class UpdateThread extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            TransUtils.beginTransaction( db2 );
            cl2 = db2.getCollectionSpace( csName ).getCollection( clName );
            cl2.update( "{$and:[{a:{$gt:0}},{a:{$lt:101}}]}", "{$inc:{a:-10}}",
                    "{'':'textIndex17185'}" );
            latch.countDown();
        }
    }

    class DeleteThread extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            TransUtils.beginTransaction( db3 );

            cl3 = db3.getCollectionSpace( csName ).getCollection( clName );
            cl3.delete( "{$and:[{a:{$gt:100}},{a:{$lt:201}}]}",
                    "{'':'textIndex17185'}" );
            latch.countDown();
        }
    }

    class IdxScanThread extends SdbThreadBase {
        private String sort;
        private String hint;
        private List< BSONObject > expRecords;

        private IdxScanThread( String sort, String hint,
                List< BSONObject > expRecords ) {
            super();
            this.sort = sort;
            this.hint = hint;
            this.expRecords = expRecords;
        }

        @Override
        public void exec() throws Exception {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                TransUtils.beginTransaction( db );
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                DBCursor cursor = cl.query(
                        "{$and:[{a:{$gt:200}},{a:{$lt:401}}]}", null, sort,
                        hint );
                List< BSONObject > records = TransUtils
                        .getReadActList( cursor );
                Assert.assertEquals( records, expRecords );
            } finally {
                latch.countDown();
                db.commit();
                db.close();
            }

        }
    }

    class TableScanThread extends SdbThreadBase {
        private String sort;
        private String hint;
        private List< BSONObject > expRecords;

        private TableScanThread( String sort, String hint,
                List< BSONObject > expRecords ) {
            super();
            this.sort = sort;
            this.hint = hint;
            this.expRecords = expRecords;
        }

        @Override
        public void exec() throws Exception {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                TransUtils.beginTransaction( db );
                // 判断事务阻塞需先获取事务id
                String transactionID2 = TransUtils.getTransactionID( db );
                setExecResult( transactionID2 );

                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                DBCursor cursor = cl.query(
                        "{$and:[{a:{$gt:200}},{a:{$lt:401}}]}", null, sort,
                        hint );
                List< BSONObject > records = null;
                records = TransUtils.getReadActList( cursor );
                Assert.assertEquals( records, expRecords );
            } finally {
                latch.countDown();
                db.commit();
                db.close();
            }

        }

    }

    private List< BSONObject > getExpRecords() {
        List< BSONObject > expRecords = new ArrayList<>();
        for ( int i = 201; i <= 400; i++ ) {
            BSONObject record = ( BSONObject ) JSON
                    .parse( "{_id:" + i + ", a:" + i + ", b:" + i + "}" );
            expRecords.add( record );
        }
        return expRecords;
    }
}
