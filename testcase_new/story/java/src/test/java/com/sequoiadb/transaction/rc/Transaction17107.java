package com.sequoiadb.transaction.rc;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @FileName: seqDB-17107:删除与插入并发，事务提交，过程中读 更新/删除走索引扫描
 * @Author zhaoyu
 * @Date 2019-01-21
 * @Version 1.00
 */
@Test(groups = { "rc", "rr" })
public class Transaction17107 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private Sequoiadb db1;
    private Sequoiadb db2;
    private Sequoiadb db3;
    private DBCollection cl = null;
    private DBCollection cl1 = null;
    private DBCollection cl2 = null;
    private DBCollection cl3 = null;
    private ArrayList< BSONObject > expList1 = new ArrayList<>();
    private ArrayList< BSONObject > expList2 = new ArrayList<>();
    private ArrayList< BSONObject > expList3 = new ArrayList<>();
    private ArrayList< BSONObject > expList4 = new ArrayList<>();
    private int startId = 0;
    private int stopId = 1000;
    private int startId2 = 1000;
    private int stopId2 = 2000;
    private String hashCLName = "cl17107_hash";
    private String mainCLName = "cl17107_main";
    private String subCLName1 = "subcl17107_1";
    private String subCLName2 = "subcl17107_2";
    private String hintTbScan = "{\"\":null}";
    private String hintIxScan = "{\"\":\"a\"}";
    private String orderByPos = "{a:1}";
    private String orderByRev = "{a: -1}";

    @DataProvider(name = "index")
    public Object[][] createIndex() {
        return new Object[][] { { "{'a': 1}", hashCLName },
                { "{'a': -1, 'b': 1}", mainCLName }, };
    }

    @BeforeClass
    public void setUp() {
        sdb = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        db1 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        db2 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        db3 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "STANDALONE MODE" );
        }
        if ( CommLib.OneGroupMode( sdb ) ) {
            throw new SkipException( "ONE GROUP MODE" );
        }
        TransUtils.createCLs( sdb, csName, hashCLName, mainCLName, subCLName1,
                subCLName2, 1000 );
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
        if ( cs.isCollectionExist( hashCLName ) ) {
            cs.dropCollection( hashCLName );
        }
        if ( cs.isCollectionExist( mainCLName ) ) {
            cs.dropCollection( mainCLName );
        }
        if ( sdb != null ) {
            sdb.close();
        }
    }

    @Test(dataProvider = "index")
    public void test( String indexKey, String clName )
            throws InterruptedException {
        try {
            cl = sdb.getCollectionSpace( csName ).getCollection( clName );
            cl.createIndex( "a", indexKey, false, false );

            // 开启3个并发事务
            TransUtils.beginTransaction( db1 );
            TransUtils.beginTransaction( db2 );
            TransUtils.beginTransaction( db3 );
            cl1 = db1.getCollectionSpace( csName ).getCollection( clName );
            cl2 = db2.getCollectionSpace( csName ).getCollection( clName );
            cl3 = db3.getCollectionSpace( csName ).getCollection( clName );

            // 1 插入记录R1
            ArrayList< BSONObject > insertR1s = TransUtils
                    .insertRandomDatas( cl, startId, stopId );

            // 2 事务1匹配R1删除
            cl1.delete( null, hintIxScan );

            // 3 事务2插入R2，与R1相同
            ArrayList< BSONObject > insertR2s = this.insertRandomDatas( cl2,
                    startId2, stopId2, 0 );

            // 4 事务1记录读
            expList1.clear();
            TransUtils.queryAndCheck( cl1,
                    "{a:{$lt:" + stopId2 + ",$gte:" + startId + "}}",
                    orderByPos, hintTbScan, expList1 );

            // 事务1索引读
            TransUtils.queryAndCheck( cl1,
                    "{a:{$lt:" + stopId2 + ",$gte:" + startId + "}}",
                    orderByPos, hintIxScan, expList1 );

            // 4 事务1记录逆序读
            TransUtils.queryAndCheck( cl1,
                    "{a:{$lt:" + stopId2 + ",$gte:" + startId + "}}",
                    orderByRev, hintTbScan, expList1 );

            // 事务1索引逆序读
            TransUtils.queryAndCheck( cl1,
                    "{a:{$lt:" + stopId2 + ",$gte:" + startId + "}}",
                    orderByRev, hintIxScan, expList1 );

            // 5 事务2记录读
            expList2.clear();
            expList2.addAll( insertR1s );
            expList2.addAll( insertR2s );
            Collections.sort( expList2, new OrderBy() );
            TransUtils.queryAndCheck( cl2,
                    "{a:{$lt:" + stopId2 + ",$gte:" + startId + "}}",
                    "{a:1, b:1}", hintTbScan, expList2 );

            // 事务2索引读
            TransUtils.queryAndCheck( cl2,
                    "{a:{$lt:" + stopId2 + ",$gte:" + startId + "}}",
                    "{a:1, b:1}", hintIxScan, expList2 );

            // 5 事务2记录逆序读
            Collections.reverse( expList2 );
            TransUtils.queryAndCheck( cl2,
                    "{a:{$lt:" + stopId2 + ",$gte:" + startId + "}}",
                    "{a: -1, b: -1}", hintTbScan, expList2 );

            // 事务2索引逆序读
            TransUtils.queryAndCheck( cl2,
                    "{a:{$lt:" + stopId2 + ",$gte:" + startId + "}}",
                    "{a: -1, b: -1}", hintIxScan, expList2 );

            // 6 事务3记录读
            expList3.clear();
            expList3.addAll( insertR1s );
            TransUtils.queryAndCheck( cl3,
                    "{a:{$lt:" + stopId2 + ",$gte:" + startId + "}}",
                    orderByPos, hintTbScan, expList3 );

            // 事务3索引读
            TransUtils.queryAndCheck( cl3,
                    "{a:{$lt:" + stopId2 + ",$gte:" + startId + "}}",
                    orderByPos, hintIxScan, expList3 );

            // 6 事务3记录读
            Collections.reverse( expList3 );
            TransUtils.queryAndCheck( cl3,
                    "{a:{$lt:" + stopId2 + ",$gte:" + startId + "}}",
                    orderByRev, hintTbScan, expList3 );

            // 事务3索引逆序读
            TransUtils.queryAndCheck( cl3,
                    "{a:{$lt:" + stopId2 + ",$gte:" + startId + "}}",
                    orderByRev, hintIxScan, expList3 );

            // 7 非事务记录读
            expList4.clear();
            expList4.addAll( insertR2s );
            TransUtils.queryAndCheck( cl,
                    "{a:{$lt:" + stopId2 + ",$gte:" + startId + "}}",
                    orderByPos, hintTbScan, expList4 );

            // 非事务索引读
            TransUtils.queryAndCheck( cl,
                    "{a:{$lt:" + stopId2 + ",$gte:" + startId + "}}",
                    orderByPos, hintIxScan, expList4 );

            // 7 非事务记录逆序读
            Collections.reverse( expList4 );
            TransUtils.queryAndCheck( cl,
                    "{a:{$lt:" + stopId2 + ",$gte:" + startId + "}}",
                    orderByRev, hintTbScan, expList4 );

            // 非事务索引逆序读
            TransUtils.queryAndCheck( cl,
                    "{a:{$lt:" + stopId2 + ",$gte:" + startId + "}}",
                    orderByRev, hintIxScan, expList4 );

            // 8 提交事务1
            db1.commit();

            // 8 非事务记录读
            Collections.reverse( expList4 );
            TransUtils.queryAndCheck( cl,
                    "{a:{$lt:" + stopId2 + ",$gte:" + startId + "}}",
                    orderByPos, hintTbScan, expList4 );

            // 非事务索引读
            TransUtils.queryAndCheck( cl,
                    "{a:{$lt:" + stopId2 + ",$gte:" + startId + "}}",
                    orderByPos, hintIxScan, expList4 );

            // 8 非事务记录逆序读
            Collections.reverse( expList4 );
            TransUtils.queryAndCheck( cl,
                    "{a:{$lt:" + stopId2 + ",$gte:" + startId + "}}",
                    orderByRev, hintTbScan, expList4 );

            // 非事务索引逆序读
            TransUtils.queryAndCheck( cl,
                    "{a:{$lt:" + stopId2 + ",$gte:" + startId + "}}",
                    orderByRev, hintIxScan, expList4 );

            // 9 事务2记录读
            if ( !"rr".equals( SdbTestBase.testGroup ) ) {
                expList2.clear();
                expList2.addAll( insertR2s );
            } else {
                Collections.reverse( expList2 );
            }

            TransUtils.queryAndCheck( cl2,
                    "{a:{$lt:" + stopId2 + ",$gte:" + startId + "}}",
                    orderByPos, hintTbScan, expList2 );

            // 事务2索引读
            TransUtils.queryAndCheck( cl2,
                    "{a:{$lt:" + stopId2 + ",$gte:" + startId + "}}",
                    orderByPos, hintIxScan, expList2 );

            // 9 事务2记录逆序读
            Collections.reverse( expList2 );
            TransUtils.queryAndCheck( cl2,
                    "{a:{$lt:" + stopId2 + ",$gte:" + startId + "}}",
                    orderByRev, hintTbScan, expList2 );

            // 事务2索引逆序读
            TransUtils.queryAndCheck( cl2,
                    "{a:{$lt:" + stopId2 + ",$gte:" + startId + "}}",
                    orderByRev, hintIxScan, expList2 );

            // 10 事务3记录读
            if ( !"rr".equals( SdbTestBase.testGroup ) ) {
                expList3.clear();
            } else {
                Collections.reverse( expList3 );
            }

            TransUtils.queryAndCheck( cl3,
                    "{a:{$lt:" + stopId2 + ",$gte:" + startId + "}}",
                    orderByPos, hintTbScan, expList3 );

            // 事务3索引读
            TransUtils.queryAndCheck( cl3,
                    "{a:{$lt:" + stopId2 + ",$gte:" + startId + "}}",
                    orderByPos, hintIxScan, expList3 );

            // 10 事务3记录逆序读
            Collections.reverse( expList3 );
            TransUtils.queryAndCheck( cl3,
                    "{a:{$lt:" + stopId2 + ",$gte:" + startId + "}}",
                    orderByRev, hintTbScan, expList3 );

            // 事务3索引逆序读
            TransUtils.queryAndCheck( cl3,
                    "{a:{$lt:" + stopId2 + ",$gte:" + startId + "}}",
                    orderByRev, hintIxScan, expList3 );

            // 11 提交事务2
            db2.commit();

            // 11 非事务记录读
            expList4.clear();
            expList4.addAll( insertR2s );
            TransUtils.queryAndCheck( cl,
                    "{a:{$lt:" + stopId2 + ",$gte:" + startId + "}}",
                    orderByPos, hintTbScan, expList4 );

            // 非事务索引读
            TransUtils.queryAndCheck( cl,
                    "{a:{$lt:" + stopId2 + ",$gte:" + startId + "}}",
                    orderByPos, hintIxScan, expList4 );

            // 11 非事务记录逆序读
            Collections.reverse( expList4 );
            TransUtils.queryAndCheck( cl,
                    "{a:{$lt:" + stopId2 + ",$gte:" + startId + "}}",
                    orderByRev, hintTbScan, expList4 );

            // 非事务索引逆序读
            TransUtils.queryAndCheck( cl,
                    "{a:{$lt:" + stopId2 + ",$gte:" + startId + "}}",
                    orderByRev, hintIxScan, expList4 );

            // 12 事务3记录读
            if ( !"rr".equals( SdbTestBase.testGroup ) ) {
                expList3.clear();
                expList3.addAll( insertR2s );
            } else {
                Collections.reverse( expList3 );
            }

            TransUtils.queryAndCheck( cl3,
                    "{a:{$lt:" + stopId2 + ",$gte:" + startId + "}}",
                    orderByPos, hintTbScan, expList3 );

            // 事务3索引读
            TransUtils.queryAndCheck( cl3,
                    "{a:{$lt:" + stopId2 + ",$gte:" + startId + "}}",
                    orderByPos, hintIxScan, expList3 );

            // 12 事务3记录逆序读
            Collections.reverse( expList3 );
            TransUtils.queryAndCheck( cl3,
                    "{a:{$lt:" + stopId2 + ",$gte:" + startId + "}}",
                    orderByRev, hintTbScan, expList3 );

            // 事务3索引逆序读
            TransUtils.queryAndCheck( cl3,
                    "{a:{$lt:" + stopId2 + ",$gte:" + startId + "}}",
                    orderByRev, hintIxScan, expList3 );

            // 提交事务3
            db3.commit();

            // 删除记录
            cl.delete( ( BSONObject ) null );

            // 非事务记录读
            expList4.clear();
            TransUtils.queryAndCheck( cl,
                    "{a:{$lt:" + stopId2 + ",$gte:" + startId + "}}",
                    orderByPos, hintTbScan, expList4 );

            // 非事务索引读
            TransUtils.queryAndCheck( cl,
                    "{a:{$lt:" + stopId2 + ",$gte:" + startId + "}}",
                    orderByPos, hintIxScan, expList4 );
        } finally {
            if ( cl.isIndexExist( "a" ) ) {
                cl.dropIndex( "a" );
            }
            cl.truncate();
        }
    }

    private ArrayList< BSONObject > insertRandomDatas( DBCollection cl,
            int startId, int endId, int startValue ) throws BaseException {
        ArrayList< BSONObject > insertDatas = new ArrayList<>();
        ArrayList< BSONObject > expDatas = new ArrayList<>();
        for ( int i = startId; i < endId; i++ ) {
            BSONObject data = ( BSONObject ) JSON.parse( "{_id:" + i + ",a:"
                    + ( startValue + i ) + ",b:" + ( startId + i ) + "}" );
            insertDatas.add( data );
            expDatas.add( data );
        }
        Collections.shuffle( insertDatas );
        cl.insert( insertDatas );
        return expDatas;
    }

    public class OrderBy implements Comparator< BSONObject > {

        @Override
        public int compare( BSONObject obj1, BSONObject obj2 ) {
            int flag = 0;
            int a1 = ( int ) obj1.get( "a" );
            int b1 = ( int ) obj1.get( "b" );
            int a2 = ( int ) obj2.get( "a" );
            int b2 = ( int ) obj2.get( "b" );
            if ( a1 > a2 ) {
                flag = 1;
            } else if ( a1 < a2 ) {
                flag = -1;
            } else {
                if ( b1 > b2 ) {
                    flag = 1;
                } else if ( b1 < b2 ) {
                    flag = -1;
                }
            }

            return flag;
        }

    }
}
