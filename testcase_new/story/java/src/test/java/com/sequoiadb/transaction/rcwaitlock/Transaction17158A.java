package com.sequoiadb.transaction.rcwaitlock;

/**
 * @Description seqDB-17158:  多个原子操作与读并发，事务提交 
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
import com.sequoiadb.transaction.TransUtils;

@Test(groups = "rcwaitlock")
public class Transaction17158A extends SdbTestBase {
    private String clName = "cl_17158A";
    private Sequoiadb sdb = null;
    private Sequoiadb db1 = null;
    private Sequoiadb db2 = null;
    private DBCollection cl = null;
    private DBCollection cl1 = null;
    private DBCollection cl2 = null;
    private DBCursor cursor = null;
    private List< BSONObject > expList = new ArrayList<>();

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        cl1 = db1.getCollectionSpace( csName ).getCollection( clName );
        cl2 = db2.getCollectionSpace( csName ).getCollection( clName );
        cl.createIndex( "a", "{a:1}", false, false );
        expList = TransUtils.insertRandomDatas( cl, 0, 10 );
    }

    @Test
    public void test() throws InterruptedException {
        TransUtils.beginTransaction( db1 );
        TransUtils.beginTransaction( db2 );

        // 事务1对同一条记录执行多个原子操作
        BSONObject insertR3 = ( BSONObject ) JSON.parse( "{a:10000,b:10000}" );
        for ( int i = 0; i < 100; i++ ) {
            cl1.insert( insertR3 );
            cl1.update( "{a:10000}", "{$set:{a:10001}}", "{'':'a'}" );
            cl1.delete( "{a:10001}", "{'':'a'}" );
        }

        // 事务2表扫描记录
        cursor = cl2.query( null, null, "{a:1}", "{'':null}" );
        Assert.assertEquals( TransUtils.getReadActList( cursor ), expList );

        // 事务2索引扫描记录
        cursor = cl2.query( null, null, "{a:1}", "{'':'a'}" );
        Assert.assertEquals( TransUtils.getReadActList( cursor ), expList );

        // 非事务表扫描记录
        cursor = cl.query( null, null, "{a:1}", "{'':null}" );
        Assert.assertEquals( TransUtils.getReadActList( cursor ), expList );

        // 非事务索引扫描记录
        cursor = cl.query( null, null, "{a:1}", "{'':'a'}" );
        Assert.assertEquals( TransUtils.getReadActList( cursor ), expList );

        db1.commit();

        // 事务2表扫描记录
        cursor = cl2.query( null, null, "{a:1}", "{'':null}" );
        Assert.assertEquals( TransUtils.getReadActList( cursor ), expList );

        // 事务2索引扫描记录
        cursor = cl2.query( null, null, "{a:1}", "{'':'a'}" );
        Assert.assertEquals( TransUtils.getReadActList( cursor ), expList );

        // 非事务表扫描记录
        cursor = cl.query( null, null, "{a:1}", "{'':null}" );
        Assert.assertEquals( TransUtils.getReadActList( cursor ), expList );

        // 非事务索引扫描记录
        cursor = cl.query( null, null, "{a:1}", "{'':'a'}" );
        Assert.assertEquals( TransUtils.getReadActList( cursor ), expList );

        db2.commit();
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
