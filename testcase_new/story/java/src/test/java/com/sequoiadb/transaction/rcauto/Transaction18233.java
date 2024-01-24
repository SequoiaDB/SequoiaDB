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
 * @testcase seqDB-18233:主子表中批量插入记录失败，整组执行事务回滚
 * @date 2019-4-16
 * @author yinzhen
 *
 */
@Test(groups = { "rcauto", "rrauto" })
public class Transaction18233 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl18233";
    private DBCollection cl = null;
    private List< String > groupNames;
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

        groupNames = CommLib.getDataGroupNames( sdb );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName,
                ( BSONObject ) JSON.parse(
                        "{ShardingType:'range', ShardingKey:{a:1}, IsMainCL:true}" ) );
        sdb.getCollectionSpace( csName ).createCollection( "subCL1",
                ( BSONObject ) JSON
                        .parse( "{Group:'" + groupNames.get( 0 ) + "'}" ) );
        sdb.getCollectionSpace( csName ).createCollection( "subCL2",
                ( BSONObject ) JSON.parse(
                        "{ShardingKey:{b:1}, ShardingType:'hash', AutoSplit: true}" ) );
        cl.attachCollection( csName + ".subCL1", ( BSONObject ) JSON
                .parse( "{LowBound:{a:0}, UpBound:{a:1000}}" ) );
        cl.attachCollection( csName + ".subCL2", ( BSONObject ) JSON
                .parse( "{LowBound:{a:1000}, UpBound:{a:2000}}" ) );
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
        BSONObject record = ( BSONObject ) JSON.parse( "{_id:0, a:0, b:0}" );
        cl.insert( record );
        expList.add( record );

        // 通过主表批量插入记录，在多个组上均已插入记录的情况下，其中某条记录的_id字段与已存在记录R1冲突
        DBCursor cursor = cl.query();
        List< BSONObject > actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );

        try {
            insertData();
            Assert.fail( "Auto Rollback Error" );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -38 );
        }

        cursor = cl.query();
        actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );
    }

    private void insertData() throws InterruptedException {
        List< BSONObject > records = new ArrayList<>();
        for ( int i = 0; i < 2000; i++ ) {
            BSONObject record = ( BSONObject ) JSON
                    .parse( "{_id:" + i + ", a:" + i + ", b:" + i + "}" );
            records.add( record );
        }
        Collections.shuffle( records );
        cl.insert( records );
        Thread.sleep( 100 );
    }
}
