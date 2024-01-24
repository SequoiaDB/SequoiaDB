package com.sequoiadb.transaction.rcwaitlock;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

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
 * @FileName:seqDB-17171：插入与更新并发，事务提交，过程中读 索引扫描
 * @Author zhaoyu
 * @Date 2019-01-29
 * @Version 1.00
 */
@Test(groups = "rcwaitlock")
public class Transaction17171B extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl_17171B";
    private Sequoiadb db1;
    private Sequoiadb db2;
    private Sequoiadb db3;
    private DBCollection cl1 = null;
    private DBCollection cl2 = null;
    private DBCollection cl3 = null;
    private DBCollection cl = null;
    private ArrayList< BSONObject > expList = new ArrayList< BSONObject >();
    private ArrayList< BSONObject > actList = new ArrayList< BSONObject >();
    private String hint = "{\"\":\"a\"}";
    private int startId = 0;
    private int stopId = 1000;
    private int updateValue = 20000;
    private String orderByPos = "{a:1}";
    private String orderByRev = "{a: -1}";

    @DataProvider(name = "index")
    public Object[][] createIndex() {
        return new Object[][] { { "{'a': 1}", "{'a': 1}" },
                { "{'a': -1, 'b': 1}", "{'a': -1}" } };
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
    @Test(dataProvider = "index")
    public void test( String indexKey, String orderBy )
            throws InterruptedException {
        try {
            cl.createIndex( "a", indexKey, false, false );

            // 开启3个并发事务
            TransUtils.beginTransaction( db1 );
            TransUtils.beginTransaction( db2 );
            TransUtils.beginTransaction( db3 );
            cl1 = db1.getCollectionSpace( csName ).getCollection( clName );
            cl2 = db2.getCollectionSpace( csName ).getCollection( clName );
            cl3 = db3.getCollectionSpace( csName ).getCollection( clName );

            // 事务1插入记录R1
            ArrayList< BSONObject > insertR1s = TransUtils
                    .insertRandomDatas( cl1, startId, stopId );

            // 事务2匹配记录R1更新为R2
            UpdateThread updateThread = new UpdateThread();
            updateThread.start();
            Assert.assertTrue( TransUtils.isTransWaitLock( sdb,
                    updateThread.getTransactionID() ) );

            // 事务3读
            TransactionQueryThread tableScanThread = new TransactionQueryThread(
                    cl3, orderBy );
            tableScanThread.start();
            Assert.assertTrue( TransUtils.isTransWaitLock( sdb,
                    tableScanThread.getTransactionID() ) );

            // 非事务读
            expList.clear();
            expList.addAll( insertR1s );
            TransUtils.queryAndCheck( cl, "{a:{$lt:" + ( stopId + updateValue )
                    + ",$gte:" + startId + "}}", orderByPos, hint, expList );
            actList.clear();

            // 非事务逆序读
            Collections.reverse( expList );
            TransUtils.queryAndCheck( cl, "{a:{$lt:" + ( stopId + updateValue )
                    + ",$gte:" + startId + "}}", orderByRev, hint, expList );
            actList.clear();

            // 提交事务1
            db1.commit();
            Assert.assertTrue( updateThread.isSuccess(),
                    updateThread.getErrorMsg() );
            Assert.assertTrue( TransUtils.isTransWaitLock( sdb,
                    tableScanThread.getTransactionID() ) );

            // 非事务读
            expList.clear();
            ArrayList< BSONObject > updateR1s = TransUtils.getIncDatas( startId,
                    stopId, updateValue );
            expList.addAll( updateR1s );
            TransUtils.queryAndCheck( cl, "{a:{$lt:" + ( stopId + updateValue )
                    + ",$gte:" + startId + "}}", orderByPos, hint, expList );
            actList.clear();

            // 非事务逆序读
            Collections.reverse( expList );
            TransUtils.queryAndCheck( cl, "{a:{$lt:" + ( stopId + updateValue )
                    + ",$gte:" + startId + "}}", orderByRev, hint, expList );
            actList.clear();

            // 事务2读
            Collections.reverse( expList );
            TransUtils.queryAndCheck( cl2, "{a:{$lt:" + ( stopId + updateValue )
                    + ",$gte:" + startId + "}}", orderByPos, hint, expList );
            actList.clear();

            // 事务2逆序读
            Collections.reverse( expList );
            TransUtils.queryAndCheck( cl2, "{a:{$lt:" + ( stopId + updateValue )
                    + ",$gte:" + startId + "}}", orderByRev, hint, expList );
            actList.clear();

            // 提交事务2
            db2.commit();
            Assert.assertTrue( tableScanThread.isSuccess(),
                    tableScanThread.getErrorMsg() );

            // 检查事务3读
            try {
                Collections.reverse( expList );
                actList = ( ArrayList< BSONObject > ) tableScanThread
                        .getExecResult();
                if ( orderBy.equals( "{'a': 1}" ) ) {
                    Assert.assertEquals( actList, expList );
                } else {
                    Assert.assertEquals( actList.size(), 0 );
                }
                actList.clear();
            } catch ( InterruptedException e ) {
                e.printStackTrace();
                Assert.fail( e.getMessage() );
            }

            // 非事务读
            TransUtils.queryAndCheck( cl, "{a:{$lt:" + ( stopId + updateValue )
                    + ",$gte:" + startId + "}}", orderByPos, hint, expList );
            actList.clear();

            // 非事务逆序读
            Collections.reverse( expList );
            TransUtils.queryAndCheck( cl, "{a:{$lt:" + ( stopId + updateValue )
                    + ",$gte:" + startId + "}}", orderByRev, hint, expList );
            actList.clear();

            // 事务3读
            Collections.reverse( expList );
            TransUtils.queryAndCheck( cl3, "{a:{$lt:" + ( stopId + updateValue )
                    + ",$gte:" + startId + "}}", orderByPos, hint, expList );
            actList.clear();

            // 事务3逆序读
            Collections.reverse( expList );
            TransUtils.queryAndCheck( cl3, "{a:{$lt:" + ( stopId + updateValue )
                    + ",$gte:" + startId + "}}", orderByRev, hint, expList );
            actList.clear();

            // 提交事务3
            db3.commit();

            // 删除记录
            cl.delete( ( BSONObject ) null );

            // 非事务读
            expList.clear();
            TransUtils.queryAndCheck( cl, "{a:{$lt:" + ( stopId + updateValue )
                    + ",$gte:" + startId + "}}", orderByPos, hint, expList );
            actList.clear();
        } finally {
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
            setTransactionID( cl2.getSequoiadb() );

            cl2.update( "{a: {$gte: " + startId + ", $lt: " + stopId + "}}",
                    "{$inc:{a:" + updateValue + "}}", hint );
        }
    }

    private class TransactionQueryThread extends SdbThreadBase {
        private DBCollection cl = null;
        private String orderBy = null;

        public TransactionQueryThread( DBCollection cl, String orderBy ) {
            super();
            this.cl = cl;
            this.orderBy = orderBy;
        }

        @Override
        public void exec() throws BaseException {
            // 判断事务阻塞需先获取事务id
            setTransactionID( cl.getSequoiadb() );

            List< BSONObject > ret = new ArrayList< BSONObject >();
            DBCursor indexCursor = cl.query( "{a:{$lt:"
                    + ( stopId + updateValue ) + ",$gte:" + startId + "}}",
                    null, orderBy, hint );
            while ( indexCursor.hasNext() ) {
                ret.add( indexCursor.getNext() );
            }
            setExecResult( ret );
        }
    }

}
