package com.sequoiadb.transaction.rc;

/**
 * @Description seqDB-17083:   对大记录进行操作与读并发，事务回滚  
 * @author Zhao Xiaoni
 * @date 2019-1-21
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

@Test(groups = "rc")
public class Transaction17083 extends SdbTestBase {
    private String clName = "cl_17083";
    private Sequoiadb sdb = null;
    private Sequoiadb db1 = null;
    private Sequoiadb db2 = null;
    private Sequoiadb dbT = null;
    private Sequoiadb dbI = null;
    private DBCollection cl = null;
    private DBCollection cl1 = null;
    private DBCollection cl2 = null;
    private DBCursor cursor = null;
    private StringBuilder b = null;
    private StringBuilder a1 = null;

    @BeforeClass
    public void setUp() {
        sdb = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        db1 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        db2 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        cl1 = db1.getCollectionSpace( csName ).getCollection( clName );
        cl2 = db2.getCollectionSpace( csName ).getCollection( clName );
        cl.createIndex( "a", "{a:1}", false, false );
    }

    @Test
    public void test() {
        b = new StringBuilder( 60 * 1000 * 20 );
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

        // 开启两个并发事务
        TransUtils.beginTransaction( db1 );
        TransUtils.beginTransaction( db2 );

        // 事务2并发表扫描
        dbT = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        TransUtils.beginTransaction( dbT );
        Read read1 = new Read( dbT, "{'':null}" );
        read1.start();

        // 事务1执行多个原子操作
        Operation operation = new Operation();
        operation.start();

        // 事务2并发索引扫描
        dbI = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        TransUtils.beginTransaction( dbI );
        Read read2 = new Read( dbI, "{'':'a'}" );
        read2.start();

        if ( !read1.isSuccess() || !read2.isSuccess()
                || !operation.isSuccess() ) {
            Assert.fail( read1.getErrorMsg() + read2.getErrorMsg()
                    + operation.getErrorMsg() );
        }

        // 非事务表扫描记录
        cursor = cl.query( null, null, "{a:1}", "{'':null}" );
        Assert.assertTrue( TransUtils.getReadActList( cursor, a1, 10 ) );

        // 非事务索引扫描记录
        cursor = cl.query( null, null, "{a:1}", "{'':'a'}" );
        Assert.assertTrue( TransUtils.getReadActList( cursor, a1, 10 ) );

        db1.rollback();

        // 事务2表扫描记录
        cursor = cl2.query( null, null, "{a:1}", "{'':null}" );
        Assert.assertTrue( TransUtils.getReadActList( cursor, a1, 0 ) );

        // 事务2索引扫描记录
        cursor = cl2.query( null, null, "{a:1}", "{'':'a'}" );
        Assert.assertTrue( TransUtils.getReadActList( cursor, a1, 0 ) );

        // 非事务表扫描记录
        cursor = cl.query( null, null, "{a:1}", "{'':null}" );
        Assert.assertTrue( TransUtils.getReadActList( cursor, a1, 0 ) );

        // 非事务索引扫描记录
        cursor = cl.query( null, null, "{a:1}", "{'':'a'}" );
        Assert.assertTrue( TransUtils.getReadActList( cursor, a1, 0 ) );

        db2.rollback();
        dbI.rollback();
        dbT.rollback();
        cursor.close();
    }

    private class Operation extends SdbThreadBase {
        @Override
        public void exec() throws Exception {
            // TODO Auto-generated method stub
            for ( int i = 0; i < 10; i++ ) {
                BSONObject insertR2 = ( BSONObject ) JSON
                        .parse( "{_id:" + ( 10 + i ) + ", a:'" + a1 + ( 10 + i )
                                + "', b:'" + b + "'}" );
                // 事务1对同一条记录执行多个操作
                cl1.insert( insertR2 );
                cl1.update( "{a:'" + a1 + ( 10 + i ) + "'}",
                        "{$set:{a:'" + a1 + 'a' + ( 10 + i ) + "'}}",
                        "{'':'a'}" );
                cl1.delete( "{a:'" + a1 + 'a' + ( 10 + i ) + "'}", "{'':'a'}" );
                // 事务1对不同记录执行多个操作
                cl1.delete( "{a:'" + a1 + i + "'}", "{'':'a'}" );
                cl1.insert( insertR2 );
                cl1.update( "{a:'" + a1 + ( 10 + i ) + "'}",
                        "{$set:{a:'" + a1 + 'a' + ( 10 + i ) + "'}}",
                        "{'':'a'}" );
                cl1.update( "{a:'" + a1 + 'a' + ( 10 + i ) + "'}",
                        "{$set:{a:'" + a1 + ( 10 + i ) + "'}}", "{'':'a'}" );
            }
        }
    }

    private class Read extends SdbThreadBase {
        private String hint = null;
        private Sequoiadb db2 = null;
        private DBCollection cl2 = null;
        private DBCursor cursor = null;

        public Read( Sequoiadb db2, String hint ) {
            // TODO Auto-generated constructor stub
            this.db2 = db2;
            this.hint = hint;
        }

        @Override
        public void exec() throws Exception {
            // TODO Auto-generated method stub
            cl2 = db2.getCollectionSpace( csName ).getCollection( clName );
            // 事务2扫描记录
            for ( int i = 0; i < 5; i++ ) {
                cursor = cl2.query( null, null, "{a:1}", hint );
                Assert.assertTrue( TransUtils.getReadActList( cursor, a1, 0 ) );
            }
            cursor.close();
        }
    }

    @AfterClass
    public void tearDown() {
        db1.commit();
        db2.commit();
        dbT.commit();
        dbI.commit();
        if ( !db1.isClosed() ) {
            db1.close();
        }
        if ( !db2.isClosed() ) {
            db2.close();
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
