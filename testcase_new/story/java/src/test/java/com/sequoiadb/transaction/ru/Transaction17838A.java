package com.sequoiadb.transaction.ru;

import java.util.ArrayList;

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
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @FileName:seqDB-17838:删除与更新并发，删除的记录同时匹配已提交记录及其他事务更新的记录，更新/删除走表扫描，事务提交， 过程中读
 *                                                                        更新
 *                                                                        先插入R1，
 *                                                                        再插入R2
 * @Author zhaoyu
 * @Date 2019-01-29
 * @Version 1.00
 */
// TODO:格式问题
@Test(groups = "ru")
public class Transaction17838A extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl_17783A";
    private Sequoiadb db1;
    private Sequoiadb db2;
    private Sequoiadb db3;
    private DBCollection cl1 = null;
    private DBCollection cl2 = null;
    private DBCollection cl3 = null;
    private DBCollection cl = null;
    private ArrayList< BSONObject > expList = new ArrayList< BSONObject >();
    private ArrayList< BSONObject > actList = new ArrayList< BSONObject >();
    private DBCursor cursor = null;
    private String hint;

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
    public void test() throws InterruptedException {

        // 开启3个并发事务
        TransUtils.beginTransaction( db1 );
        TransUtils.beginTransaction( db2 );
        TransUtils.beginTransaction( db3 );
        cl1 = db1.getCollectionSpace( csName ).getCollection( clName );
        cl2 = db2.getCollectionSpace( csName ).getCollection( clName );
        cl3 = db3.getCollectionSpace( csName ).getCollection( clName );

        // 插入记录R1、R2
        BSONObject insertR1 = ( BSONObject ) JSON.parse( "{_id:1,a:1,b:1}" );
        cl.insert( insertR1 );
        BSONObject insertR2 = ( BSONObject ) JSON.parse( "{_id:2,a:2,b:2}" );
        cl.insert( insertR2 );

        // 事务1删除记录R1
        hint = "{\"\":null}";
        cl1.delete( "{a:1}", hint );

        // 事务2匹配记录R1、R2更新为R3、R4
        UpdateThread updateThread = new UpdateThread();
        updateThread.start();
        Assert.assertTrue( TransUtils.isTransWaitLock( sdb,
                updateThread.getTransactionID() ) );

        // 事务1索引读
        expList.add( insertR2 );
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
        Assert.assertTrue( updateThread.isSuccess(),
                updateThread.getErrorMsg() );

        // 非事务索引读
        expList.clear();
        BSONObject updateR4 = ( BSONObject ) JSON.parse( "{_id:2,a:3,b:2}" );
        expList.add( updateR4 );
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

        // 事务3索引读
        hint = "{\"\":\"a\"}";
        cursor = cl3.query( null, null, null, hint );
        actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );
        actList.clear();

        // 事务3记录读
        hint = "{\"\":null}";
        cursor = cl3.query( null, null, null, hint );
        actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );
        actList.clear();

        // 删除记录
        cl.delete( ( BSONObject ) null );

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

        // 提交事务3
        db3.commit();
    }

    private class UpdateThread extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            // 判断事务阻塞需先获取事务id
            setTransactionID( cl2.getSequoiadb() );

            hint = "{\"\":'a'}";
            cl2.update( null, "{$set:{a:3}}", hint );
        }
    }
}
