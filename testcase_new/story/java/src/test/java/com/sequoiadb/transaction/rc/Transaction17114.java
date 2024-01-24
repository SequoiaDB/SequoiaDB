package com.sequoiadb.transaction.rc;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.concurrent.CountDownLatch;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @testcase seqDB-17114:并发读写不同的记录
 * @date 2019-1-21
 * @author yinzhen
 *
 */
@Test(groups = { "rc" })
public class Transaction17114 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private DBCollection cl = null;
    private Sequoiadb db1 = null;
    private Sequoiadb db2 = null;
    private Sequoiadb db3 = null;
    private Sequoiadb db4 = null;
    private Sequoiadb db5 = null;
    private Sequoiadb db6 = null;
    private Sequoiadb db7 = null;
    private DBCollection cl1 = null;
    private DBCollection cl2 = null;
    private DBCollection cl3 = null;
    private DBCollection cl4 = null;
    private DBCollection cl5 = null;
    private DBCollection cl6 = null;
    private DBCollection cl7 = null;
    private CountDownLatch latch = null;
    private List< BSONObject > expList = new ArrayList<>();
    private String hashCLName = "cl17114_hash";
    private String mainCLName = "cl17114_main";
    private String subCLName1 = "subcl17114_1";
    private String subCLName2 = "subcl17114_2";

    @BeforeClass
    public void setUp() {
        sdb = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        db1 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        db2 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        db3 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        db4 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        db5 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        db6 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        db7 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "STANDALONE MODE" );
        }
        if ( CommLib.OneGroupMode( sdb ) ) {
            throw new SkipException( "ONE GROUP MODE" );
        }

        TransUtils.createCLs( sdb, csName, hashCLName, mainCLName, subCLName1,
                subCLName2, 250 );
    }

    @AfterClass
    public void tearDown() {
        db1.commit();
        db2.commit();
        db3.commit();
        db4.commit();
        db5.commit();
        db6.commit();
        db7.commit();

        if ( !db1.isClosed() ) {
            db1.close();
        }
        if ( !db2.isClosed() ) {
            db2.close();
        }
        if ( !db3.isClosed() ) {
            db3.close();
        }
        if ( !db4.isClosed() ) {
            db4.close();
        }
        if ( !db5.isClosed() ) {
            db5.close();
        }
        if ( !db6.isClosed() ) {
            db6.close();
        }
        if ( !db7.isClosed() ) {
            db7.close();
        }
        CollectionSpace cs = sdb.getCollectionSpace( csName );
        if ( cs.isCollectionExist( hashCLName ) ) {
            cs.dropCollection( hashCLName );
        }
        if ( cs.isCollectionExist( mainCLName ) ) {
            cs.dropCollection( mainCLName );
        }
        if ( sdb != null ) {
            sdb.close();
        }
    }

    @DataProvider(name = "index")
    public Object[][] createIndex() {
        return new Object[][] { { "{'a':-1}", hashCLName },
                { "{'a':1}", mainCLName } };
    }

    @Test(dataProvider = "index")
    public void test( String indexKey, String clName ) {
        try {
            latch = new CountDownLatch( 7 );
            cl = sdb.getCollectionSpace( csName ).getCollection( clName );
            cl.createIndex( "textIndex17114", indexKey, false, false );
            insertData();

            // 开启并发事务
            cl1 = db1.getCollectionSpace( csName ).getCollection( clName );
            cl2 = db2.getCollectionSpace( csName ).getCollection( clName );
            cl3 = db3.getCollectionSpace( csName ).getCollection( clName );
            cl4 = db4.getCollectionSpace( csName ).getCollection( clName );
            cl5 = db5.getCollectionSpace( csName ).getCollection( clName );
            cl6 = db6.getCollectionSpace( csName ).getCollection( clName );
            cl7 = db7.getCollectionSpace( csName ).getCollection( clName );
            TransUtils.beginTransaction( db1 );
            TransUtils.beginTransaction( db2 );
            TransUtils.beginTransaction( db3 );
            TransUtils.beginTransaction( db4 );
            TransUtils.beginTransaction( db5 );
            TransUtils.beginTransaction( db6 );
            TransUtils.beginTransaction( db7 );

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
            QueryThread positiveThread = new QueryThread( cl4, "{a:1}",
                    "{'':'textIndex17114'}", expList );
            positiveThread.start();

            List< BSONObject > expRecords = new ArrayList<>( expList );
            Collections.reverse( expRecords );

            QueryThread reverseThread = new QueryThread( cl5, "{a:-1}",
                    "{'':'textIndex17114'}", expRecords );
            reverseThread.start();

            // 事务5读记录走表扫描
            QueryThread positiveThread2 = new QueryThread( cl6, "{a:1}",
                    "{'':null}", expList );
            positiveThread2.start();

            QueryThread reverseThread2 = new QueryThread( cl7, "{a:-1}",
                    "{'':null}", expRecords );
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
            Assert.assertTrue( positiveThread2.isSuccess(),
                    positiveThread2.getErrorMsg() );
            Assert.assertTrue( reverseThread2.isSuccess(),
                    reverseThread2.getErrorMsg() );

            // 提交事务
            db1.commit();
            db2.commit();
            db3.commit();
            db4.commit();
            db5.commit();
            db6.commit();
            db7.commit();

            // 非事务表扫描
            getExpList();
            TransUtils.queryAndCheck( cl, "{_id:1}", "{'':null}", expList );

            // 非事务索引扫描
            TransUtils.queryAndCheck( cl, "{_id:1}", "{'':'textIndex17114'}",
                    expList );
            latch.await();
        } catch ( InterruptedException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            db1.commit();
            db2.commit();
            db3.commit();
            db4.commit();
            db5.commit();
            db6.commit();
            db7.commit();
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
        expList.clear();
        expList.addAll( records );
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
            cl2.update( "{$and:[{a:{$gt:0}},{a:{$lt:101}}]}", "{$inc:{a:-10}}",
                    "{'':'textIndex17114'}" );
            latch.countDown();
        }
    }

    class DeleteThread extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            cl3.delete( "{$and:[{a:{$gt:100}},{a:{$lt:201}}]}",
                    "{'':'textIndex17114'}" );
            latch.countDown();
        }
    }

    class QueryThread extends SdbThreadBase {
        private String sort;
        private String hint;
        private DBCollection cl;
        private List< BSONObject > expRecords;

        private QueryThread( DBCollection cl, String sort, String hint,
                List< BSONObject > expRecords ) {
            super();
            this.cl = cl;
            this.sort = sort;
            this.hint = hint;
            this.expRecords = expRecords;
        }

        @Override
        public void exec() throws Exception {
            TransUtils.queryAndCheck( cl, sort, hint, expRecords );
            latch.countDown();
        }
    }
}
