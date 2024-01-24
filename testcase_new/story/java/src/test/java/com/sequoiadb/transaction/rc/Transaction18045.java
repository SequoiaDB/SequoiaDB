package com.sequoiadb.transaction.rc;

import java.util.ArrayList;
import java.util.Arrays;
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
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @testcase seqDB-18045:并发事务中读写不同的记录，使用复合索引查询
 * @date 2019-3-26
 * @author yinzhen
 *
 */
@Test(groups = "rc")
public class Transaction18045 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl18045";
    private DBCollection cl = null;
    private Sequoiadb db1 = null;
    private Sequoiadb db2 = null;
    private Sequoiadb db3 = null;
    private Sequoiadb db4 = null;
    private Sequoiadb db5 = null;
    private DBCollection cl1 = null;
    private DBCollection cl2 = null;
    private DBCollection cl3 = null;
    private DBCollection cl4 = null;
    private DBCollection cl5 = null;
    private CountDownLatch latch = null;
    private List< BSONObject > expList = new ArrayList<>();

    @BeforeClass
    public void setUp() {
        sdb = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        db1 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        db2 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        db3 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        db4 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        db5 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
    }

    @AfterClass
    public void tearDown() {
        db1.commit();
        db2.commit();
        db3.commit();
        db4.commit();
        db5.commit();
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
        CollectionSpace cs = sdb.getCollectionSpace( csName );
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }
        if ( !sdb.isClosed() ) {
            sdb.close();
        }
    }

    @DataProvider(name = "index")
    private Object[][] createIndex() {
        return new Object[][] { { "{'a':1, 'b':1}" }, { "{'a':1, 'b':-1}" },
                { "{'a':-1, 'b':1}" }, { "{'a':-1, 'b':-1}" } };
    }

    @Test(dataProvider = "index")
    public void test( String indexKey ) {
        try {
            latch = new CountDownLatch( 5 );
            cl = sdb.getCollectionSpace( csName ).createCollection( clName );
            cl.createIndex( "textIndex18045", indexKey, false, false );
            insertData();

            // 开启并发事务
            cl1 = db1.getCollectionSpace( csName ).getCollection( clName );
            cl2 = db2.getCollectionSpace( csName ).getCollection( clName );
            cl3 = db3.getCollectionSpace( csName ).getCollection( clName );
            cl4 = db4.getCollectionSpace( csName ).getCollection( clName );
            cl5 = db5.getCollectionSpace( csName ).getCollection( clName );
            TransUtils.beginTransaction( db1 );
            TransUtils.beginTransaction( db2 );
            TransUtils.beginTransaction( db3 );
            TransUtils.beginTransaction( db4 );
            TransUtils.beginTransaction( db5 );

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
            QueryThread queryThread = new QueryThread();
            queryThread.start();

            // 事务5读记录走表扫描
            QueryThread2 queryThread2 = new QueryThread2();
            queryThread2.start();

            Assert.assertTrue( insertThread.isSuccess() );
            Assert.assertTrue( updateThread.isSuccess() );
            Assert.assertTrue( deleteThread.isSuccess() );
            Assert.assertTrue( queryThread.isSuccess() );
            Assert.assertTrue( queryThread2.isSuccess() );

            // 提交事务
            db1.commit();
            db2.commit();
            db3.commit();
            db4.commit();
            db5.commit();

            // 非事务表扫描
            expList = getExpList();
            TransUtils.queryAndCheck( cl,
                    "{$and:[{a:{$in:" + Arrays.toString( getAllRandArray() )
                            + "}},{b:{$in:"
                            + Arrays.toString( getAllRandArray() ) + "}}]}",
                    "", "{a:1, b:-1, _id:1}", "{'':null}", expList );

            // 非事务索引扫描
            TransUtils.queryAndCheck( cl,
                    "{$and:[{a:{$in:" + Arrays.toString( getAllRandArray() )
                            + "}},{b:{$in:"
                            + Arrays.toString( getAllRandArray() ) + "}}]}",
                    "", "{a:1, b:-1, _id:1}", "{'':'textIndex18045'}",
                    expList );

            latch.await();
        } catch ( InterruptedException e ) {
            e.printStackTrace();
        } finally {
            tearDownCommit();
            CollectionSpace cs = sdb.getCollectionSpace( csName );
            cs.dropCollection( clName );
        }
    }

    private void tearDownCommit() {
        db1.commit();
        db2.commit();
        db3.commit();
        db4.commit();
        db5.commit();
    }

    // 构造记录 a 字段相等 b 字段不相等，a b 字段都不相等的记录
    private void insertData() {
        int a = 0;
        List< BSONObject > records = new ArrayList< BSONObject >();
        for ( int i = 0; i <= 40000; i++ ) {
            if ( i < 20000 ) {
                a = 100;
            } else {
                a = i;
            }
            BSONObject object = ( BSONObject ) JSON.parse(
                    "{_id:" + i + ", a:" + a + ", b:" + ( 40000 - i ) + "}" );
            records.add( object );
        }
        expList.clear();
        expList.addAll( records );
        Collections.shuffle( records );
        cl.insert( records );
    }

    private Integer[] getAllRandArray() {
        List< Integer > randList = new ArrayList<>();
        for ( int i = 0; i <= 50000; i++ ) {
            randList.add( i );
        }
        Collections.shuffle( randList );
        Integer[] rangArray = randList
                .toArray( new Integer[ randList.size() ] );
        return rangArray;
    }

    private List< BSONObject > getExpList() {
        List< BSONObject > records = new ArrayList<>();
        int a = 0;
        int b = 0;
        for ( int i = 0; i < 40001; i++ ) {
            if ( i < 10000 ) {
                a = 100;
                b = 40000 - i;
            } else if ( i >= 10000 && i < 20000 ) {
                continue;
            } else if ( i >= 20000 && i < 30000 ) {
                a = i + 10;
                b = 40000 - i - 10;
            } else {
                a = i;
                b = 40000 - i;
            }
            String str = "{_id:" + i + ", a:" + a + ", b:" + b + "}";
            BSONObject object = ( BSONObject ) JSON.parse( str );
            records.add( object );
        }
        for ( int i = 40001; i <= 50000; i++ ) {
            BSONObject record = ( BSONObject ) JSON
                    .parse( "{_id:" + i + ", a:" + i + ", b:" + i + "}" );
            records.add( record );
        }
        return records;
    }

    class InsertThread extends SdbThreadBase {
        @Override
        public void exec() throws Exception {
            try {
                List< BSONObject > records = new ArrayList<>();
                for ( int i = 40001; i <= 50000; i++ ) {
                    BSONObject record = ( BSONObject ) JSON.parse(
                            "{_id:" + i + ", a:" + i + ", b:" + i + "}" );
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
            cl2.update( "{$and:[{b:{$gt:10000}},{b:{$lt:20001}}]}",
                    "{$inc:{a:10, b:-10}}", "{'':'textIndex18045'}" );
            latch.countDown();
        }
    }

    class DeleteThread extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            cl3.delete( "{$and:[{b:{$gt:20000}},{b:{$lt:30001}}]}",
                    "{'':'textIndex18045'}" );
            latch.countDown();
        }
    }

    private Integer[] getRandomArray() {
        List< Integer > randList = new ArrayList<>();
        for ( int i = 0; i <= 40000; i++ ) {
            randList.add( i );
        }
        Collections.shuffle( randList );
        Integer[] randArray = randList
                .toArray( new Integer[ randList.size() ] );
        return randArray;
    }

    class QueryThread extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            Integer[] randArray = getRandomArray();
            TransUtils.queryAndCheck( cl4,
                    "{$and:[{a:{$in:" + Arrays.toString( randArray )
                            + "}},{b:{$in:" + Arrays.toString( randArray )
                            + "}}]}",
                    null, "{a:1, b:-1}", "{'':'textIndex18045'}", expList );
            latch.countDown();
        }
    }

    class QueryThread2 extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            Integer[] randArray = getRandomArray();
            TransUtils.queryAndCheck( cl5,
                    "{$and:[{a:{$in:" + Arrays.toString( randArray )
                            + "}},{b:{$in:" + Arrays.toString( randArray )
                            + "}}]}",
                    null, "{a:1, b:-1}", "{'':null}", expList );
            latch.countDown();
        }
    }
}
