package com.sequoiadb.transaction.ru;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @Description Transaction17259.java 创建/删除索引与事务操作并发
 * @author luweikang
 * @date 2019年1月15日
 */
@Test(groups = "ru")
public class Transaction17259 extends SdbTestBase {

    private String clName = "transCL_17259";
    private Sequoiadb sdb = null;
    private Sequoiadb sdb1 = null;
    private Sequoiadb sdb2 = null;
    private Sequoiadb sdb3 = null;
    private DBCollection cl = null;
    private DBCollection cl1 = null;
    private DBCollection cl2 = null;
    private DBCollection cl3 = null;
    private int recordNum = 10000;
    private DBCursor recordCur = null;
    private List< BSONObject > expDataList = null;
    private List< BSONObject > actDataList = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        sdb1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        sdb2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        sdb3 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        expDataList = prepareData( recordNum );

    }

    @Test
    public void test() {
        cl1 = sdb1.getCollectionSpace( csName ).getCollection( clName );
        cl2 = sdb2.getCollectionSpace( csName ).getCollection( clName );
        cl3 = sdb3.getCollectionSpace( csName ).getCollection( clName );

        TransUtils.beginTransaction( sdb );
        TransUtils.beginTransaction( sdb3 );

        // 1 trans1 insert/update/delete record
        cl1.insert( expDataList );
        cl1.update( "{'a':{'$gte':0, '$lt': " + recordNum / 2 + "}}",
                "{'$set':{ 'a': 1024, 'b': 'test_update_1024'}}", null );
        cl1.delete( "{'a':{'$gte':" + recordNum / 2 + ", '$lt': " + recordNum
                + "}}" );

        // 2 sdb2 create/drop index
        for ( int i = 0; i < 10; i++ ) {
            cl2.createIndex( "a", "{a:1, b:-1}", false, false );
            cl2.dropIndex( "a" );
        }
        cl2.createIndex( "a", "{a:1, b:-1}", false, false );
        Assert.assertTrue( cl.isIndexExist( "a" ) );

        // 3 trans2 selete record
        expDataList = expData();
        recordCur = cl3.query( null, "{'_id': {'$include': 0}}", null,
                "{'': null}" );
        actDataList = TransUtils.getReadActList( recordCur );
        Assert.assertEquals( actDataList, expDataList );
        actDataList.clear();

        recordCur = cl3.query( null, "{'_id': {'$include': 0}}", null,
                "{'': 'a'}" );
        actDataList = TransUtils.getReadActList( recordCur );
        Assert.assertEquals( actDataList, expDataList );
        actDataList.clear();

        // 4 commit trans1
        sdb.commit();

        // 5 trans2 selete record
        expDataList.clear();
        expDataList = expData();
        recordCur = cl3.query( null, "{'_id': {'$include': 0}}", null,
                "{'': null}" );
        actDataList = TransUtils.getReadActList( recordCur );
        Assert.assertEquals( actDataList, expDataList );
        actDataList.clear();

        recordCur = cl3.query( null, "{'_id': {'$include': 0}}", null,
                "{'': 'a'}" );
        actDataList = TransUtils.getReadActList( recordCur );
        Assert.assertEquals( actDataList, expDataList );
        actDataList.clear();

        // 6 commit all trans
        sdb3.commit();

        cl.delete( "{a:{'$isnull': 0}}" );
        Assert.assertEquals( cl.getCount(), 0 );
    }

    @AfterClass
    public void tearDown() {
        sdb1.commit();
        sdb3.commit();

        sdb.getCollectionSpace( csName ).dropCollection( clName );
        if ( recordCur != null ) {
            recordCur.close();
        }
        if ( sdb != null ) {
            sdb.close();
        }
        if ( sdb2 != null ) {
            sdb2.close();
        }
        if ( sdb3 != null ) {
            sdb3.close();
        }
    }

    private List< BSONObject > prepareData( int recordNum ) {
        List< BSONObject > dataList = new ArrayList< BSONObject >();
        for ( int i = 0; i < recordNum; i++ ) {
            BSONObject data = new BasicBSONObject();
            data.put( "a", i );
            data.put( "b", "testTrans_17259_" + i );
            data.put( "c", 13700000000L );
            data.put( "d", "customer transaction type data application." );
            dataList.add( data );
        }
        return dataList;
    }

    private List< BSONObject > expData() {
        List< BSONObject > dataList = new ArrayList< BSONObject >();
        BSONObject data = null;
        for ( int i = 0; i < recordNum / 2; i++ ) {
            data = new BasicBSONObject();
            data.put( "a", 1024 );
            data.put( "b", "test_update_1024" );
            data.put( "c", 13700000000L );
            data.put( "d", "customer transaction type data application." );
            dataList.add( data );
        }
        return dataList;
    }

}
