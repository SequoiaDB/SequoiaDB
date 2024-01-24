package com.sequoiadb.transaction.rs;

import java.util.ArrayList;
import java.util.Date;
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
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;
import com.sequoiadb.transaction.TransUtils;

/**
 * @testcase seqDB-17960:转账的同时查询总账（索引扫描），创建删除索引
 * @date 2020-1-16
 * @author zhaoyu
 *
 */

@Test(groups = "rs")
public class Transaction17960A extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl17960A";
    private String idxName = "idx17960A";
    private DBCollection cl = null;
    private String indexKey = null;
    private int insertNum = 100;
    private int loopNum = 100;
    // 经过实际测试，由于写操作优先于读操作，设置并发数会导致读操作极少，测试点覆盖不到，并发数暂时设置为1
    private int threadNum = 1;
    private int expSum = 1000000;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        insertData();
    }

    @AfterClass
    public void tearDown() {
        CollectionSpace cs = sdb.getCollectionSpace( csName );
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }
        if ( !sdb.isClosed() ) {
            sdb.close();
        }
    }

    @DataProvider(name = "index")
    public Object[][] createIndex() {
        return new Object[][] { { "{'b':-1}" }, { "{'b':1}" } };
    }

    @Test(dataProvider = "index")
    public void test( String indexKey ) throws Exception {
        this.indexKey = indexKey;

        // 开启 3 个并发事务
        ThreadExecutor threadExecutor = new ThreadExecutor( 3600000 );
        for ( int i = 0; i < threadNum; i++ ) {
            threadExecutor.addWorker( new UpdateThread() );
            threadExecutor.addWorker( new QueryThread() );
        }
        threadExecutor.addWorker( new DropIndexThread() );
        threadExecutor.run();
    }

    private void insertData() {
        List< BSONObject > records = new ArrayList< BSONObject >();
        for ( int i = 0; i < insertNum; i++ ) {
            BSONObject object = ( BSONObject ) JSON
                    .parse( "{_id:" + i + ", a:10000, b:" + i + "}" );
            records.add( object );
        }
        cl.insert( records );
    }

    private class UpdateThread {
        private Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

        @ExecuteOrder(step = 1, desc = "转账")
        private void update() {
            try {
                db.setSessionAttr(
                        ( BSONObject ) JSON.parse( "{TransTimeout:5}" ) );
                for ( int i = 0; i < loopNum / 4; i++ ) {
                    int aid = ( int ) ( Math.random() * insertNum );
                    int bid = ( int ) ( Math.random() * insertNum );
                    int value = ( int ) ( Math.random() * 100 ) + 1;

                    // 开启更新事务
                    TransUtils.beginTransaction( db );
                    DBCollection cl = db.getCollectionSpace( csName )
                            .getCollection( clName );

                    // 由于更新和读存在死锁，因此需要规避此问题
                    try {
                        cl.update( "{b:" + aid + "}",
                                "{$inc:{a:-" + value + "}}",
                                "{'':'" + idxName + "'}" );
                        cl.update( "{b:" + bid + "}",
                                "{$inc:{a:" + value + "}}",
                                "{'':'" + idxName + "'}" );
                    } catch ( BaseException e ) {
                        if ( e.getErrorCode() == -48 || e.getErrorCode() == -47
                                || e.getErrorCode() == -52
                                || e.getErrorCode() == -10
                                || e.getErrorCode() == -199
                                || e.getErrorCode() == -13 ) {
                            db.rollback();
                            continue;
                        } else {
                            e.printStackTrace();
                            throw e;
                        }
                    }
                    // 提交更新事务
                    db.commit();
                }
            } finally {
                db.commit();
                db.close();
                System.out.println( "testcase: "
                        + new Exception().getStackTrace()[ 0 ].getClassName()
                        + " udpate thread end" + new Date() );
            }
        }
    }

    private class QueryThread {
        private Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

        @ExecuteOrder(step = 1, desc = "查询记录总账")
        private void query() throws Exception {
            try {
                db.setSessionAttr(
                        ( BSONObject ) JSON.parse( "{TransTimeout:5}" ) );
                for ( int i = 0; i < loopNum / 4; i++ ) {

                    // 开启查询事务，索引扫描
                    TransUtils.beginTransaction( db );
                    String sqlTblScan = "select sum(a) as sum from " + csName
                            + "." + clName + " /*+use_index(" + idxName + ")*/";
                    ArrayList< BSONObject > actNums = null;
                    try {
                        DBCursor cursor = db.exec( sqlTblScan );
                        actNums = TransUtils.getReadActList( cursor );
                    } catch ( BaseException e ) {
                        if ( e.getErrorCode() == -48 || e.getErrorCode() == -47
                                || e.getErrorCode() == -52
                                || e.getErrorCode() == -10
                                || e.getErrorCode() == -199
                                || e.getErrorCode() == -13 ) {
                            db.rollback();
                            continue;
                        } else {
                            Assert.fail( e.getMessage() );
                        }
                    }

                    Assert.assertEquals( actNums.size(), 1 );
                    double sumValue = ( double ) actNums.get( 0 ).get( "sum" );
                    int sum = ( int ) sumValue;
                    db.commit();
                    if ( sum != expSum ) {
                        throw new Exception(
                                "IdxScan check sum error, expect sum is "
                                        + expSum + ", but actual sum:" + +sum );
                    }
                }
            } finally {
                db.commit();
                db.closeAllCursors();
                db.close();
                System.out.println( "testcase: "
                        + new Exception().getStackTrace()[ 0 ].getClassName()
                        + " query thread end" + new Date() );
            }
        }
    }

    private class DropIndexThread {
        private Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

        @ExecuteOrder(step = 1, desc = "删除索引")
        private void dropIndex() {
            try {
                for ( int i = 0; i < loopNum; i++ ) {
                    DBCollection cl = db.getCollectionSpace( csName )
                            .getCollection( clName );
                    cl.createIndex( idxName, indexKey, false, false );
                    Assert.assertTrue( cl.isIndexExist( idxName ) );
                    cl.dropIndex( idxName );
                    Assert.assertFalse( cl.isIndexExist( idxName ) );

                }
            } finally {
                db.commit();
                db.close();
                System.out.println( "testcase: "
                        + new Exception().getStackTrace()[ 0 ].getClassName()
                        + " create drop index thread end" + new Date() );
            }
        }
    }
}
