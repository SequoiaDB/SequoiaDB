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
 * @testcase seqDB-18231:query and remove操作失败自动回滚
 * @date 2019-4-16
 * @author yinzhen
 *
 */
@Test(groups = { "rcauto", "rrauto" })
public class Transaction18231 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private Sequoiadb db1;
    private String clName = "cl18231";
    private DBCollection cl = null;
    private List< BSONObject > expList = new ArrayList<>();

    @BeforeClass
    public void setUp() {
        sdb = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        db1 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        sdb.setSessionAttr( ( BSONObject ) JSON.parse( "{TransTimeout:5}" ) );
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
        db1.commit();
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
        DBCollection cl1 = db1.getCollectionSpace( csName )
                .getCollection( clName );

        // 在集合中创建正序的唯一索引，比如：a为唯一索引，并插入多条包含索引字段的记录R1s
        cl.createIndex( "idx18231", "{a:-1, b:-1}", true, false );
        insertData();

        // 开启事务1，插入记录R2，索引字段值R1s大于R2
        TransUtils.beginTransaction( db1 );
        BSONObject record = ( BSONObject ) JSON
                .parse( "{_id:-10, a:-10, b:-10}" );
        expList.add( 0, record );
        cl1.insert( record );

        DBCursor cursor = cl1.query( "", "", "{a:1, b:1}", "" );
        List< BSONObject > actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );

        // 使用query and remove批量删除记录R1s及R2，删除操作走索引，等待死锁超时后，提交事务1
        // ERROR：普通表阻塞后回滚，分区表不会回滚
        try {
            cursor = cl.queryAndRemove(
                    ( BSONObject ) JSON
                            .parse( "{$and:[{a:{$gte:-10}},{a:{$lt:200}}]}" ),
                    null, null, ( BSONObject ) JSON.parse( "{'':'idx18231'}" ),
                    0, -1, 0 );
            actList = TransUtils.getReadActList( cursor );
            Assert.fail( "Auto Rollback Error" );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -13 );
        } finally {
            TransUtils.commitTransaction( db1 );
        }

        cursor = cl.query( "", "", "{a:1, b:1}", "" );
        actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );
    }

    private void insertData() throws InterruptedException {
        List< BSONObject > records = new ArrayList<>();
        for ( int i = 0; i < 51; i++ ) {
            BSONObject record = ( BSONObject ) JSON
                    .parse( "{_id:" + i + ", a:" + i + ", b:" + i + "}" );
            records.add( record );
        }
        expList.addAll( records );
        Collections.shuffle( records );
        cl.insert( records );
        Thread.sleep( 100 );
    }
}
