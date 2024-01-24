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
 * @testcase seqDB-17943:转账的同时执行总账查询，查询走索引扫描
 * @date 2020-1-16
 * @author zhaoyu
 *
 */

@Test(groups = "rs")
public class Transaction17943 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl17943";
    private String idxName = "idx17943";
    private DBCollection cl = null;
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
        try {

            // 创建索引
            cl.createIndex( idxName, indexKey, false, false );

            // 开启 3 个并发事务
            ThreadExecutor threadExecutor = new ThreadExecutor( 3600000 );
            for ( int i = 0; i < threadNum; i++ ) {
                threadExecutor.addWorker( new UpdateThread() );
                threadExecutor.addWorker( new QueryThread() );
            }
            threadExecutor.run();
        } finally {
            // 删除索引
            cl.dropIndex( idxName );
        }
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
        public void update() {
            try {
                db.setSessionAttr(
                        ( BSONObject ) JSON.parse( "{TransTimeout:5}" ) );
                for ( int i = 0; i < loopNum; i++ ) {
                    int aid = ( int ) ( Math.random() * insertNum );
                    int bid = ( int ) ( Math.random() * insertNum );
                    int value = ( int ) ( Math.random() * 100 ) + 1;

                    // 开启更新事务
                    TransUtils.beginTransaction( db );
                    DBCollection cl = db.getCollectionSpace( csName )
                            .getCollection( clName );
                    try {

                        cl.update( "{b:" + aid + "}",
                                "{$inc:{a:-" + value + "}}",
                                "{'':'" + idxName + "'}" );
                        cl.update( "{b:" + bid + "}",
                                "{$inc:{a:" + value + "}}",
                                "{'':'" + idxName + "'}" );
                    } catch ( BaseException e ) {
                        if ( e.getErrorCode() == -13 ) {
                            continue;
                        } else {
                            throw e;
                        }
                    }
                    // 提交、回滚更新事务
                    if ( aid % 2 == 0 ) {
                        db.commit();
                    } else {
                        db.rollback();
                    }

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
        public void query() throws Exception {
            try {
                db.setSessionAttr(
                        ( BSONObject ) JSON.parse( "{TransTimeout:5}" ) );
                for ( int i = 0; i < loopNum; i++ ) {

                    // 开启查询事务，索引扫描
                    TransUtils.beginTransaction( db );
                    String sqlTblScan = "select sum(a) as sum from " + csName
                            + "." + clName + " /*+use_index(" + idxName + ")*/";
                    ArrayList< BSONObject > actNums;
                    try {
                        DBCursor cursor = db.exec( sqlTblScan );
                        actNums = TransUtils.getReadActList( cursor );
                    } catch ( BaseException e ) {
                        if ( e.getErrorCode() == -13 ) {
                            continue;
                        } else {
                            throw e;
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
}
