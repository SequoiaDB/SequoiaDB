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
 * @testcase seqDB-18227: 批量插入操作失败自动回滚
 * @date 2019-4-17
 * @author yinzhen
 *
 */
@Test(groups = { "rcauto", "rrauto" })
public class Transaction18227 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl18227";
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
        // 在集合中创建正序的唯一索引，比如：a为唯一索引，插入一条包含索引字段的记录R1
        cl.createIndex( "idx18227", "{a:1, b:1}", true, false );
        BSONObject record = ( BSONObject ) JSON.parse( "{_id:1, a:1, b:1}" );
        cl.insert( record );
        expList.add( record );

        // 事务中查询记录
        DBCursor cursor = cl.query( "", "", "{a:1, b:1}", "" );
        List< BSONObject > actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );

        // 批量插入记录，过程中某条记录由于唯一索引键与R1冲突导致插入失败
        try {
            insertData();
            Assert.fail( "Auto Rollback Error" );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -38 );
        }

        // 事务中查询记录
        cursor = cl.query( "", "", "{a:1, b:1}", "" );
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
        Collections.shuffle( records );
        cl.insert( records );
        Thread.sleep( 100 );
    }
}
