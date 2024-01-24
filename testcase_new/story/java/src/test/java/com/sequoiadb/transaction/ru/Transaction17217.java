package com.sequoiadb.transaction.ru;

import java.util.ArrayList;
import java.util.Collections;

import org.bson.BSONObject;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
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
 * @FileName: seqDB-17217：更新并发，匹配更新后记录，事务回滚，过程中读 更新走表扫描
 * @Author zhaoyu
 * @Date 2019-01-15
 * @Version 1.00
 */
@Test(groups = "ru")
public class Transaction17217 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl_17217";
    private Sequoiadb db1;
    private Sequoiadb db2;
    private Sequoiadb db3;
    private DBCollection cl = null;
    private DBCollection cl1 = null;
    private DBCollection cl2 = null;
    private DBCollection cl3 = null;
    private ArrayList< BSONObject > expList = new ArrayList< BSONObject >();
    private ArrayList< BSONObject > actList = new ArrayList< BSONObject >();
    private DBCursor cursor = null;
    private String hint = null;
    private int startId = 0;
    private int stopId = 1000;
    private int updateValue1 = 20000;
    private int updateValue2 = 30000;

    @DataProvider(name = "index")
    public Object[][] createIndex() {
        return new Object[][] { { "{'a': 1}" }, { "{'a': -1, 'b': 1}" } };
    }

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db3 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
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

    @Test(dataProvider = "index")
    public void test( String indexKey ) throws InterruptedException {
        try {
            cl.createIndex( "a", indexKey, false, false );

            // 开启3个并发事务
            TransUtils.beginTransaction( db1 );
            TransUtils.beginTransaction( db2 );
            TransUtils.beginTransaction( db3 );
            cl1 = db1.getCollectionSpace( csName ).getCollection( clName );
            cl2 = db2.getCollectionSpace( csName ).getCollection( clName );
            cl3 = db3.getCollectionSpace( csName ).getCollection( clName );

            // 插入记录R1
            ArrayList< BSONObject > insertR1s = TransUtils
                    .insertRandomDatas( cl, startId, stopId );

            // 事务1匹配R1更新为R2
            hint = "{\"\":null}";
            cl1.update( "{a: {$gte: " + startId + ", $lt: " + stopId + "}}",
                    "{$inc:{a:" + updateValue1 + "}}", hint );

            // 事务2匹配记录R2更新为R3
            UpdateThread updateThread = new UpdateThread();
            updateThread.start();
            Assert.assertTrue( TransUtils.isTransWaitLock( sdb,
                    updateThread.getTransactionID() ) );

            // 事务1记录读
            ArrayList< BSONObject > updateR1s = TransUtils.getIncDatas( startId,
                    stopId, updateValue1 );
            expList.clear();
            expList.addAll( updateR1s );
            hint = "{\"\":null}";
            cursor = cl1.query( null, null, "{_id:1}", hint );
            actList = TransUtils.getReadActList( cursor );
            Assert.assertEquals( actList, expList );
            actList.clear();

            // 事务1记录逆序读
            Collections.reverse( expList );
            hint = "{\"\":null}";
            cursor = cl1.query( null, null, "{_id: -1}", hint );
            actList = TransUtils.getReadActList( cursor );
            Assert.assertEquals( actList, expList );
            actList.clear();

            // 事务1索引读
            Collections.reverse( expList );
            hint = "{\"\":\"a\"}";
            cursor = cl1.query( null, null, "{_id:1}", hint );
            actList = TransUtils.getReadActList( cursor );
            Assert.assertEquals( actList, expList );
            actList.clear();

            // 事务1索引逆序读
            Collections.reverse( expList );
            hint = "{\"\":\"a\"}";
            cursor = cl1.query( null, null, "{_id: -1}", hint );
            actList = TransUtils.getReadActList( cursor );
            Assert.assertEquals( actList, expList );
            actList.clear();

            // 事务3记录读
            Collections.reverse( expList );
            hint = "{\"\":null}";
            cursor = cl3.query( null, null, "{_id:1}", hint );
            actList = TransUtils.getReadActList( cursor );
            Assert.assertEquals( actList, expList );
            actList.clear();

            // 事务3记录逆序读
            Collections.reverse( expList );
            hint = "{\"\":null}";
            cursor = cl3.query( null, null, "{_id: -1}", hint );
            actList = TransUtils.getReadActList( cursor );
            Assert.assertEquals( actList, expList );
            actList.clear();

            // 事务3索引读
            Collections.reverse( expList );
            hint = "{\"\":\"a\"}";
            cursor = cl3.query( null, null, "{_id:1}", hint );
            actList = TransUtils.getReadActList( cursor );
            Assert.assertEquals( actList, expList );
            actList.clear();

            // 事务3索引逆序读
            Collections.reverse( expList );
            hint = "{\"\":\"a\"}";
            cursor = cl3.query( null, null, "{_id: -1}", hint );
            actList = TransUtils.getReadActList( cursor );
            Assert.assertEquals( actList, expList );
            actList.clear();

            // 非事务记录读
            Collections.reverse( expList );
            hint = "{\"\":null}";
            cursor = cl.query( null, null, "{_id:1}", hint );
            actList = TransUtils.getReadActList( cursor );
            Assert.assertEquals( actList, expList );
            actList.clear();

            // 非事务记录逆序读
            Collections.reverse( expList );
            hint = "{\"\":null}";
            cursor = cl.query( null, null, "{_id: -1}", hint );
            actList = TransUtils.getReadActList( cursor );
            Assert.assertEquals( actList, expList );
            actList.clear();

            // 非事务索引读
            Collections.reverse( expList );
            hint = "{\"\":\"a\"}";
            cursor = cl.query( null, null, "{_id:1}", hint );
            actList = TransUtils.getReadActList( cursor );
            Assert.assertEquals( actList, expList );
            actList.clear();

            // 非事务索引逆序读
            Collections.reverse( expList );
            hint = "{\"\":\"a\"}";
            cursor = cl.query( null, null, "{_id: -1}", hint );
            actList = TransUtils.getReadActList( cursor );
            Assert.assertEquals( actList, expList );
            actList.clear();

            // 回滚事务1
            db1.rollback();
            Assert.assertTrue( updateThread.isSuccess(),
                    updateThread.getErrorMsg() );

            // 非事务记录读
            expList.clear();
            expList.addAll( insertR1s );
            hint = "{\"\":null}";
            cursor = cl.query( null, null, "{_id:1}", hint );
            actList = TransUtils.getReadActList( cursor );
            Assert.assertEquals( actList, expList );
            actList.clear();

            // 非事务记录逆序读
            Collections.reverse( expList );
            hint = "{\"\":null}";
            cursor = cl.query( null, null, "{_id: -1}", hint );
            actList = TransUtils.getReadActList( cursor );
            Assert.assertEquals( actList, expList );
            actList.clear();

            // 非事务索引读
            Collections.reverse( expList );
            hint = "{\"\":\"a\"}";
            cursor = cl.query( null, null, "{_id:1}", hint );
            actList = TransUtils.getReadActList( cursor );
            Assert.assertEquals( actList, expList );
            actList.clear();

            // 非事务索引逆序读
            Collections.reverse( expList );
            hint = "{\"\":\"a\"}";
            cursor = cl.query( null, null, "{_id: -1}", hint );
            actList = TransUtils.getReadActList( cursor );
            Assert.assertEquals( actList, expList );
            actList.clear();

            // 事务2记录读
            Collections.reverse( expList );
            hint = "{\"\":null}";
            cursor = cl2.query( null, null, "{_id:1}", hint );
            actList = TransUtils.getReadActList( cursor );
            Assert.assertEquals( actList, expList );
            actList.clear();

            // 事务2记录逆序读
            Collections.reverse( expList );
            hint = "{\"\":null}";
            cursor = cl2.query( null, null, "{_id: -1}", hint );
            actList = TransUtils.getReadActList( cursor );
            Assert.assertEquals( actList, expList );
            actList.clear();

            // 事务2索引读
            Collections.reverse( expList );
            hint = "{\"\":\"a\"}";
            cursor = cl2.query( null, null, "{_id:1}", hint );
            actList = TransUtils.getReadActList( cursor );
            Assert.assertEquals( actList, expList );
            actList.clear();

            // 事务2索引逆序读
            Collections.reverse( expList );
            hint = "{\"\":\"a\"}";
            cursor = cl2.query( null, null, "{_id: -1}", hint );
            actList = TransUtils.getReadActList( cursor );
            Assert.assertEquals( actList, expList );
            actList.clear();

            // 事务3记录读
            Collections.reverse( expList );
            hint = "{\"\":null}";
            cursor = cl3.query( null, null, "{_id:1}", hint );
            actList = TransUtils.getReadActList( cursor );
            Assert.assertEquals( actList, expList );
            actList.clear();

            // 事务3记录逆序读
            Collections.reverse( expList );
            hint = "{\"\":null}";
            cursor = cl3.query( null, null, "{_id: -1}", hint );
            actList = TransUtils.getReadActList( cursor );
            Assert.assertEquals( actList, expList );
            actList.clear();

            // 事务3索引读
            Collections.reverse( expList );
            hint = "{\"\":\"a\"}";
            cursor = cl3.query( null, null, "{_id:1}", hint );
            actList = TransUtils.getReadActList( cursor );
            Assert.assertEquals( actList, expList );
            actList.clear();

            // 事务3索引逆序读
            Collections.reverse( expList );
            hint = "{\"\":\"a\"}";
            cursor = cl3.query( null, null, "{_id:-1}", hint );
            actList = TransUtils.getReadActList( cursor );
            Assert.assertEquals( actList, expList );
            actList.clear();

            // 提交事务2
            db2.commit();

            // 非事务记录读
            Collections.reverse( expList );
            hint = "{\"\":null}";
            cursor = cl.query( null, null, "{_id:1}", hint );
            actList = TransUtils.getReadActList( cursor );
            Assert.assertEquals( actList, expList );
            actList.clear();

            // 非事务记录逆序读
            Collections.reverse( expList );
            hint = "{\"\":null}";
            cursor = cl.query( null, null, "{_id: -1}", hint );
            actList = TransUtils.getReadActList( cursor );
            Assert.assertEquals( actList, expList );
            actList.clear();

            // 非事务索引读
            Collections.reverse( expList );
            hint = "{\"\":\"a\"}";
            cursor = cl.query( null, null, "{_id:1}", hint );
            actList = TransUtils.getReadActList( cursor );
            Assert.assertEquals( actList, expList );
            actList.clear();

            // 非事务索引逆序读
            Collections.reverse( expList );
            hint = "{\"\":\"a\"}";
            cursor = cl.query( null, null, "{_id: -1}", hint );
            actList = TransUtils.getReadActList( cursor );
            Assert.assertEquals( actList, expList );
            actList.clear();

            // 事务3记录读
            Collections.reverse( expList );
            hint = "{\"\":null}";
            cursor = cl3.query( null, null, "{_id:1}", hint );
            actList = TransUtils.getReadActList( cursor );
            Assert.assertEquals( actList, expList );
            actList.clear();

            // 事务3记录逆序读
            Collections.reverse( expList );
            hint = "{\"\":null}";
            cursor = cl3.query( null, null, "{_id: -1}", hint );
            actList = TransUtils.getReadActList( cursor );
            Assert.assertEquals( actList, expList );
            actList.clear();

            // 事务3索引读
            Collections.reverse( expList );
            hint = "{\"\":\"a\"}";
            cursor = cl3.query( null, null, "{_id:1}", hint );
            actList = TransUtils.getReadActList( cursor );
            Assert.assertEquals( actList, expList );
            actList.clear();

            // 事务3索引逆序读
            Collections.reverse( expList );
            hint = "{\"\":\"a\"}";
            cursor = cl3.query( null, null, "{_id: -1}", hint );
            actList = TransUtils.getReadActList( cursor );
            Assert.assertEquals( actList, expList );
            actList.clear();

            // 提交事务3
            db3.commit();

            // 删除记录
            cl.delete( ( BSONObject ) null );

            // 非事务索引读
            hint = "{\"\":\"a\"}";
            cursor = cl.query( null, null, null, hint );
            actList = TransUtils.getReadActList( cursor );
            Assert.assertTrue( actList.isEmpty() );
            actList.clear();

            // 非事务记录读
            hint = "{\"\":null}";
            cursor = cl.query( null, null, null, hint );
            actList = TransUtils.getReadActList( cursor );
            Assert.assertTrue( actList.isEmpty() );
            actList.clear();
        } finally {
            // db1.commit();
            // db2.commit();
            // db3.commit();
            if ( cl.isIndexExist( "a" ) ) {
                cl.dropIndex( "a" );
            }
            // cl.truncate();
        }
    }

    private class UpdateThread extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            // 判断事务阻塞需先获取事务id
            setTransactionID( cl2.getSequoiadb() );

            hint = "{\"\":null}";
            cl2.update(
                    "{a: {$gte: " + ( startId + updateValue1 ) + ", $lt: "
                            + ( stopId + updateValue1 ) + "}}",
                    "{$inc:{a:" + updateValue2 + "}}", hint );
        }
    }

}
