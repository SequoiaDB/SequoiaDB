package com.sequoiadb.transaction.rcwaitlock;

/**
 * @Description seqDB-17159:  多个原子操作与读并发，事务回滚 
 * @author xiaoni Zhao
 * @date 2019-1-23
 */
import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;
import com.sequoiadb.transaction.TransUtils;

@Test(groups = "rcwaitlock")
public class Transaction17159B extends SdbTestBase {
    private String clName = "cl_17159B";
    private Sequoiadb sdb = null;
    private Sequoiadb db1 = null;
    private DBCollection cl = null;
    private DBCollection cl1 = null;
    private DBCursor cursor = null;
    private List< BSONObject > insertR1s = new ArrayList< BSONObject >();
    private List< BSONObject > expList = new ArrayList< BSONObject >();

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        cl1 = db1.getCollectionSpace( csName ).getCollection( clName );
        cl.createIndex( "a", "{a:1}", false, false );
    }

    @Test
    public void test() throws InterruptedException {
        insertR1s = TransUtils.insertRandomDatas( cl, 0, 10 );

        // 开启事务1
        TransUtils.beginTransaction( db1 );

        // 事务1对不同记录执行多个原子操作
        for ( int i = 0; i < 100; i++ ) {
            cl1.delete( "{a:" + i + "}", "{'':'a'}" );
            cl1.insert( ( BSONObject ) JSON.parse(
                    "{_id:" + ( 10000 + i ) + ", a:" + i + ",b:" + i + "}" ) );
            cl1.update( "{a:" + i + "}", "{$set:{a:" + ( i + 10000 ) + "}}",
                    "{'':'a'}" );
            expList.add( ( BSONObject ) JSON.parse( "{_id:" + ( 10000 + i )
                    + ", a:" + ( i + 10000 ) + ",b:" + i + "}" ) );
        }

        // 事务2表扫描记录
        Read read1 = new Read( "{'':null}" );
        read1.start();

        // 事务2索引扫描记录
        Read read2 = new Read( "{'':'a'}" );
        read2.start();
        Assert.assertTrue(
                TransUtils.isTransWaitLock( sdb, read1.getTransactionID() ) );
        Assert.assertTrue(
                TransUtils.isTransWaitLock( sdb, read2.getTransactionID() ) );

        // 非事务表扫描记录
        cursor = cl.query( null, null, "{a:1}", "{'':null}" );
        Assert.assertEquals( TransUtils.getReadActList( cursor ), expList );

        // 非事务索引扫描记录
        cursor = cl.query( null, null, "{a:1}", "{'':'a'}" );
        Assert.assertEquals( TransUtils.getReadActList( cursor ), expList );

        db1.rollback();

        // 校验阻塞线程返回的记录
        if ( !read1.isSuccess() || !read2.isSuccess() ) {
            Assert.fail( read1.getErrorMsg() + read2.getErrorMsg() );
        }
        try {
            Assert.assertEquals( read1.getExecResult(), insertR1s );
            Assert.assertEquals( read2.getExecResult(), insertR1s );
        } catch ( Exception e ) {
            Assert.fail( e.getMessage() );
        }

        cursor.close();
    }

    private class Read extends SdbThreadBase {
        private Sequoiadb db = null;
        private Sequoiadb db2 = null;
        private DBCollection cl = null;
        private DBCollection cl2 = null;
        private String hint = null;
        private DBCursor cursor = null;

        public Read( String hint ) {
            this.hint = hint;
        }

        @Override
        public void exec() throws Exception {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            db2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            cl = db.getCollectionSpace( csName ).getCollection( clName );
            cl2 = db2.getCollectionSpace( csName ).getCollection( clName );

            // 开启并发事务2
            TransUtils.beginTransaction( db2 );

            // 判断事务阻塞需先获取事务id
            setTransactionID( db2 );

            try {
                cursor = cl2.query( null, null, "{a:1}", hint );
                List< BSONObject > records = TransUtils
                        .getReadActList( cursor );
                setExecResult( records );

                // 事务2扫描记录
                cursor = cl2.query( null, null, "{a:1}", hint );
                Assert.assertEquals( TransUtils.getReadActList( cursor ),
                        insertR1s );

                // 非事务扫描记录
                cursor = cl.query( null, null, "{a:1}", hint );
                Assert.assertEquals( TransUtils.getReadActList( cursor ),
                        insertR1s );

                db2.rollback();
            } finally {
                db2.rollback();
                db2.close();
                cursor.close();
                db.close();
            }
        }
    }

    @AfterClass
    public void tearDown() {
        db1.commit();
        if ( !db1.isClosed() ) {
            db1.close();
        }
        CollectionSpace cs = sdb.getCollectionSpace( csName );
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }
        if ( !sdb.isClosed() ) {
            sdb.close();
        }
    }
}
