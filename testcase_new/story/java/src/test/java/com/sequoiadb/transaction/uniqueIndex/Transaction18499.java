package com.sequoiadb.transaction.uniqueIndex;

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
 * @Description seqDB-18499:主子表事务中唯一索引重复，事务回滚
 * @author yinzhen
 * @date 2019-6-11
 *
 */
public class Transaction18499 extends SdbTestBase {
    private Sequoiadb sdb;
    private Sequoiadb db;
    private String clName = "cl18499";
    private String idxName = "textIndex18499";
    private List< BSONObject > expList;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "STANDALONE MODE" );
        }
        if ( CommLib.OneGroupMode( sdb ) ) {
            throw new SkipException( "ONE GROUP MODE" );
        }
        DBCollection cl = sdb.getCollectionSpace( csName )
                .createCollection( clName, ( BSONObject ) JSON.parse(
                        "{ShardingKey:{b:1}, ShardingType:'range', IsMainCL:true}" ) );
        sdb.getCollectionSpace( csName ).createCollection( "sub118499" );
        sdb.getCollectionSpace( csName ).createCollection( "sub218499",
                ( BSONObject ) JSON.parse(
                        "{ShardingKey:{b:1}, ShardingType:'hash', AutoSplit:true}" ) );
        sdb.getCollectionSpace( csName ).createCollection( "sub318499",
                ( BSONObject ) JSON.parse(
                        "{ShardingKey:{b:1}, ShardingType:'hash', AutoSplit:true}" ) );
        cl.attachCollection( csName + ".sub118499", ( BSONObject ) JSON
                .parse( "{LowBound:{b:{'$minKey':1}}, UpBound:{b:100}}" ) );
        cl.attachCollection( csName + ".sub218499", ( BSONObject ) JSON
                .parse( "{LowBound:{b:100}, UpBound:{b:200}}" ) );
        cl.attachCollection( csName + ".sub318499", ( BSONObject ) JSON
                .parse( "{LowBound:{b:200}, UpBound:{b:{'$maxKey':1}}}" ) );
        cl.createIndex( idxName, "{a:1, b:1}", true, false );

        // 插入多条记录R1s
        expList = TransUtils.insertRandomDatas( cl, 0, 300 );
    }

    @AfterClass
    public void tearDown() {
        if ( db != null ) {
            db.commit();
            db.close();
        }
        if ( sdb != null ) {
            CollectionSpace cs = sdb.getCollectionSpace( csName );
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
            sdb.close();
        }
    }

    @Test
    public void test() throws InterruptedException {
        db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        DBCollection cl1 = db.getCollectionSpace( csName )
                .getCollection( clName );

        // 开启事务，插入多条记录R2s
        TransUtils.beginTransaction( db );
        try {
            insertData( cl1 );
            Assert.fail();
        } catch ( BaseException e ) {
            if ( -38 != e.getErrorCode() ) {
                throw e;
            }
        }

        // 事务回滚，非事务查询，查询到记录R1s
        // 索引扫描
        DBCollection cl2 = db.getCollectionSpace( csName )
                .getCollection( clName );
        DBCursor cursor = cl2.query( "", "", "{a:1, b:1}",
                "{'':'" + idxName + "'}" );
        List< BSONObject > actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );

        // 表扫描
        cursor = cl2.query( "", "", "{a:1, b:1}", "{'':null}" );
        actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );

        // 事务查询表扫描
        TransUtils.beginTransaction( db );
        cursor = cl2.query( "", "", "{a:1, b:1}", "{'':null}" );
        actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );

        // 事务查询索引扫描
        cursor = cl2.query( "", "", "{a:1, b:1}", "{'':'" + idxName + "'}" );
        actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );

        db.commit();
    }

    private void insertData( DBCollection cl ) {
        List< BSONObject > records = new ArrayList< BSONObject >();
        for ( int i = 0; i < 299; i++ ) {
            BSONObject obj = ( BSONObject ) JSON.parse( "{_id:" + ( i + 50 )
                    + ", a:" + ( i + 50 ) + ", b:" + i + "}" );
            records.add( obj );
        }
        BSONObject obj = ( BSONObject ) JSON.parse( "{a:1000, b:1000}" );
        records.add( obj );
        Collections.shuffle( records );
        cl.insert( records );
    }
}
