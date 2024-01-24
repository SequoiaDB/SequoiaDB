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
 * @Description seqDB-17360.java 插入与删除并发，
 *              删除的记录同时匹配已提交记录及其他事务插入的记录，删除走索引，事务提交，过程中读 R1>R2
 * @author luweikang
 * @date 2019年1月15日
 */
@Test(groups = { "rc", "rr" })
public class Transaction17360A extends SdbTestBase {

    private String clName = "transCL_17360A";
    private Sequoiadb sdb = null;
    private Sequoiadb sdb1 = null;
    private Sequoiadb sdb2 = null;
    private Sequoiadb sdb3 = null;
    private DBCollection cl1 = null;
    private DBCollection cl2 = null;
    private DBCollection cl3 = null;
    private DBCollection cl = null;
    private BSONObject insertR1 = new BasicBSONObject();
    private BSONObject insertR2 = new BasicBSONObject();
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
                .parse( "{_id:'insertID17360A_1',a:2,b:2,c:2}" );
        insertR2 = ( BSONObject ) JSON
                .parse( "{_id:'insertID17360A_2',a:1,b:1,c:1}" );

    }

    @DataProvider(name = "index")
    public Object[][] createIndex() {

        // 第一次非事务读正序查询的预期结果
        List< BSONObject > expPositiveReadList1 = new ArrayList<>();
        expPositiveReadList1.add( insertR2 );
        expPositiveReadList1.add( insertR1 );

        // 第一次非事务读逆序查询的预期结果
        List< BSONObject > expReverseReadList1 = new ArrayList<>();
        expReverseReadList1.add( insertR1 );
        expReverseReadList1.add( insertR2 );

        // 第一次非事务读的预期结果
        List< BSONObject > expReadList3 = new ArrayList<>();
        expReadList3.add( insertR2 );

        return new Object[][] {
                { "{'a': 1}", expPositiveReadList1, expReverseReadList1 },
                { "{'a': 1, b: 1}", expPositiveReadList1, expReverseReadList1 },
                { "{'a': 1, b: -1}", expPositiveReadList1,
                        expReverseReadList1 },
                { "{'a': -1}", expReadList3, expReadList3 },
                { "{'a': -1, b: 1}", expReadList3, expReadList3 },
                { "{'a': -1, b: -1}", expReadList3, expReadList3 },

        };
    }

    @Test(dataProvider = "index")
    public void test( String indexKey, List< BSONObject > expPositiveReadList1,
            List< BSONObject > expReverseReadList1 )
            throws InterruptedException {
        try {
            // 插入记录R1
            cl.createIndex( "a", indexKey, false, false );

            cl.insert( insertR1 );

            cl1 = sdb1.getCollectionSpace( csName ).getCollection( clName );
            cl2 = sdb2.getCollectionSpace( csName ).getCollection( clName );
            cl3 = sdb3.getCollectionSpace( csName ).getCollection( clName );

            TransUtils.beginTransaction( sdb1 );
            TransUtils.beginTransaction( sdb2 );
            TransUtils.beginTransaction( sdb3 );

            // 事务1插入记录R2，R1>R2
            cl1.insert( insertR2 );

            // 事务2删除R1及R2
            DeleteThread deleteThread = new DeleteThread();
            deleteThread.start();
            Assert.assertTrue( TransUtils.isTransWaitLock( sdb,
                    deleteThread.getTransactionID() ) );

            // 事务1正序记录读
            expDataList.clear();
            expDataList.add( insertR2 );
            expDataList.add( insertR1 );
            TransUtils.queryAndCheck( cl1, orderBy1, hintTbScan, expDataList );

            // 事务1正序索引读
            TransUtils.queryAndCheck( cl1, orderBy1, hintIxScan, expDataList );

            // 事务1逆序记录读
            Collections.reverse( expDataList );
            TransUtils.queryAndCheck( cl1, orderBy2, hintTbScan, expDataList );

            // 事务1逆序索引读
            TransUtils.queryAndCheck( cl1, orderBy2, hintIxScan, expDataList );

            // 事务3正序记录读
            expDataList.clear();
            expDataList.add( insertR1 );
            TransUtils.queryAndCheck( cl3, orderBy1, hintTbScan, expDataList );

            // 事务3正序索引读
            TransUtils.queryAndCheck( cl3, orderBy1, hintIxScan, expDataList );

            // 事务3逆序记录读
            TransUtils.queryAndCheck( cl3, orderBy2, hintTbScan, expDataList );

            // 事务3逆序索引读
            TransUtils.queryAndCheck( cl3, orderBy2, hintIxScan, expDataList );

            // 非事务正序记录读
            TransUtils.queryAndCheck( cl, orderBy1, hintTbScan,
                    expPositiveReadList1 );

            // 非事务正序索引读
            TransUtils.queryAndCheck( cl, orderBy1, hintIxScan,
                    expPositiveReadList1 );

            // 非事务逆序记录读
            TransUtils.queryAndCheck( cl, orderBy2, hintTbScan,
                    expReverseReadList1 );

            // 非事务逆序索引读
            TransUtils.queryAndCheck( cl, orderBy2, hintIxScan,
                    expReverseReadList1 );

            // 提交事务1
            sdb1.commit();
            Assert.assertTrue( deleteThread.isSuccess(),
                    deleteThread.getErrorMsg() );

            // 非事务读
            Assert.assertEquals( cl.getCount(
                    new BasicBSONObject( "a",
                            new BasicBSONObject( "$isnull", 0 ) ),
                    new BasicBSONObject( "", null ) ), 0 );
            Assert.assertEquals( cl.getCount(
                    new BasicBSONObject( "a",
                            new BasicBSONObject( "$isnull", 0 ) ),
                    new BasicBSONObject( "", "a" ) ), 0 );

            // 事务2读
            Assert.assertEquals( cl2.getCount(
                    new BasicBSONObject( "a",
                            new BasicBSONObject( "$isnull", 0 ) ),
                    new BasicBSONObject( "", null ) ), 0 );
            Assert.assertEquals( cl2.getCount(
                    new BasicBSONObject( "a",
                            new BasicBSONObject( "$isnull", 0 ) ),
                    new BasicBSONObject( "", "a" ) ), 0 );

            // 事务3正序记录读
            expDataList.clear();
            if ( !"rr".equals( SdbTestBase.testGroup ) ) {
                expDataList.add( insertR2 );
                expDataList.add( insertR1 );
            } else {
                expDataList.add( insertR1 );
            }

            TransUtils.queryAndCheck( cl3, orderBy1, hintTbScan, expDataList );

            // 事务3正序索引读
            TransUtils.queryAndCheck( cl3, orderBy1, hintIxScan, expDataList );

            // 事务3逆序记录读
            Collections.reverse( expDataList );
            TransUtils.queryAndCheck( cl3, orderBy2, hintTbScan, expDataList );

            // 事务3逆序索引读
            TransUtils.queryAndCheck( cl3, orderBy2, hintIxScan, expDataList );

            // 提交事务2
            sdb2.commit();

            // 非事务读
            Assert.assertEquals( cl.getCount(
                    new BasicBSONObject( "a",
                            new BasicBSONObject( "$isnull", 0 ) ),
                    new BasicBSONObject( "", null ) ), 0 );
            Assert.assertEquals( cl.getCount(
                    new BasicBSONObject( "a",
                            new BasicBSONObject( "$isnull", 0 ) ),
                    new BasicBSONObject( "", "a" ) ), 0 );

            // 事务3读
            expDataList.clear();
            if ( !"rr".equals( SdbTestBase.testGroup ) ) {
            } else {
                expDataList.add( insertR1 );
            }

            TransUtils.queryAndCheck( cl3, orderBy1, hintTbScan, expDataList );

            // 事务3正序索引读
            TransUtils.queryAndCheck( cl3, orderBy1, hintIxScan, expDataList );

            // 事务3逆序记录读
            Collections.reverse( expDataList );
            TransUtils.queryAndCheck( cl3, orderBy2, hintTbScan, expDataList );

            // 事务3逆序索引读
            TransUtils.queryAndCheck( cl3, orderBy2, hintIxScan, expDataList );

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

    private class DeleteThread extends SdbThreadBase {

        @Override
        public void exec() throws BaseException {
            // 判断事务阻塞需先获取事务id
            setTransactionID( cl2.getSequoiadb() );

            cl2.delete( null, hintIxScan );
        }
    }

}
