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

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @Description seqDB-17821.java
 *              插入与更新并发，更新的记录同时匹配已提交记录及其他事务插入的记录，更新走表扫描，事务提交，过程中读
 * @author luweikang
 * @date 2019年1月15日
 */
@Test(groups = { "rc", "rr" })
public class Transaction17821 extends SdbTestBase {

    private String clName = "transCL_17821";
    private Sequoiadb sdb = null;
    private Sequoiadb sdb1 = null;
    private Sequoiadb sdb2 = null;
    private Sequoiadb sdb3 = null;
    private DBCollection cl = null;
    private DBCollection cl1 = null;
    private DBCollection cl2 = null;
    private DBCollection cl3 = null;
    private BSONObject insertR1 = new BasicBSONObject();
    private BSONObject insertR2 = new BasicBSONObject();
    private BSONObject updateR1 = new BasicBSONObject();
    private BSONObject updateR2 = new BasicBSONObject();
    private List< BSONObject > expDataList = new ArrayList<>();
    private String orderBy1 = "{a: 1, b: -1}";
    private String orderBy2 = "{a: -1, b: 1}";
    private String hintTbScan = "{'': null}";
    private String hintIxScan = "{'': 'a'}";

    @BeforeClass
    public void setUp() {
        sdb = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        sdb1 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        sdb2 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        sdb3 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        insertR1 = ( BSONObject ) JSON
                .parse( "{_id:'insertID17821_1',a:1,b:1,c:1}" );
        insertR2 = ( BSONObject ) JSON
                .parse( "{_id:'insertID17821_2',a:2,b:2,c:2}" );
        updateR1 = ( BSONObject ) JSON
                .parse( "{_id:'insertID17821_1',a:3,b:3,c:1}" );
        updateR2 = ( BSONObject ) JSON
                .parse( "{_id:'insertID17821_2',a:4,b:4,c:2}" );
    }

    @DataProvider(name = "index")
    public Object[][] createIndex() {

        // 第一次非事务读正序查询的预期结果
        List< BSONObject > expPositiveReadList1 = new ArrayList<>();
        expPositiveReadList1.add( insertR2 );
        expPositiveReadList1.add( updateR1 );

        // 第一次非事务读逆序查询的预期结果
        List< BSONObject > expReverseReadList1 = new ArrayList<>();
        expReverseReadList1.add( updateR1 );
        expReverseReadList1.add( insertR2 );

        // 第二次非事务读正序查询的预期结果
        List< BSONObject > expPositiveReadList2 = new ArrayList<>();
        expPositiveReadList2.add( updateR1 );
        expPositiveReadList2.add( updateR2 );

        // 第二次非事务读逆序查询的预期结果
        List< BSONObject > expReverseReadList2 = new ArrayList<>();
        expReverseReadList2.add( updateR2 );
        expReverseReadList2.add( updateR1 );

        // 第一次非事务读正序查询的预期结果
        List< BSONObject > expPositiveReadList3 = new ArrayList<>();
        expPositiveReadList3.add( insertR1 );
        expPositiveReadList3.add( insertR2 );

        // 第一次非事务读逆序查询的预期结果
        List< BSONObject > expReverseReadList3 = new ArrayList<>();
        expReverseReadList3.add( insertR2 );
        expReverseReadList3.add( insertR1 );

        return new Object[][] {
                { "{'a': 1}", expPositiveReadList1, expReverseReadList1,
                        expPositiveReadList2, expReverseReadList2 },
                { "{'a': 1, b: 1}", expPositiveReadList1, expReverseReadList1,
                        expPositiveReadList2, expReverseReadList2 },
                { "{'a': 1, b: -1}", expPositiveReadList1, expReverseReadList1,
                        expPositiveReadList2, expReverseReadList2 },
                { "{'a': -1}", expPositiveReadList1, expReverseReadList1,
                        expPositiveReadList2, expReverseReadList2 },
                { "{'a': -1, b: 1}", expPositiveReadList1, expReverseReadList1,
                        expPositiveReadList2, expReverseReadList2 },
                { "{'a': -1, b: -1}", expPositiveReadList1, expReverseReadList1,
                        expPositiveReadList2, expReverseReadList2 },

        };
    }

    @Test(dataProvider = "index")
    public void test( String indexKey, List< BSONObject > expPositiveReadList1,
            List< BSONObject > expReverseReadList1,
            List< BSONObject > expPositiveReadList2,
            List< BSONObject > expReverseReadList2 )
            throws InterruptedException {
        try {
            // 插入记录R1
            cl.createIndex( "a", indexKey, false, false );

            cl.insert( insertR1 );

            cl1 = sdb1.getCollectionSpace( csName ).getCollection( clName );
            cl2 = sdb2.getCollectionSpace( csName ).getCollection( clName );
            cl3 = sdb3.getCollectionSpace( csName ).getCollection( clName );

            // 开启事务
            TransUtils.beginTransaction( sdb1 );
            TransUtils.beginTransaction( sdb2 );
            TransUtils.beginTransaction( sdb3 );

            // 事务1插入记录
            cl1.insert( insertR2 );

            // 事务2更新记录R1为R3，R2为R4
            UpdateThread updateThread = new UpdateThread();
            updateThread.start();
            Assert.assertTrue( TransUtils.isTransWaitLock( sdb,
                    updateThread.getTransactionID() ) );

            // 事务1记录读，正序查询
            expDataList.clear();
            expDataList.add( insertR1 );
            expDataList.add( insertR2 );
            TransUtils.queryAndCheck( cl1, "{a: 1}", hintTbScan, expDataList );

            // 事务1索引读，正序查询
            TransUtils.queryAndCheck( cl1, "{a: 1}", hintIxScan, expDataList );

            // 事务1记录读，逆序查询
            Collections.reverse( expDataList );
            TransUtils.queryAndCheck( cl1, "{a: -1}", hintTbScan, expDataList );

            // 事务1索引读，逆序查询
            TransUtils.queryAndCheck( cl1, "{a: -1}", hintIxScan, expDataList );

            // 事务3记录读，正序查询
            expDataList.clear();
            expDataList.add( insertR1 );
            TransUtils.queryAndCheck( cl3, "{a: 1}", hintTbScan, expDataList );

            // 事务3索引读，正序查询
            TransUtils.queryAndCheck( cl3, "{a: 1}", hintIxScan, expDataList );

            // 事务3记录读，逆序查询
            TransUtils.queryAndCheck( cl3, "{a: -1}", hintTbScan, expDataList );

            // 事务3索引读，逆序查询
            TransUtils.queryAndCheck( cl3, "{a: -1}", hintIxScan, expDataList );

            // 非事务记录读，正序查询
            TransUtils.queryAndCheck( cl, "{a: 1}", hintTbScan,
                    expPositiveReadList1 );

            // 非事务索引读，正序查询
            TransUtils.queryAndCheck( cl, "{a: 1}", hintIxScan,
                    expPositiveReadList1 );

            // 非事务记录读，逆序查询
            TransUtils.queryAndCheck( cl, "{a: -1}", hintTbScan,
                    expReverseReadList1 );

            // 非事务索引读，逆序查询
            TransUtils.queryAndCheck( cl, "{a: -1}", hintIxScan,
                    expReverseReadList1 );

            // 提交事务1
            sdb1.commit();
            Assert.assertTrue( updateThread.isSuccess(),
                    updateThread.getErrorMsg() );

            // 非事务记录读，正序查询
            TransUtils.queryAndCheck( cl, "{a: 1}", hintTbScan,
                    expPositiveReadList2 );

            // 非事务索引读，正序查询
            TransUtils.queryAndCheck( cl, "{a: 1}", hintIxScan,
                    expPositiveReadList2 );

            // 非事务记录读，逆序查询
            TransUtils.queryAndCheck( cl, "{a: -1}", hintTbScan,
                    expReverseReadList2 );

            // 非事务索引读，逆序查询
            TransUtils.queryAndCheck( cl, "{a: -1}", hintIxScan,
                    expReverseReadList2 );

            // 事务2记录读，正序查询
            expDataList.clear();
            expDataList.add( updateR1 );
            expDataList.add( updateR2 );
            TransUtils.queryAndCheck( cl2, "{a: 1}", hintTbScan, expDataList );

            // 事务2索引读，正序查询
            TransUtils.queryAndCheck( cl2, "{a: 1}", hintIxScan, expDataList );

            // 事务2记录读，逆序查询
            Collections.reverse( expDataList );
            TransUtils.queryAndCheck( cl2, "{a: -1}", hintTbScan, expDataList );

            // 事务2索引读，逆序查询
            TransUtils.queryAndCheck( cl2, "{a: -1}", hintIxScan, expDataList );

            // 事务3记录读，正序查询
            expDataList.clear();
            if ( !"rr".equals( SdbTestBase.testGroup ) ) {
                expDataList.add( insertR1 );
                expDataList.add( insertR2 );
            } else {
                expDataList.add( insertR1 );
            }
            TransUtils.queryAndCheck( cl3, orderBy1, hintTbScan, expDataList );

            // 事务3索引读，正序查询
            TransUtils.queryAndCheck( cl3, orderBy1, hintIxScan, expDataList );

            // 事务3记录读，逆序查询
            Collections.reverse( expDataList );
            TransUtils.queryAndCheck( cl3, orderBy2, hintTbScan, expDataList );

            // 事务3索引读，逆序查询
            TransUtils.queryAndCheck( cl3, orderBy2, hintIxScan, expDataList );

            // 提交事务2
            sdb2.commit();

            // 非事务记录读，正序查询
            TransUtils.queryAndCheck( cl, "{a: 1}", hintTbScan,
                    expPositiveReadList2 );

            // 非事务索引读，正序查询
            TransUtils.queryAndCheck( cl, "{a: 1}", hintIxScan,
                    expPositiveReadList2 );

            // 非事务记录读，逆序查询
            TransUtils.queryAndCheck( cl, "{a: -1}", hintTbScan,
                    expReverseReadList2 );

            // 非事务索引读，逆序查询
            TransUtils.queryAndCheck( cl, "{a: -1}", hintIxScan,
                    expReverseReadList2 );

            // 事务3记录读，正序查询
            expDataList.clear();
            if ( !"rr".equals( SdbTestBase.testGroup ) ) {
                expDataList.add( updateR1 );
                expDataList.add( updateR2 );
            } else {
                expDataList.add( insertR1 );
            }
            TransUtils.queryAndCheck( cl3, "{a: 1}", hintTbScan, expDataList );

            // 事务3索引读，正序查询
            TransUtils.queryAndCheck( cl3, "{a: 1}", hintIxScan, expDataList );

            // 事务3记录读，逆序查询
            Collections.reverse( expDataList );
            TransUtils.queryAndCheck( cl3, "{a: -1}", hintTbScan, expDataList );

            // 事务3索引读，逆序查询
            TransUtils.queryAndCheck( cl3, "{a: -1}", hintIxScan, expDataList );

            // 提交事务3
            sdb3.commit();
        } finally {
            // 关闭事务连接
            sdb1.commit();
            sdb2.commit();
            sdb3.commit();

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
        if ( sdb1 != null ) {
            sdb1.close();
        }
        if ( sdb2 != null ) {
            sdb2.close();
        }
        if ( sdb3 != null ) {
            sdb3.close();
        }

        sdb.getCollectionSpace( csName ).dropCollection( clName );
        if ( sdb != null ) {
            sdb.close();
        }
    }

    private class UpdateThread extends SdbThreadBase {

        @Override
        public void exec() throws BaseException {
            // 判断事务阻塞需先获取事务id
            setTransactionID( cl2.getSequoiadb() );

            cl2.update( null, "{'$inc': {'a': 2, 'b': 2}}", hintTbScan );
        }
    }

}
