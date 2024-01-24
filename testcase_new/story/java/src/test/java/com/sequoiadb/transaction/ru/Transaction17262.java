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
 * @Description Transaction17262.java query.update操作时，事务提交，与读并发
 * @author luweikang
 * @date 2019年1月15日
 */
@Test(groups = "ru")
public class Transaction17262 extends SdbTestBase {

    private String clName = "transCL_17262";
    private Sequoiadb sdb = null;
    private Sequoiadb sdb2 = null;
    private DBCollection cl = null;
    private DBCollection cl2 = null;
    private BSONObject data = null;
    private BSONObject data2 = null;
    private BSONObject modifier = null;
    private DBCursor recordCur = null;
    private List< BSONObject > expDataList = null;
    private List< BSONObject > actDataList = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        sdb2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        cl.createIndex( "a", "{a:1}", false, false );
        expDataList = new ArrayList< BSONObject >();

        data = new BasicBSONObject();
        data.put( "_id", "insertID17262" );
        data.put( "a", 1 );
        data.put( "b", 1 );
        data.put( "c", 13700000000L );
        data.put( "d", "customer transaction type data application." );
        cl.insert( data );

        modifier = new BasicBSONObject();
        data2 = new BasicBSONObject();
        data2.put( "_id", "updateID17262" );
        data2.put( "a", 2 );
        data2.put( "b", "update2" );
        data2.put( "c", 13700000000L );
        data2.put( "d", "customer transaction type data application." );
        modifier.put( "$set", data2 );

    }

    @Test
    public void test() {
        cl2 = sdb2.getCollectionSpace( csName ).getCollection( clName );

        TransUtils.beginTransaction( sdb );
        TransUtils.beginTransaction( sdb2 );

        // 2 query.update
        DBCursor tbCur = cl.queryAndUpdate( new BasicBSONObject( "a", 1 ), null,
                null, null, modifier, 0, -1, 0, true );
        BSONObject actData = tbCur.getNext();
        try {
            Assert.assertEquals( actData, data2 );
        } finally {
            if ( tbCur != null ) {
                tbCur.close();
            }
        }

        // trans1 query
        expDataList.add( data2 );
        recordCur = cl.query( null, null, "{a:1}", "{'': null}" );
        actDataList = TransUtils.getReadActList( recordCur );
        Assert.assertEquals( actDataList, expDataList );
        actDataList.clear();

        recordCur = cl.query( null, null, "{a:1}", "{'': 'a'}" );
        actDataList = TransUtils.getReadActList( recordCur );
        Assert.assertEquals( actDataList, expDataList );
        actDataList.clear();

        // 3 trans2 query
        expDataList.clear();
        expDataList.add( data2 );
        recordCur = cl2.query( null, null, "{a:1}", "{'': null}" );
        actDataList = TransUtils.getReadActList( recordCur );
        Assert.assertEquals( actDataList, expDataList );
        actDataList.clear();

        recordCur = cl2.query( null, null, "{a:1}", "{'': 'a'}" );
        actDataList = TransUtils.getReadActList( recordCur );
        Assert.assertEquals( actDataList, expDataList );
        actDataList.clear();

        // 4 commit trans1 and query
        sdb.commit();
        recordCur = cl.query( null, null, "{a:1}", "{'': null}" );
        actDataList = TransUtils.getReadActList( recordCur );
        Assert.assertEquals( actDataList, expDataList );
        actDataList.clear();

        recordCur = cl.query( null, null, "{a:1}", "{'': 'a'}" );
        actDataList = TransUtils.getReadActList( recordCur );
        Assert.assertEquals( actDataList, expDataList );
        actDataList.clear();

        // 5 trans2 query
        recordCur = cl2.query( null, null, "{a:1}", "{'': null}" );
        actDataList = TransUtils.getReadActList( recordCur );
        Assert.assertEquals( actDataList, expDataList );
        actDataList.clear();

        recordCur = cl2.query( null, null, "{a:1}", "{'': 'a'}" );
        actDataList = TransUtils.getReadActList( recordCur );
        Assert.assertEquals( actDataList, expDataList );
        actDataList.clear();

        // 6 commit trans2 and query
        sdb2.commit();
        recordCur = cl.query( null, null, "{a:1}", "{'': null}" );
        actDataList = TransUtils.getReadActList( recordCur );
        Assert.assertEquals( actDataList, expDataList );
        actDataList.clear();

        recordCur = cl.query( null, null, "{a:1}", "{'': 'a'}" );
        actDataList = TransUtils.getReadActList( recordCur );
        Assert.assertEquals( actDataList, expDataList );
        actDataList.clear();
    }

    @AfterClass
    public void tearDown() {
        sdb.commit();
        sdb2.commit();

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
    }

}
