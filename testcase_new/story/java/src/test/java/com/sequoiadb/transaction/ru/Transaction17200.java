package com.sequoiadb.transaction.ru;

/**
 * @Description seqDB-17200:   对大记录进行操作与读并发，事务提交  
 * @author Zhao Xiaoni
 * @date 2019-1-16
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
import com.sequoiadb.transaction.TransUtils;

@Test(groups = "ru")
public class Transaction17200 extends SdbTestBase {
    private String clName = "cl_17200";
    private Sequoiadb sdb = null;
    private Sequoiadb db1 = null;
    private Sequoiadb db2 = null;
    private DBCollection cl = null;
    private DBCollection cl1 = null;
    private DBCollection cl2 = null;
    private DBCursor cursor = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        cl1 = db1.getCollectionSpace( csName ).getCollection( clName );
        cl2 = db2.getCollectionSpace( csName ).getCollection( clName );
        cl.createIndex( "a", "{a:1}", false, false );
    }

    @Test
    public void test() {
        StringBuilder b = new StringBuilder( 60 * 1000 * 20 );
        for ( int i = 0; i < 60 * 1000; i++ ) {
            b.append( "bbbbbbbbbbbbbbbbbbbb" );
        }

        StringBuilder a1 = new StringBuilder( 4000 );
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

        // 事务1执行多个原子操作
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
        cursor = cl2.query( null, null, "{a:1}", "{'':null}" );
        Assert.assertTrue( TransUtils.getReadActList( cursor, a1, 10 ) );

        // 事务2索引扫描记录
        cursor = cl2.query( null, null, "{a:1}", "{'':'a'}" );
        Assert.assertTrue( TransUtils.getReadActList( cursor, a1, 10 ) );

        // 非事务表扫描记录
        cursor = cl.query( null, null, "{a:1}", "{'':null}" );
        Assert.assertTrue( TransUtils.getReadActList( cursor, a1, 10 ) );

        // 非事务索引扫描记录
        cursor = cl.query( null, null, "{a:1}", "{'':'a'}" );
        Assert.assertTrue( TransUtils.getReadActList( cursor, a1, 10 ) );

        db1.commit();

        // 事务2表扫描记录
        cursor = cl2.query( null, null, "{a:1}", "{'':null}" );
        Assert.assertTrue( TransUtils.getReadActList( cursor, a1, 10 ) );

        // 事务2索引扫描记录
        cursor = cl2.query( null, null, "{a:1}", "{'':'a'}" );
        Assert.assertTrue( TransUtils.getReadActList( cursor, a1, 10 ) );

        // 非事务表扫描记录
        cursor = cl.query( null, null, "{a:1}", "{'':null}" );
        Assert.assertTrue( TransUtils.getReadActList( cursor, a1, 10 ) );

        // 非事务索引扫描记录
        cursor = cl.query( null, null, "{a:1}", "{'':'a'}" );
        Assert.assertTrue( TransUtils.getReadActList( cursor, a1, 10 ) );

        db2.commit();
        cursor.close();
    }

    @AfterClass
    public void tearDown() {
        db1.commit();
        db2.commit();
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
