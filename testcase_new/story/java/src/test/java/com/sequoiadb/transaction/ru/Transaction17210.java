package com.sequoiadb.transaction.ru;

import java.util.ArrayList;

import org.bson.BSONObject;
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

/**
 * @FileName:seqDB-17210:插入并发，过程中读
 * @Author zhaoyu
 * @Date 2019-01-14
 * @Version 1.00
 */
@Test(groups = "ru")
public class Transaction17210 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl_17210";
    private Sequoiadb db1;
    private Sequoiadb db2;
    private Sequoiadb db3;
    DBCollection cl = null;
    private ArrayList< BSONObject > expList = new ArrayList< BSONObject >();
    private ArrayList< BSONObject > actList = new ArrayList< BSONObject >();
    private DBCursor cursor = null;
    private String hint = null;
    private int startId = 0;
    private int stopId = 1000;
    private int insertValue = 10000;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db3 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        cl.createIndex( "a", "{a:1}", false, false );
    }

    @AfterClass
    public void tearDown() {
        db1.commit();
        db2.commit();
        db3.commit();

        // 关闭所有游标
        sdb.closeAllCursors();
        db1.closeAllCursors();
        db2.closeAllCursors();
        db3.closeAllCursors();

        // 先关闭事务连接，再删除集合
        CollectionSpace cs = sdb.getCollectionSpace( csName );
        if ( !db1.isClosed() ) {
            db1.close();
        }
        if ( !db2.isClosed() ) {
            db2.close();
        }
        if ( !db3.isClosed() ) {
            db3.close();
        }
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }
        if ( !sdb.isClosed() ) {
            sdb.close();
        }
    }

    @Test
    public void test() {
        // 开启3个并发事务
        TransUtils.beginTransaction( db1 );
        TransUtils.beginTransaction( db2 );
        TransUtils.beginTransaction( db3 );
        DBCollection cl1 = db1.getCollectionSpace( csName )
                .getCollection( clName );
        DBCollection cl2 = db2.getCollectionSpace( csName )
                .getCollection( clName );
        DBCollection cl3 = db3.getCollectionSpace( csName )
                .getCollection( clName );

        // 事务1插入记录R1
        ArrayList< BSONObject > insertR1s = TransUtils.insertDatas( cl1,
                startId, stopId, insertValue );

        // 事务2插入记录R2，记录内容与R1相同
        ArrayList< BSONObject > insertR2s = TransUtils.insertDatas( cl2,
                startId + 1000, stopId + 1000, insertValue );

        // 事务1索引读
        expList.addAll( insertR1s );
        expList.addAll( insertR2s );
        hint = "{\"\":\"a\"}";
        cursor = cl1.query( null, null, "{_id:1}", hint );
        actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );
        actList.clear();

        // 事务1记录读
        hint = "{\"\":null}";
        cursor = cl1.query( null, null, "{_id:1}", hint );
        actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );
        actList.clear();

        // 事务2索引读
        hint = "{\"\":\"a\"}";
        cursor = cl2.query( null, null, "{_id:1}", hint );
        actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );
        actList.clear();

        // 事务2记录读
        hint = "{\"\":null}";
        cursor = cl2.query( null, null, "{_id:1}", hint );
        actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );
        actList.clear();

        // 事务3索引读
        hint = "{\"\":\"a\"}";
        cursor = cl3.query( null, null, "{_id:1}", hint );
        actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );
        actList.clear();

        // 事务3记录读
        hint = "{\"\":null}";
        cursor = cl3.query( null, null, "{_id:1}", hint );
        actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );
        actList.clear();

        // 非事务索引读
        hint = "{\"\":\"a\"}";
        cursor = cl.query( null, null, "{_id:1}", hint );
        actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );
        actList.clear();

        // 非事务记录读
        hint = "{\"\":null}";
        cursor = cl.query( null, null, "{_id:1}", hint );
        actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );
        actList.clear();

        // 提交事务1
        db1.commit();

        // 非事务索引读
        hint = "{\"\":\"a\"}";
        cursor = cl.query( null, null, "{_id:1}", hint );
        actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );
        actList.clear();

        // 非事务记录读
        hint = "{\"\":null}";
        cursor = cl.query( null, null, "{_id:1}", hint );
        actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );
        actList.clear();

        // 事务2索引读
        hint = "{\"\":\"a\"}";
        cursor = cl2.query( null, null, "{_id:1}", hint );
        actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );
        actList.clear();

        // 事务2记录读
        hint = "{\"\":null}";
        cursor = cl2.query( null, null, "{_id:1}", hint );
        actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );
        actList.clear();

        // 事务3索引读
        hint = "{\"\":\"a\"}";
        cursor = cl3.query( null, null, "{_id:1}", hint );
        actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );
        actList.clear();

        // 事务3记录读
        hint = "{\"\":null}";
        cursor = cl3.query( null, null, "{_id:1}", hint );
        actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );
        actList.clear();

        // 提交事务2
        db2.commit();

        // 非事务索引读
        hint = "{\"\":\"a\"}";
        cursor = cl.query( null, null, "{_id:1}", hint );
        actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );
        actList.clear();

        // 非事务记录读
        hint = "{\"\":null}";
        cursor = cl.query( null, null, "{_id:1}", hint );
        actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );
        actList.clear();

        // 事务3索引读
        hint = "{\"\":\"a\"}";
        cursor = cl3.query( null, null, "{_id:1}", hint );
        actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );
        actList.clear();

        // 事务3记录读
        hint = "{\"\":null}";
        cursor = cl3.query( null, null, "{_id:1}", hint );
        actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );
        actList.clear();

        // 提交事务3
        db3.commit();

        // 删除记录
        cl1.delete( ( BSONObject ) null );

        // 非事务索引读
        expList.clear();
        hint = "{\"\":\"a\"}";
        cursor = cl.query( null, null, null, hint );
        actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );
        actList.clear();

        // 非事务记录读
        hint = "{\"\":null}";
        cursor = cl.query( null, null, null, hint );
        actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );
        actList.clear();

    }

}
