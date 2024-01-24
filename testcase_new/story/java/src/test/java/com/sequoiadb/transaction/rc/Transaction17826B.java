package com.sequoiadb.transaction.rc;

import java.util.ArrayList;
import java.util.Collections;
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
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @FileName:seqDB-17826：删除与更新并发，删除的记录同时匹配已提交记录及其他事务更新的记录，事务提交，过程中读 更新/删除走表扫描,
 *                                                                  先插入R2，再插入R1
 * @Author zhaoyu
 * @Date 2019-01-29
 * @Version 1.00
 */
@Test(groups = { "rc", "rr" })
public class Transaction17826B extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl_17826B";
    private Sequoiadb db1;
    private Sequoiadb db2;
    private Sequoiadb db3;
    private DBCollection cl1 = null;
    private DBCollection cl2 = null;
    private DBCollection cl3 = null;
    private DBCollection cl = null;
    private BSONObject insertR1 = new BasicBSONObject();
    private BSONObject insertR2 = new BasicBSONObject();
    private BSONObject updateR2 = new BasicBSONObject();
    private ArrayList< BSONObject > expList = new ArrayList<>();
    private String hintTbScan = "{\"\":null}";
    private String hintIxScan = "{\"\":\"a\"}";
    private String orderBy1 = "{a: 1, b: -1}";
    private String orderBy2 = "{a: -1, b: 1}";

    @BeforeClass
    public void setUp() {
        sdb = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        db1 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        db2 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        db3 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        insertR1 = ( BSONObject ) JSON
                .parse( "{_id:'insertID17826A_1',a:1,b:1,c:1}" );
        insertR2 = ( BSONObject ) JSON
                .parse( "{_id:'insertID17826A_2',a:2,b:2,c:2}" );
        updateR2 = ( BSONObject ) JSON
                .parse( "{_id:'insertID17826A_2',a:3,b:3,c:2}" );
    }

    @DataProvider(name = "index")
    public Object[][] createIndex() {

        // 第一次非事务读查询的预期结果
        List< BSONObject > expReadList = new ArrayList<>();
        expReadList.add( updateR2 );

        return new Object[][] { { "{'a': 1}", expReadList },
                { "{'a': 1, b: 1}", expReadList },
                { "{'a': 1, b: -1}", expReadList },
                { "{'a': -1}", expReadList },
                { "{'a': -1, b: 1}", expReadList },
                { "{'a': -1, b: -1}", expReadList },

        };
    }

    @Test(dataProvider = "index")
    public void test( String indexKey, List< BSONObject > expReadList )
            throws InterruptedException {
        try {
            // 插入记录R1、R2
            cl.createIndex( "a", indexKey, false, false );

            cl.insert( insertR2 );
            cl.insert( insertR1 );

            // 开启3个并发事务
            TransUtils.beginTransaction( db1 );
            TransUtils.beginTransaction( db2 );
            TransUtils.beginTransaction( db3 );
            cl1 = db1.getCollectionSpace( csName ).getCollection( clName );
            cl2 = db2.getCollectionSpace( csName ).getCollection( clName );
            cl3 = db3.getCollectionSpace( csName ).getCollection( clName );

            // 事务1删除记录R1
            cl1.delete( "{a:1}", hintTbScan );

            // 事务2匹配记录R1、R2更新为R3、R4,R1<R2<R3
            UpdateThread updateThread = new UpdateThread();
            updateThread.start();
            Assert.assertTrue( TransUtils.isTransWaitLock( sdb,
                    updateThread.getTransactionID() ) );

            // 事务1正序记录读
            expList.clear();
            expList.add( insertR2 );
            TransUtils.queryAndCheck( cl1, orderBy1, hintTbScan, expList );

            // 事务1正序索引读
            TransUtils.queryAndCheck( cl1, orderBy1, hintIxScan, expList );

            // 事务1逆序记录读
            TransUtils.queryAndCheck( cl1, orderBy2, hintTbScan, expList );

            // 事务1逆序索引读
            TransUtils.queryAndCheck( cl1, orderBy2, hintIxScan, expList );

            // 事务3正序记录读
            expList.clear();
            expList.add( insertR1 );
            expList.add( insertR2 );
            TransUtils.queryAndCheck( cl3, orderBy1, hintTbScan, expList );

            // 事务3正序索引读
            TransUtils.queryAndCheck( cl3, orderBy1, hintIxScan, expList );

            // 事务3逆序记录读
            Collections.reverse( expList );
            TransUtils.queryAndCheck( cl3, orderBy2, hintTbScan, expList );

            // 事务3逆序索引读
            TransUtils.queryAndCheck( cl3, orderBy2, hintIxScan, expList );

            // 非事务正序记录读
            TransUtils.queryAndCheck( cl, orderBy1, hintTbScan, expReadList );

            // 非事务正序索引读
            TransUtils.queryAndCheck( cl, orderBy1, hintIxScan, expReadList );

            // 非事务逆序记录读
            TransUtils.queryAndCheck( cl, orderBy2, hintTbScan, expReadList );

            // 非事务逆序索引读
            TransUtils.queryAndCheck( cl, orderBy2, hintIxScan, expReadList );

            // 提交事务1
            db1.commit();
            Assert.assertTrue( updateThread.isSuccess(),
                    updateThread.getErrorMsg() );

            // 非事务正序记录读
            TransUtils.queryAndCheck( cl, orderBy1, hintTbScan, expReadList );

            // 非事务正序索引读
            TransUtils.queryAndCheck( cl, orderBy1, hintIxScan, expReadList );

            // 非事务逆序记录读
            TransUtils.queryAndCheck( cl, orderBy2, hintTbScan, expReadList );

            // 非事务逆序索引读
            TransUtils.queryAndCheck( cl, orderBy2, hintIxScan, expReadList );

            // 事务2正序记录读
            expList.clear();
            if ( !"rr".equals( SdbTestBase.testGroup ) ) {
                expList.add( updateR2 );
            } else {
                expList.add( insertR1 );
                expList.add( updateR2 );
            }
            TransUtils.queryAndCheck( cl2, orderBy1, hintTbScan, expList );

            // 事务2正序索引读
            TransUtils.queryAndCheck( cl2, orderBy1, hintIxScan, expList );

            // 事务2逆序记录读
            Collections.reverse( expList );
            TransUtils.queryAndCheck( cl2, orderBy2, hintTbScan, expList );

            // 事务2逆序索引读
            TransUtils.queryAndCheck( cl2, orderBy2, hintIxScan, expList );

            // 事务3正序记录读
            expList.clear();
            if ( !"rr".equals( SdbTestBase.testGroup ) ) {
                expList.add( insertR2 );
            } else {
                expList.add( insertR1 );
                expList.add( insertR2 );
            }
            TransUtils.queryAndCheck( cl3, orderBy1, hintTbScan, expList );

            // 事务3正序索引读
            TransUtils.queryAndCheck( cl3, orderBy1, hintIxScan, expList );

            // 事务3逆序记录读
            Collections.reverse( expList );
            TransUtils.queryAndCheck( cl3, orderBy2, hintTbScan, expList );

            // 事务3逆序索引读
            TransUtils.queryAndCheck( cl3, orderBy2, hintIxScan, expList );

            // 提交事务2
            db2.commit();

            // 非事务正序记录读
            TransUtils.queryAndCheck( cl, orderBy1, hintTbScan, expReadList );

            // 非事务正序索引读
            TransUtils.queryAndCheck( cl, orderBy1, hintIxScan, expReadList );

            // 非事务逆序记录读
            TransUtils.queryAndCheck( cl, orderBy2, hintTbScan, expReadList );

            // 非事务逆序索引读
            TransUtils.queryAndCheck( cl, orderBy2, hintIxScan, expReadList );

            // 事务3正序记录读
            expList.clear();
            if ( !"rr".equals( SdbTestBase.testGroup ) ) {
                expList.add( updateR2 );
            } else {
                expList.add( insertR1 );
                expList.add( insertR2 );
            }
            TransUtils.queryAndCheck( cl3, orderBy1, hintTbScan, expList );

            // 事务3正序索引读
            TransUtils.queryAndCheck( cl3, orderBy1, hintIxScan, expList );

            // 事务3逆序记录读
            Collections.reverse( expList );
            TransUtils.queryAndCheck( cl3, orderBy2, hintTbScan, expList );

            // 事务3逆序索引读
            TransUtils.queryAndCheck( cl3, orderBy2, hintIxScan, expList );

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

    private class UpdateThread extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            // 判断事务阻塞需先获取事务id
            setTransactionID( cl2.getSequoiadb() );

            cl2.update( null, "{$set:{a:3,b:3}}", hintTbScan );
        }
    }
}
