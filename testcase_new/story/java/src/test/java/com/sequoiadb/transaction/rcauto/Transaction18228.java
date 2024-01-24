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
 * @testcase seqDB-18228: 批量更新操作失败自动回滚
 * @date 2019-4-16
 * @author yinzhen
 *
 */
@Test(groups = { "rcauto", "rrauto" })
public class Transaction18228 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl18228";
    private CollectionSpace cs = null;
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

        cs = sdb.getCollectionSpace( csName );
        cl = cs.createCollection( clName, ( BSONObject ) JSON.parse(
                "{ShardingKey:{b:1}, ShardingType:'hash', AutoSplit: true}" ) );
    }

    @AfterClass
    public void tearDown() {
        cs.dropCollection( clName );
        if ( !sdb.isClosed() ) {
            sdb.close();
        }
    }

    @Test
    public void test() throws InterruptedException {
        // 在集合中创建正序的唯一索引，比如：a为唯一索引，并插入多条包含索引字段的记录R1s
        cl.createIndex( "idx18228", "{a:1, b:1}", true, false );
        insertData();

        // 再插入记录R2，索引字段值R2大于R1s
        BSONObject record = ( BSONObject ) JSON
                .parse( "{_id:250, a:250, b:250}" );
        cl.insert( record );
        expList.add( record );

        DBCursor cursor = cl.query( "", "", "{a:1, b:1}", "" );
        List< BSONObject > actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );

        // 使用update批量更新记录R1s为R2s，过程中某条记录由于唯一索引键与R2冲突导致更新失败，更新操作走索引
        try {
            cl.update( "{$and:[{a:{$gte:0}},{a:{$lt:200}}]}", "{$inc:{a:100}}",
                    "{'':'idx18228'}" );
            Assert.fail( "Auto Rollback Error" );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -38 );
        }

        cursor = cl.query( "", "", "{a:1,b:1}", "" );
        actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );

    }

    private void insertData() throws InterruptedException {
        List< BSONObject > records = new ArrayList<>();
        for ( int i = 0; i < 200; i++ ) {
            BSONObject record = ( BSONObject ) JSON
                    .parse( "{_id:" + i + ", a:" + i + ", b:" + i + "}" );
            records.add( record );
        }

        records.add( 151,
                ( BSONObject ) JSON.parse( "{_id:200, a:150, b:250}" ) );
        expList.addAll( records );
        Collections.shuffle( records );
        cl.insert( records );
        Thread.sleep( 100 );
    }
}
