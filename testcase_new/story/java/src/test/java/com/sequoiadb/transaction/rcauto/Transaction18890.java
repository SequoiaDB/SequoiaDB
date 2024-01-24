package com.sequoiadb.transaction.rcauto;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @testcase seqDB-18890:分区表执行多次游标查询，不取完游标
 * @date 2019-7-19
 * @author zhaoyu
 *
 */
@Test(groups = { "rcauto", "rrauto" })
public class Transaction18890 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl18890";
    private DBCollection cl = null;

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
                        "{ShardingKey:{a:1}, ShardingType:'hash', AutoSplit: true}" ) );
    }

    @AfterClass
    public void tearDown() {
        try {
            CollectionSpace cs = sdb.getCollectionSpace( csName );
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
        } finally {
            if ( !sdb.isClosed() ) {
                sdb.close();
            }
        }

    }

    @Test
    public void test() throws InterruptedException {
        // 插入记录
        insertData();

        // 开启游标查询，游标未走完，关闭游标后，再次执行游标查询
        DBCursor cursor = cl.query();
        cursor.getNext();
        cursor.close();

        DBCursor cursor1 = cl.query();
        cursor1.getNext();
        cursor1.close();
    }

    private void insertData() throws InterruptedException {
        for ( int j = 0; j < 10; j++ ) {
            List< BSONObject > records = new ArrayList<>();
            for ( int i = 0; i < 1000; i++ ) {
                BSONObject record = ( BSONObject ) JSON
                        .parse( "{a:" + i + ", b:" + i + "}" );
                records.add( record );
            }
            Collections.shuffle( records );
            cl.insert( records );
        }
        Thread.sleep( 100 );

    }
}
