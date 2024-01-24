package com.sequoiadb.transaction.ru;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
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
 * @FileName:seqDB-17782:删除并发，删除的记录同时匹配已提交记录及其他事务删除的记录，事务回滚，过程中读 删除走索引扫描
 * @Author zhaoyu
 * @Date 2019-01-29
 * @Version 1.00
 */
@Test(groups = "ru")
public class Transaction17782 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl_17782";
    private Sequoiadb db1;
    private Sequoiadb db2;
    private Sequoiadb db3;
    private DBCollection cl1 = null;
    private DBCollection cl2 = null;
    private DBCollection cl3 = null;
    private DBCollection cl = null;
    private BSONObject insertR1 = new BasicBSONObject();
    private BSONObject insertR2 = new BasicBSONObject();
    private ArrayList< BSONObject > expList = new ArrayList<>();
    private ArrayList< BSONObject > actList = new ArrayList<>();
    private DBCursor cursor = null;
    private String hint;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db3 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        insertR1 = ( BSONObject ) JSON
                .parse( "{_id:'insertID17784A_1',a:1,b:1}" );
        insertR2 = ( BSONObject ) JSON
                .parse( "{_id:'insertID17784A_2',a:2,b:2}" );
    }

    @DataProvider(name = "index")
    public Object[][] createIndex() {
        // 正序索引
        List< BSONObject > expPositiveReadList = new ArrayList<>();
        expPositiveReadList.add( insertR2 );

        // 逆序索引
        List< BSONObject > expReverseReadList = new ArrayList<>();

        return new Object[][] { { "{'a': 1}", expPositiveReadList },
                { "{'a': -1}", expReverseReadList } };
    }

    @Test(dataProvider = "index")
    public void test( String indexKey, List< BSONObject > expReadList )
            throws InterruptedException {
        try {

            // 开启3个并发事务
            TransUtils.beginTransaction( db1 );
            TransUtils.beginTransaction( db2 );
            TransUtils.beginTransaction( db3 );
            cl1 = db1.getCollectionSpace( csName ).getCollection( clName );
            cl2 = db2.getCollectionSpace( csName ).getCollection( clName );
            cl3 = db3.getCollectionSpace( csName ).getCollection( clName );

            // 插入记录R1、R2
            cl.createIndex( "a", indexKey, false, false );
            cl.insert( insertR1 );
            cl.insert( insertR2 );

            // 事务1删除记录R1
            hint = "{\"\":\"a\"}";
            cl1.delete( "{a:1}", hint );

            // 事务2匹配R1、R2、R3删除
            DeleteThread deleteThread = new DeleteThread();
            deleteThread.start();
            Assert.assertTrue( TransUtils.isTransWaitLock( sdb,
                    deleteThread.getTransactionID() ) );

            // 事务1记录读,正序
            hint = "{\"\":null}";
            cursor = cl1.query( null, null, "{a:1}", hint );
            actList = TransUtils.getReadActList( cursor );
            Assert.assertEquals( actList, expReadList );
            actList.clear();

            // 事务1索引读,正序
            hint = "{\"\":\"a\"}";
            cursor = cl1.query( null, null, "{a:1}", hint );
            actList = TransUtils.getReadActList( cursor );
            Assert.assertEquals( actList, expReadList );
            actList.clear();

            // 事务3记录读,正序
            hint = "{\"\":null}";
            cursor = cl3.query( null, null, "{a:1}", hint );
            actList = TransUtils.getReadActList( cursor );
            Assert.assertEquals( actList, expReadList );
            actList.clear();

            // 事务3索引读,正序
            hint = "{\"\":\"a\"}";
            cursor = cl3.query( null, null, "{a:1}", hint );
            actList = TransUtils.getReadActList( cursor );
            Assert.assertEquals( actList, expReadList );
            actList.clear();

            // 非事务记录读,正序
            hint = "{\"\":null}";
            cursor = cl.query( null, null, "{a:1}", hint );
            actList = TransUtils.getReadActList( cursor );
            Assert.assertEquals( actList, expReadList );
            actList.clear();

            // 非事务索引读,正序
            hint = "{\"\":\"a\"}";
            cursor = cl.query( null, null, "{a:1}", hint );
            actList = TransUtils.getReadActList( cursor );
            Assert.assertEquals( actList, expReadList );
            actList.clear();

            // 事务1记录读,逆序
            expList.clear();
            expList.add( insertR2 );
            hint = "{\"\":null}";
            cursor = cl1.query( null, null, "{a:-1}", hint );
            actList = TransUtils.getReadActList( cursor );
            Assert.assertEquals( actList, expReadList );
            actList.clear();

            // 事务1索引读,逆序
            hint = "{\"\":\"a\"}";
            cursor = cl1.query( null, null, "{a:-1}", hint );
            actList = TransUtils.getReadActList( cursor );
            Assert.assertEquals( actList, expReadList );
            actList.clear();

            // 事务3记录读,逆序
            hint = "{\"\":null}";
            cursor = cl3.query( null, null, "{a:-1}", hint );
            actList = TransUtils.getReadActList( cursor );
            Assert.assertEquals( actList, expReadList );
            actList.clear();

            // 事务3索引读,逆序
            hint = "{\"\":\"a\"}";
            cursor = cl3.query( null, null, "{a:-1}", hint );
            actList = TransUtils.getReadActList( cursor );
            Assert.assertEquals( actList, expReadList );
            actList.clear();

            // 非事务记录读,逆序
            hint = "{\"\":null}";
            cursor = cl.query( null, null, "{a:-1}", hint );
            actList = TransUtils.getReadActList( cursor );
            Assert.assertEquals( actList, expReadList );
            actList.clear();

            // 非事务索引读,逆序
            hint = "{\"\":\"a\"}";
            cursor = cl.query( null, null, "{a:-1}", hint );
            actList = TransUtils.getReadActList( cursor );
            Assert.assertEquals( actList, expReadList );
            actList.clear();

            // 回滚事务1
            db1.rollback();
            Assert.assertTrue( deleteThread.isSuccess(),
                    deleteThread.getErrorMsg() );

            // 非事务记录读
            expList.clear();
            hint = "{\"\":null}";
            cursor = cl.query( null, null, null, hint );
            actList = TransUtils.getReadActList( cursor );
            Assert.assertEquals( actList, expList );
            actList.clear();

            // 非事务索引读
            expList.clear();
            hint = "{\"\":\"a\"}";
            cursor = cl.query( null, null, null, hint );
            actList = TransUtils.getReadActList( cursor );
            Assert.assertEquals( actList, expList );
            actList.clear();

            // 事务2记录读
            hint = "{\"\":null}";
            cursor = cl2.query( null, null, "{a:1}", hint );
            actList = TransUtils.getReadActList( cursor );
            Assert.assertEquals( actList, expList );
            actList.clear();

            // 事务2索引读
            hint = "{\"\":\"a\"}";
            cursor = cl2.query( null, null, "{a:1}", hint );
            actList = TransUtils.getReadActList( cursor );
            Assert.assertEquals( actList, expList );
            actList.clear();

            // 事务3记录读
            hint = "{\"\":null}";
            cursor = cl3.query( null, null, "{a:1}", hint );
            actList = TransUtils.getReadActList( cursor );
            Assert.assertEquals( actList, expList );
            actList.clear();

            // 事务3索引读
            hint = "{\"\":\"a\"}";
            cursor = cl3.query( null, null, "{a:1}", hint );
            actList = TransUtils.getReadActList( cursor );
            Assert.assertEquals( actList, expList );
            actList.clear();

            // 提交事务2
            db2.commit();

            // 非事务记录读
            hint = "{\"\":null}";
            cursor = cl.query( null, null, null, hint );
            actList = TransUtils.getReadActList( cursor );
            Assert.assertEquals( actList, expList );
            actList.clear();

            // 非事务索引读
            hint = "{\"\":\"a\"}";
            cursor = cl.query( null, null, null, hint );
            actList = TransUtils.getReadActList( cursor );
            Assert.assertEquals( actList, expList );
            actList.clear();

            // 事务3记录读
            hint = "{\"\":null}";
            cursor = cl3.query( null, null, null, hint );
            actList = TransUtils.getReadActList( cursor );
            Assert.assertEquals( actList, expList );
            actList.clear();

            // 事务3索引读
            hint = "{\"\":\"a\"}";
            cursor = cl3.query( null, null, null, hint );
            actList = TransUtils.getReadActList( cursor );
            Assert.assertEquals( actList, expList );
            actList.clear();

            // 提交事务3
            db3.commit();

        } finally {
            // 关闭事务连接
            db1.commit();
            db2.commit();
            db3.commit();

            // 删除索引
            if ( cl.isIndexExist( "a" ) ) {
                cl.dropIndex( "a" );
            }

            // 删除记录
            cl.truncate();

        }

    }

    @AfterClass
    public void tearDown() {
        if ( !db1.isClosed() ) {
            db1.close();
        }
        if ( !db2.isClosed() ) {
            db2.close();
        }
        if ( !db3.isClosed() ) {
            db3.close();
        }
        // 先关闭事务连接，再删除集合
        CollectionSpace cs = sdb.getCollectionSpace( csName );
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }
        if ( !sdb.isClosed() ) {
            sdb.close();
        }
    }

    private class DeleteThread extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            // 判断事务阻塞需先获取事务id
            setTransactionID( cl2.getSequoiadb() );

            hint = "{\"\":\"a\"}";
            cl2.delete( null, hint );
        }
    }
}
