package com.sequoiadb.transaction.rs;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
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
 * @testcase seqDB-18136:读并发与更新并发
 * @date 2019-4-8
 * @author zhaoyu
 *
 */
@Test(groups = "rs")
public class Transaction18136A extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl18136A";
    private DBCollection cl = null;
    private List< BSONObject > expList = new ArrayList< BSONObject >();
    private List< BSONObject > actList = new ArrayList< BSONObject >();
    private Sequoiadb db1 = null;
    private Sequoiadb db2 = null;
    private Sequoiadb db3 = null;
    private DBCollection cl1 = null;
    private DBCollection cl2 = null;
    private DBCollection cl3 = null;

    @DataProvider(name = "index")
    public Object[][] createIndex() {
        return new Object[][] { { "{'a': 1}" }, { "{'a': -1, 'b': 1}" }, };
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
        if ( !db1.isClosed() ) {
            db1.close();
        }
        if ( !db2.isClosed() ) {
            db2.close();
        }
        if ( !db3.isClosed() ) {
            db3.close();
        }
        CollectionSpace cs = sdb.getCollectionSpace( csName );
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
            BSONObject R1 = ( BSONObject ) JSON.parse( "{_id:1, a:1, b:1}" );
            cl.insert( R1 );

            // 1 开启并发事务
            cl1 = db1.getCollectionSpace( csName ).getCollection( clName );
            cl2 = db2.getCollectionSpace( csName ).getCollection( clName );
            cl3 = db3.getCollectionSpace( csName ).getCollection( clName );
            TransUtils.beginTransaction( db1 );
            TransUtils.beginTransaction( db2 );
            TransUtils.beginTransaction( db3 );

            // 事务1读记录走表扫描
            expList.clear();
            expList.add( R1 );
            DBCursor recordsCursor = cl1.query( null, null, null, "{'':null}" );
            actList = TransUtils.getReadActList( recordsCursor );
            Assert.assertEquals( actList, expList );

            // 事务1逆序读记录走表扫描
            recordsCursor = cl1.query( null, null, "{'a': -1}", "{'':null}" );
            actList = TransUtils.getReadActList( recordsCursor );
            Assert.assertEquals( actList, expList );

            // 事务2读记录走表扫描
            recordsCursor = cl2.query( null, null, null, "{'':null}" );
            actList = TransUtils.getReadActList( recordsCursor );
            Assert.assertEquals( actList, expList );

            // 事务2逆序读记录走表扫描
            recordsCursor = cl2.query( null, null, "{'a': -1}", "{'':null}" );
            actList = TransUtils.getReadActList( recordsCursor );
            Assert.assertEquals( actList, expList );

            // 事务3更新记录
            UpdateThread updateThread = new UpdateThread();
            updateThread.start();
            Assert.assertTrue( TransUtils.isTransWaitLock( sdb,
                    updateThread.getTransactionID() ) );

            // 提交事务1和事务2
            db1.commit();
            db2.commit();
            Assert.assertTrue( updateThread.isSuccess(),
                    updateThread.getErrorMsg() );

            // 非事务表扫描
            BSONObject R2 = ( BSONObject ) JSON.parse( "{_id:1, a:2, b:1}" );
            expList.clear();
            expList.add( R2 );
            recordsCursor = cl.query( null, null, null, "{'':null}" );
            actList = TransUtils.getReadActList( recordsCursor );
            Assert.assertEquals( actList, expList );

            // 非事务索引扫描
            recordsCursor = cl.query( null, null, null, "{'':'a'}" );
            actList = TransUtils.getReadActList( recordsCursor );
            Assert.assertEquals( actList, expList );

            // 非事务表扫描逆序读
            recordsCursor = cl.query( null, null, "{'a': -1}", "{'':null}" );
            actList = TransUtils.getReadActList( recordsCursor );
            Assert.assertEquals( actList, expList );

            // 非事务索引扫描逆序读
            recordsCursor = cl.query( null, null, "{'a': -1}", "{'':'a'}" );
            actList = TransUtils.getReadActList( recordsCursor );
            Assert.assertEquals( actList, expList );

            // 事务3提交
            db3.commit();

            // 非事务表扫描
            recordsCursor = cl.query( null, null, null, "{'':null}" );
            actList = TransUtils.getReadActList( recordsCursor );
            Assert.assertEquals( actList, expList );

            // 非事务索引扫描
            recordsCursor = cl.query( null, null, null, "{'':'a'}" );
            actList = TransUtils.getReadActList( recordsCursor );
            Assert.assertEquals( actList, expList );

            // 非事务表扫描逆序读
            recordsCursor = cl.query( null, null, "{'a': -1}", "{'':null}" );
            actList = TransUtils.getReadActList( recordsCursor );
            Assert.assertEquals( actList, expList );

            // 非事务索引扫描逆序读
            recordsCursor = cl.query( null, null, "{'a': -1}", "{'':'a'}" );
            actList = TransUtils.getReadActList( recordsCursor );
            Assert.assertEquals( actList, expList );

            // 删除记录
            cl.delete( null, "{'':'a'}" );
            recordsCursor.close();
        } finally {
            db1.commit();
            db2.commit();
            db3.commit();
            if ( cl.isIndexExist( "a" ) ) {
                cl.dropIndex( "a" );
            }
            cl.truncate();
        }
    }

    private class UpdateThread extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            // 判断事务阻塞需先获取事务id
            setTransactionID( cl3.getSequoiadb() );

            cl3.update( null, "{$set:{a:" + 2 + "}}", "{\"\":null}" );
        }
    }
}
