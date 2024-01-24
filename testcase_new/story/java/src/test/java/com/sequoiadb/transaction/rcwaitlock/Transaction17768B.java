package com.sequoiadb.transaction.rcwaitlock;

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
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @FileName:seqDB-17768：更新与删除并发， 删除的记录同时匹配已提交记录及其他事务更新的记录，事务回滚，过程中读 索引扫描,R1>R2
 * @Author zhaoyu
 * @Date 2019-01-29
 * @Version 1.00
 */
@Test(groups = "rcwaitlock")
public class Transaction17768B extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl_17768B";
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
    private String hint = "{\"\":\"a\"}";

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

    @SuppressWarnings("unchecked")
    @Test
    public void test() throws InterruptedException {
        // 开启3个并发事务
        TransUtils.beginTransaction( db1 );
        TransUtils.beginTransaction( db2 );
        TransUtils.beginTransaction( db3 );
        cl1 = db1.getCollectionSpace( csName ).getCollection( clName );
        cl2 = db2.getCollectionSpace( csName ).getCollection( clName );
        cl3 = db3.getCollectionSpace( csName ).getCollection( clName );

        // 插入记录R1、R2，R1>R2
        BSONObject insertR1 = ( BSONObject ) JSON.parse( "{_id:1,a:2,b:2}" );
        cl.insert( insertR1 );
        BSONObject insertR2 = ( BSONObject ) JSON.parse( "{_id:2,a:1,b:1}" );
        cl.insert( insertR2 );

        // 事务1更新记录R1为R3
        cl1.update( "{a:2}", "{$set:{a:3}}", hint );

        // 事务2匹配R1、R2删除
        DeleteThread deleteThread = new DeleteThread();
        deleteThread.start();
        Assert.assertTrue( TransUtils.isTransWaitLock( sdb,
                deleteThread.getTransactionID() ) );

        // 事务3记录读
        TransactionQueryThread tableScanThread1 = new TransactionQueryThread(
                cl3 );
        tableScanThread1.start();
        Assert.assertTrue( TransUtils.isTransWaitLock( sdb,
                tableScanThread1.getTransactionID() ) );

        // 非事务读
        BSONObject updateR1 = ( BSONObject ) JSON.parse( "{_id:1,a:3,b:2}" );
        expList.add( updateR1 );
        cursor = cl.query( null, null, "{_id:1}", hint );
        actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );
        actList.clear();

        // 提交事务1
        db1.commit();
        Assert.assertTrue( deleteThread.isSuccess(),
                deleteThread.getErrorMsg() );
        Assert.assertTrue( TransUtils.isTransWaitLock( sdb,
                tableScanThread1.getTransactionID() ) );

        // 非事务读
        expList.clear();
        cursor = cl.query( null, null, "{_id:1}", hint );
        actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );
        actList.clear();

        // 事务2记录读
        cursor = cl2.query( null, null, "{_id:1}", hint );
        actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );
        actList.clear();

        // 回滚事务2
        db2.rollback();
        Assert.assertTrue( tableScanThread1.isSuccess(),
                tableScanThread1.getErrorMsg() );

        // 校验事务3读返回的记录
        expList.add( updateR1 );
        expList.add( insertR2 );
        try {
            actList = ( ArrayList< BSONObject > ) tableScanThread1
                    .getExecResult();
            Assert.assertEquals( actList, expList );
            actList.clear();

        } catch ( InterruptedException e ) {
            e.printStackTrace();
            Assert.fail();
        }

        // 非事务记录读
        cursor = cl.query( null, null, "{_id:1}", hint );
        actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );
        actList.clear();

        // 事务3读记录
        cursor = cl3.query( null, null, "{_id:1}", hint );
        actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );
        actList.clear();

        // 提交事务3
        db3.commit();
    }

    private class DeleteThread extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            // 判断事务阻塞需先获取事务id
            setTransactionID( cl2.getSequoiadb() );

            cl2.delete( null, hint );
        }
    }

    private class TransactionQueryThread extends SdbThreadBase {
        private DBCollection cl = null;

        public TransactionQueryThread( DBCollection cl ) {
            super();
            this.cl = cl;
        }

        @Override
        public void exec() throws BaseException {
            // 判断事务阻塞需先获取事务id
            setTransactionID( cl.getSequoiadb() );

            List< BSONObject > ret = new ArrayList< BSONObject >();
            DBCursor indexCursor = cl.query( null, null, "{_id:1}", hint );
            while ( indexCursor.hasNext() ) {
                ret.add( indexCursor.getNext() );
            }
            setExecResult( ret );
        }
    }

}
