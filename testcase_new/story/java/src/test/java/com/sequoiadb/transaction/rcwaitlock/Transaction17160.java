package com.sequoiadb.transaction.rcwaitlock;

/**
 * @Description seqDB-17160:  对大记录进行操作与读并发，事务提交 
 * @author xiaoni Zhao
 * @date 2019-1-23
 */
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
public class Transaction17160 extends SdbTestBase {
    private String clName = "cl_17160";
    private Sequoiadb sdb = null;
    private Sequoiadb db1 = null;
    private DBCollection cl = null;
    private DBCollection cl1 = null;
    private DBCursor cursor = null;
    private StringBuilder a1 = null;

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
        StringBuilder b = new StringBuilder( 60 * 1000 * 20 );
        for ( int i = 0; i < 60 * 1000; i++ ) {
            b.append( "bbbbbbbbbbbbbbbbbbbb" );
        }

        a1 = new StringBuilder( 4000 );
        for ( int i = 0; i < 200; i++ ) {
            a1.append( "aaaaaaaaaaaaaaaaaaaa" );
        }

        for ( int i = 0; i < 10; i++ ) {
            BSONObject insertR1 = ( BSONObject ) JSON.parse(
                    "{_id:" + i + ", a:'" + a1 + i + "', b:'" + b + "'}" );
            cl.insert( insertR1 );
        }

        TransUtils.beginTransaction( db1 );

        // 事务1对同一条记录执行多个操作
        for ( int i = 0; i < 10; i++ ) {
            BSONObject insertR2 = ( BSONObject ) JSON
                    .parse( "{_id:" + ( 10 + i ) + ", a:'" + a1 + ( 10 + i )
                            + "', b:'" + b + "'}" );
            // 事务1对同一条记录执行多个操作
            cl1.insert( insertR2 );
            cl1.update( "{a:'" + a1 + ( 10 + i ) + "'}",
                    "{$set:{a:'" + a1 + 'a' + ( 10 + i ) + "'}}", "{'':'a'}" );
            cl1.delete( "{a:'" + a1 + 'a' + ( 10 + i ) + "'}", "{'':'a'}" );
            // 事务1对不同记录执行多个操作
            cl1.delete( "{a:'" + a1 + i + "'}", "{'':'a'}" );
            cl1.insert( insertR2 );
            cl1.update( "{a:'" + a1 + ( 10 + i ) + "'}",
                    "{$set:{a:'" + a1 + 'a' + ( 10 + i ) + "'}}", "{'':'a'}" );
            cl1.update( "{a:'" + a1 + 'a' + ( 10 + i ) + "'}",
                    "{$set:{a:'" + a1 + ( 10 + i ) + "'}}", "{'':'a'}" );
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
        cursor = cl.query( null, "{id:1, a:1}", "{a:1}", "{'':null}" );
        Assert.assertTrue( TransUtils.getReadActList( cursor, a1, 10 ) );

        // 非事务索引扫描记录
        cursor = cl.query( null, "{id:1, a:1}", "{a:1}", "{'':'a'}" );
        Assert.assertTrue( TransUtils.getReadActList( cursor, a1, 10 ) );

        db1.commit();

        // 校验被阻塞线程返回的记录
        if ( !read1.isSuccess() || !read2.isSuccess() ) {
            Assert.fail( read1.getErrorMsg() + read2.getErrorMsg() );
        }
        try {
            Assert.assertTrue( ( boolean ) read1.getExecResult() );
            Assert.assertTrue( ( boolean ) read2.getExecResult() );
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
            setTransactionID( db2 );

            try {
                cursor = cl2.query( null, "{id:1, a:1}", "{a:1}", hint );
                boolean ret = TransUtils.getReadActList( cursor, a1, 10 );
                setExecResult( ret );

                // 事务2扫描记录
                cursor = cl2.query( null, "{id:1, a:1}", "{a:1}", hint );
                Assert.assertTrue(
                        TransUtils.getReadActList( cursor, a1, 10 ) );

                // 非事务扫描记录
                cursor = cl.query( null, "{id:1, a:1}", "{a:1}", hint );
                Assert.assertTrue(
                        TransUtils.getReadActList( cursor, a1, 10 ) );

                db2.commit();
            } finally {
                db2.commit();
                cursor.close();
                db2.close();
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
