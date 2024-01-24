package com.sequoiadb.transaction.ru;

/**
 * @Description seqDB-17232:更新多个索引，同时与读并发
 * @author Zhao Xiaoni
 * @date 2019-1-17
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

@Test(groups = "ru")
public class Transaction17231B extends SdbTestBase {
    private String clName = "cl_17231B";
    private Sequoiadb sdb = null;
    private Sequoiadb db1 = null;
    private Sequoiadb db2 = null;
    private DBCollection cl = null;
    private DBCollection cl1 = null;
    private DBCollection cl2 = null;
    private DBCursor cursor = null;
    private List< BSONObject > expList = new ArrayList< BSONObject >();
    private List< BSONObject > actList = new ArrayList< BSONObject >();

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        cl1 = db1.getCollectionSpace( csName ).getCollection( clName );
        cl2 = db2.getCollectionSpace( csName ).getCollection( clName );
        cl.createIndex( "a", "{a:1, b:1, c:1}", false, false );

        BSONObject insertR1 = ( BSONObject ) JSON
                .parse( "{_id:1, a:1, b:1, c:1}" );
        cl.insert( insertR1 );
    }

    @Test
    public void test() {
        // 开启两个并发事务
        TransUtils.beginTransaction( db1 );
        TransUtils.beginTransaction( db2 );

        // 事务1将部分索引字段删除
        cl1.update( null, "{$unset:{a:1}}", "{'':'a'}" );
        BSONObject updateR1 = ( BSONObject ) JSON.parse( "{_id:1, b:1, c:1}" );
        expList.add( updateR1 );

        // 事务1表扫描记录
        cursor = cl1.query( null, null, null, "{'':null}" );
        actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );
        actList.clear();

        // 事务1索引扫描记录
        cursor = cl1.query( null, null, null, "{'':'a'}" );
        actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );
        actList.clear();

        // 事务2表扫描记录
        cursor = cl2.query( null, null, null, "{'':null}" );
        actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );
        actList.clear();

        // 事务2索引扫描记录
        cursor = cl2.query( null, null, null, "{'':'a'}" );
        actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );
        actList.clear();

        // 非事务表扫描记录
        cursor = cl.query( null, null, null, "{'':null}" );
        actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );
        actList.clear();

        // 非事务索引扫描记录
        cursor = cl.query( null, null, null, "{'':'a'}" );
        actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );
        actList.clear();

        db1.commit();

        // 事务2表扫描记录
        cursor = cl2.query( null, null, null, "{'':null}" );
        actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );
        actList.clear();

        // 事务2索引扫描记录
        cursor = cl2.query( null, null, null, "{'':'a'}" );
        actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );
        actList.clear();

        // 非事务表扫描记录
        cursor = cl.query( null, null, null, "{'':null}" );
        actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );
        actList.clear();

        // 非事务索引扫描记录
        cursor = cl.query( null, null, null, "{'':'a'}" );
        actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );
        actList.clear();

        db2.commit();

        // 非事务表扫描记录
        cursor = cl.query( null, null, null, "{'':null}" );
        actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );
        actList.clear();

        // 非事务索引扫描记录
        cursor = cl.query( null, null, null, "{'':'a'}" );
        actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );

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
