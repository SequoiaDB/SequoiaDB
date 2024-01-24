package com.sequoiadb.transaction.rc;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

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
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @testcase seqDB-18046:并发事务中读写记录存在交集，使用复合索引查询
 * @date 2019-3-26
 * @author yinzhen
 *
 */
@Test(groups = "rc")
public class Transaction18046 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl18046";
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
            cl = sdb.getCollectionSpace( csName ).createCollection( clName );
            cl.createIndex( "textIndex18046", indexKey, false, false );
            expList = TransUtils.getCompositeRecords( 0, 8000, 0, 10 );
            cl.insert( expList );
            TransUtils.sortCompositeRecords( expList, true );

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
            QueryThread queryThread = new QueryThread( cl4,
                    "{'':'textIndex18046'}" );
            queryThread.start();

            // 事务5读记录走表扫描
            QueryThread queryThread2 = new QueryThread( cl5, "{'':null}" );
            queryThread2.start();

            // 先判断表扫描和索引扫描记录
            Assert.assertTrue( queryThread.isSuccess(),
                    queryThread.getErrorMsg() );
            Assert.assertTrue( queryThread2.isSuccess(),
                    queryThread2.getErrorMsg() );
            db4.commit();
            db5.commit();

            // 提交插入事务
            Assert.assertTrue( insertThread.isSuccess(),
                    insertThread.getErrorMsg() );

            db1.commit();
            Assert.assertTrue( updateThread.isSuccess(),
                    updateThread.getErrorMsg() );

            db2.commit();
            Assert.assertTrue( deleteThread.isSuccess(),
                    deleteThread.getErrorMsg() );

            // 提交事务
            db3.commit();

            // 非事务表扫描
            List< BSONObject > tbScanActList = TransUtils.queryToBSONList( cl,
                    "{$and:[{a:{$in:" + Arrays.toString( getAllRandArray() )
                            + "}},{b:{$in:"
                            + Arrays.toString( getAllRandArray() ) + "}}]}",
                    "", "{a:1, b:-1, _id:1}", "{'':null}" );

            // 非事务索引扫描
            List< BSONObject > ixScanActList = TransUtils.queryToBSONList( cl,
                    "{$and:[{a:{$in:" + Arrays.toString( getAllRandArray() )
                            + "}},{b:{$in:"
                            + Arrays.toString( getAllRandArray() ) + "}}]}",
                    "", "{a:1, b:-1, _id:1}", "{'':'textIndex18046'}" );
            Assert.assertEquals( tbScanActList, ixScanActList );

        } catch ( BaseException e ) {
            throw e;
        } finally {
            db1.commit();
            db2.commit();
            db3.commit();
            db4.commit();
            db5.commit();
            CollectionSpace cs = sdb.getCollectionSpace( csName );
            cs.dropCollection( clName );

        }
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

    // 事务1插入记录,插入记录的索引值与事务2中更新后的值存在重复
    class InsertThread extends SdbThreadBase {
        @Override
        public void exec() throws Exception {
            try {
                List< BSONObject > records = new ArrayList<>();
                for ( int i = 44001; i <= 54000; i++ ) {
                    BSONObject record = ( BSONObject ) JSON.parse(
                            "{_id:" + i + ", a:" + i + ", b:" + i + "}" );
                    records.add( record );
                }
                Collections.shuffle( records );
                cl1.insert( records );
            } catch ( Exception e ) {
                e.printStackTrace();
                throw e;
            }
        }
    }

    // 事务2更新记录，更新匹配到集合中原有的记录及事务1中插入的记录
    class UpdateThread extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            try {
                cl2.update( "{$and:[{b:{$gt:39000}},{b:{$lt:49001}}]}",
                        "{$inc:{a:10, b:10}}", "{'':'textIndex18046'}" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -13 ) {
                    e.printStackTrace();
                    throw e;
                }
            }
        }
    }

    // 事务3删除记录，删除的记录匹配集合中原有记录，事务1中插入的记录，事务2中更新的记录
    class DeleteThread extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            try {
                cl3.delete( "{$and:[{b:{$gt:36000}},{b:{$lt:46001}}]}",
                        "{'':'textIndex18046'}" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -13 ) {
                    e.printStackTrace();
                    throw e;
                }
            }
        }
    }

    private Integer[] getRandomArray() {
        List< Integer > randList = new ArrayList<>();
        for ( int i = 0; i <= 44000; i++ ) {
            randList.add( i );
        }
        Collections.shuffle( randList );
        Integer[] randArray = randList
                .toArray( new Integer[ randList.size() ] );
        return randArray;
    }

    class QueryThread extends SdbThreadBase {
        private DBCollection cl;
        private String hint;

        private QueryThread( DBCollection cl, String hint ) {
            this.cl = cl;
            this.hint = hint;
        }

        @Override
        public void exec() throws Exception {
            Integer[] randArray = getRandomArray();
            TransUtils.queryAndCheck( cl,
                    "{$and:[{a:{$in:" + Arrays.toString( randArray )
                            + "}},{b:{$in:" + Arrays.toString( randArray )
                            + "}}]}",
                    null, "{a:1, b:1}", hint, expList );
        }
    }
}
