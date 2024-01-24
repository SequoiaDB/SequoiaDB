package com.sequoiadb.transaction.rc;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

import org.bson.BSONObject;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @Description seqDB-18636:批量插入/更新/删除记录与带查询条件执行查询并发，事务提交
 * @date 2019-7-9
 * @author yinzhen
 *
 */
@Test(groups = { "rc" })
public class Transaction18636 extends SdbTestBase {
    private Sequoiadb sdb;
    private Sequoiadb db1;
    private Sequoiadb db2;
    private String hashCLName = "cl18636_hash";
    private List< BSONObject > expList;
    private String hintTbScan = "{'':null}";
    private String hintIxScan = "{'':'idx18636'}";
    private DBCollection cl;

    @BeforeClass
    public void setUp() {
        sdb = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        db1 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        db2 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "STANDALONE MODE" );
        }
        if ( CommLib.OneGroupMode( sdb ) ) {
            throw new SkipException( "ONE GROUP MODE" );
        }

        // 创建分区表并插入记录R1s
        TransUtils.createHashCL( sdb, csName, hashCLName );
        cl = sdb.getCollectionSpace( csName ).getCollection( hashCLName );
        cl.createIndex( "idx18636", "{a:1}", false, false );
        expList = TransUtils.insertRandomDatas( cl, 0, 100 );
    }

    @AfterClass
    public void tearDown() {
        if ( db1 != null ) {
            db1.commit();
            db1.close();
        }
        if ( db2 != null ) {
            db2.commit();
            db2.close();
        }
        if ( sdb != null ) {
            CollectionSpace cs = sdb.getCollectionSpace( csName );
            cs.dropCollection( hashCLName );
            sdb.close();
        }
    }

    @Test
    public void test() {
        // 开启两个并发事务
        TransUtils.beginTransaction( db1 );
        TransUtils.beginTransaction( db2 );
        DBCollection cl1 = db1.getCollectionSpace( csName )
                .getCollection( hashCLName );
        DBCollection cl2 = db2.getCollectionSpace( csName )
                .getCollection( hashCLName );

        // 事务1批量插入记录后为R2s
        List< BSONObject > addList = TransUtils.insertRandomDatas( cl1, 100,
                120 );

        // 事务2表扫描/索引扫描记录
        String range = Arrays.toString( getInRange( 0, 120 ) );
        TransUtils.queryAndCheck( cl2, "{a:{$in:" + range + "}}", null, "{a:1}",
                hintTbScan, expList );
        TransUtils.queryAndCheck( cl2, "{a:{$in:" + range + "}}", null, "{a:1}",
                hintIxScan, expList );

        // 非事务表扫描/索引扫描记录
        List< BSONObject > expRecords = new ArrayList<>( expList );
        expRecords.addAll( addList );
        TransUtils.queryAndCheck( cl, "{a:{$in:" + range + "}}", null, "{a:1}",
                hintTbScan, expRecords );
        TransUtils.queryAndCheck( cl, "{a:{$in:" + range + "}}", null, "{a:1}",
                hintIxScan, expRecords );

        // 事务1批量更新记录为R3s
        cl1.update( "{a:{$in:" + range + "}}", "{$inc:{a:10}}", hintIxScan );

        // 事务2表扫描/索引扫描记录
        TransUtils.queryAndCheck( cl2, "{a:{$in:" + range + "}}", null, "{a:1}",
                hintTbScan, expList );
        TransUtils.queryAndCheck( cl2, "{a:{$in:" + range + "}}", null, "{a:1}",
                hintIxScan, expList );

        // 非事务表扫描/索引扫描记录
        expRecords = TransUtils.getIncDatas( 0, 110, 10 );
        TransUtils.queryAndCheck( cl, "{a:{$in:" + range + "}}", null, "{a:1}",
                hintTbScan, expRecords );
        TransUtils.queryAndCheck( cl, "{a:{$in:" + range + "}}", null, "{a:1}",
                hintIxScan, expRecords );

        // 事务1批量删除记录为R4s
        cl1.delete( "{a:{$in:" + range + "}}", hintIxScan );

        // 事务2表扫描/索引扫描记录
        TransUtils.queryAndCheck( cl2, "{a:{$in:" + range + "}}", null, "{a:1}",
                hintTbScan, expList );
        TransUtils.queryAndCheck( cl2, "{a:{$in:" + range + "}}", null, "{a:1}",
                hintIxScan, expList );

        // 非事务表扫描/索引扫描记录
        TransUtils.queryAndCheck( cl, "{a:{$in:" + range + "}}", null, "{a:1}",
                hintTbScan, new ArrayList< BSONObject >() );
        TransUtils.queryAndCheck( cl, "{a:{$in:" + range + "}}", null, "{a:1}",
                hintIxScan, new ArrayList< BSONObject >() );

        // 事务1提交
        db1.commit();

        // 事务2表扫描/索引扫描记录
        TransUtils.queryAndCheck( cl2, "{a:{$in:" + range + "}}", null, "{a:1}",
                hintTbScan, new ArrayList< BSONObject >() );
        TransUtils.queryAndCheck( cl2, "{a:{$in:" + range + "}}", null, "{a:1}",
                hintIxScan, new ArrayList< BSONObject >() );

        // 非事务表扫描/索引扫描记录
        TransUtils.queryAndCheck( cl, "{a:{$in:" + range + "}}", null, "{a:1}",
                hintTbScan, new ArrayList< BSONObject >() );
        TransUtils.queryAndCheck( cl, "{a:{$in:" + range + "}}", null, "{a:1}",
                hintIxScan, new ArrayList< BSONObject >() );

        // 事务2提交
        db2.commit();
    }

    private Integer[] getInRange( int start, int end ) {
        List< Integer > rangeList = new ArrayList<>();
        for ( int i = start; i < end; i++ ) {
            rangeList.add( i );
        }
        Collections.shuffle( rangeList );
        Integer[] rangeArray = rangeList
                .toArray( new Integer[ rangeList.size() ] );
        return rangeArray;
    }
}
