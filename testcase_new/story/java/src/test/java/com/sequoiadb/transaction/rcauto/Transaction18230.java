package com.sequoiadb.transaction.rcauto;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @testcase seqDB-18230:query and update操作失败自动回滚
 * @date 2019-4-16
 * @author yinzhen
 *
 */
@Test(groups = { "rcauto", "rrauto" })
public class Transaction18230 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl18230";
    private DBCollection cl = null;
    private List< BSONObject > expList = new ArrayList<>();

    @BeforeClass
    public void setUp() {
        sdb = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "STANDALONE MODE" );
        }
        if ( CommLib.OneGroupMode( sdb ) ) {
            throw new SkipException( "less than two groups" );
        }

        cl = sdb.getCollectionSpace( csName ).createCollection( clName,
                ( BSONObject ) JSON.parse(
                        "{ShardingKey:{b:1}, ShardingType:'hash', AutoSplit: true}" ) );
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

    @Test
    public void test() throws InterruptedException {
        // 在集合中创建正序的唯一索引，比如：a为唯一索引，并插入多条包含索引字段的记录R1s
        cl.createIndex( "idx18230", "{a:1, b:1}", true, false );
        insertData();

        // 再插入记录R2，索引字段值R2大于R1s
        BSONObject record = ( BSONObject ) JSON
                .parse( "{_id:250, a:250, b:250}" );
        cl.insert( record );
        expList.add( record );

        DBCursor cursor = cl.query( "", "", "{a:1, b:1}", "" );
        List< BSONObject > actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );

        // 使用query and update批量更新记录R1s为R2s，过程中某条记录由于唯一索引键与R2冲突导致更新失败，更新操作走索引
        // ERROR ：分区表不使用 sort 参数的话会导致回滚失败，普通表则不会
        try {
            cursor = cl.queryAndUpdate(
                    ( BSONObject ) JSON
                            .parse( "{$and:[{a:{$gte:0}},{a:{$lt:200}}]}" ),
                    null, null, ( BSONObject ) JSON.parse( "{'':'idx18230'}" ),
                    ( BSONObject ) JSON.parse( "{$inc:{a:100}}" ), 0, -1, 0,
                    true );
            actList = TransUtils.getReadActList( cursor );
            Assert.fail( "Auto Rollback Error" );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -38 );
        }

        cursor = cl.query( "", "", "{a:1, b:1}", "" );
        actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );
    }

    private void insertData() throws InterruptedException {
        List< BSONObject > records = new ArrayList<>();
        for ( int i = 0; i < 99; i++ ) {
            BSONObject record = ( BSONObject ) JSON
                    .parse( "{_id:" + i + ", a:" + i + ", b:" + i + "}" );
            records.add( record );
        }

        records.add( ( BSONObject ) JSON.parse( "{_id:200, a:150, b:250}" ) );
        expList.addAll( records );
        Collections.shuffle( records );
        cl.insert( records );
        Thread.sleep( 100 );
    }
}
