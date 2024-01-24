package com.sequoiadb.transaction.rcwaitlock;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
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
 * @FileName:seqDB-17391：插入并发，过程中读 索引扫描
 * @Author zhaoyu
 * @Date 2019-01-28
 * @Version 1.00
 */
@Test(groups = "rcwaitlock")
public class Transaction17391B extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl_17391B";
    private Sequoiadb db1;
    private Sequoiadb db2;
    private Sequoiadb db3;
    DBCollection cl = null;
    private ArrayList< BSONObject > expList = new ArrayList< BSONObject >();
    private ArrayList< BSONObject > actList = new ArrayList< BSONObject >();
    private String hint = "{\"\":\"a\"}";
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

    @SuppressWarnings("unchecked")
    @Test
    public void test() throws InterruptedException {

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

        // 事务3读
        TransactionQueryThread tableScanThread1 = new TransactionQueryThread(
                cl3 );
        tableScanThread1.start();
        Assert.assertTrue( TransUtils.isTransWaitLock( sdb,
                tableScanThread1.getTransactionID() ) );

        // 非事务读
        expList.addAll( insertR1s );
        expList.addAll( insertR2s );
        TransUtils.queryAndCheck( cl, "{a:" + insertValue + "}", "{_id:1}",
                hint, expList );
        actList.clear();

        // 提交事务1
        db1.commit();
        Assert.assertTrue( TransUtils.isTransWaitLock( sdb,
                tableScanThread1.getTransactionID() ) );

        // 非事务读
        TransUtils.queryAndCheck( cl, "{a:" + insertValue + "}", "{_id:1}",
                hint, expList );
        actList.clear();

        // 事务2读
        TransUtils.queryAndCheck( cl2, "{a:" + insertValue + "}", "{_id:1}",
                hint, expList );
        actList.clear();

        // 提交事务2
        db2.commit();
        Assert.assertTrue( tableScanThread1.isSuccess(),
                tableScanThread1.getErrorMsg() );

        // 检查事务3读
        try {
            actList = ( ArrayList< BSONObject > ) tableScanThread1
                    .getExecResult();
            Assert.assertEquals( actList, expList );
            actList.clear();

        } catch ( InterruptedException e ) {
            e.printStackTrace();
            Assert.fail();
        }

        // 非事务读
        TransUtils.queryAndCheck( cl, "{a:" + insertValue + "}", "{_id:1}",
                hint, expList );
        actList.clear();

        // 事务3读
        TransUtils.queryAndCheck( cl3, "{a:" + insertValue + "}", "{_id:1}",
                hint, expList );
        actList.clear();

        // 提交事务3
        db3.commit();

        // 删除记录
        cl.delete( ( BSONObject ) null );

        // 非事务读
        expList.clear();
        TransUtils.queryAndCheck( cl, "{a:" + insertValue + "}", "{_id:1}",
                hint, expList );
        actList.clear();

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
            DBCursor indexCursor = cl.query( "{a:" + insertValue + "}", null,
                    "{_id:1}", hint );
            while ( indexCursor.hasNext() ) {
                ret.add( indexCursor.getNext() );
            }
            setExecResult( ret );
        }
    }

}
