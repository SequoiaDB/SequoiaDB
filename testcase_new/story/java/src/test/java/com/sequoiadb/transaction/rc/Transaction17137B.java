package com.sequoiadb.transaction.rc;

import java.util.ArrayList;
import java.util.Collections;
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
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @Description Transaction17137.java 创建/删除索引与事务操作并发
 * @author luweikang
 * @date 2019年1月15日
 */
@Test(groups = "rc")
public class Transaction17137B extends SdbTestBase {

    private String clName = "transCL_17137B";
    private Sequoiadb sdb = null;
    private Sequoiadb sdb1 = null;
    private Sequoiadb sdb2 = null;
    private DBCollection cl = null;
    private int recordNum = 200;
    private DBCursor recordCur = null;
    private List< BSONObject > rs1 = null;
    private List< BSONObject > rs2 = null;
    private List< BSONObject > expDataList = null;

    @BeforeClass
    public void setUp() {
        sdb = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        sdb1 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        sdb2 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        expDataList = prepareData( recordNum );
        rs1 = expDataList.subList( 0, recordNum / 2 );
        rs2 = expDataList.subList( recordNum / 2, recordNum );
        cl.insert( rs1 );

    }

    @Test
    public void test() {

        TransUtils.beginTransaction( sdb1 );
        TransUtils.beginTransaction( sdb2 );

        CRUDThread crudThread = new CRUDThread();
        crudThread.start();

        IndexThread indexThread = new IndexThread();
        indexThread.start();

        QueryThread queryThread = new QueryThread();
        queryThread.start();

        Assert.assertTrue( crudThread.isSuccess(), crudThread.getErrorMsg() );
        Assert.assertTrue( indexThread.isSuccess(), indexThread.getErrorMsg() );
        Assert.assertTrue( queryThread.isSuccess(), queryThread.getErrorMsg() );

        sdb1.commit();
        Assert.assertTrue( cl.isIndexExist( "a" ) );

        expDataList.clear();
        expDataList = expData();

        TransUtils.queryAndCheck( cl, null, "{'_id': {'$include': 0}}", null,
                "{'': null}", expDataList );
        TransUtils.queryAndCheck( cl, null, "{'_id': {'$include': 0}}", null,
                "{'': 'a'}", expDataList );

        sdb2.commit();

        TransUtils.queryAndCheck( cl, null, "{'_id': {'$include': 0}}", null,
                "{'': null}", expDataList );
        TransUtils.queryAndCheck( cl, null, "{'_id': {'$include': 0}}", null,
                "{'': 'a'}", expDataList );
    }

    @AfterClass
    public void tearDown() {
        sdb1.commit();
        sdb2.commit();

        if ( sdb1 != null ) {
            sdb1.close();
        }
        if ( sdb2 != null ) {
            sdb2.close();
        }
        if ( recordCur != null ) {
            recordCur.close();
        }
        sdb.getCollectionSpace( csName ).dropCollection( clName );
        if ( sdb != null ) {
            sdb.close();
        }
    }

    private List< BSONObject > prepareData( int recordNum ) {
        List< BSONObject > dataList = new ArrayList< BSONObject >();
        for ( int i = 0; i < recordNum; i++ ) {
            BSONObject data = new BasicBSONObject();
            data.put( "a", i );
            data.put( "b", "testTrans_17137_" + i );
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

    private class CRUDThread extends SdbThreadBase {

        @Override
        public void exec() {
            DBCollection cl1 = sdb1.getCollectionSpace( csName )
                    .getCollection( clName );
            cl1.insert( rs2 );

            String modifier = "{'$set':{ 'a': 1024, 'b': 'test_update_1024'}}";
            cl1.update( "{'a':{'$gte':0, '$lt': " + recordNum / 2 + "}}",
                    modifier, "{'': null}" );

            cl1.delete( "{'a':{'$gte':" + recordNum / 2 + ", '$lt': "
                    + recordNum + "}}", "{'': null}" );
        }
    }

    private class IndexThread extends SdbThreadBase {

        @Override
        public void exec() {
            Sequoiadb db = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
            try {
                TransUtils.beginTransaction( db );
                DBCollection dbcl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                for ( int i = 0; i < 100; i++ ) {
                    dbcl.createIndex( "a", "{a:1, b:-1}", false, false );
                    dbcl.dropIndex( "a" );
                }
                dbcl.createIndex( "a", "{a:1, b:-1}", false, false );
            } finally {
                db.commit();
                if ( db != null ) {
                    db.close();
                }
            }
        }
    }

    private class QueryThread extends SdbThreadBase {

        @Override
        public void exec() {
            DBCursor cur = null;
            try {
                List< BSONObject > positiveRsList = new ArrayList< BSONObject >();
                positiveRsList.addAll( rs1 );
                List< BSONObject > reversedRsList = new ArrayList< BSONObject >();
                Collections.reverse( rs1 );
                reversedRsList.addAll( rs1 );
                Collections.reverse( rs1 );
                for ( int i = 0; i < 5; i++ ) {
                    DBCollection dbcl = sdb2.getCollectionSpace( csName )
                            .getCollection( clName );

                    // 正序查询
                    TransUtils.queryAndCheck( dbcl, "{a :1}", "{'': null}",
                            positiveRsList );

                    // 逆序查询
                    TransUtils.queryAndCheck( dbcl, "{a : -1}", "{'': null}",
                            reversedRsList );

                    try {
                        // 正序查询
                        TransUtils.checkQueryResultOnly( dbcl, "", "{a :1}",
                                "{'': 'a'}", positiveRsList );

                        // 逆序查询
                        TransUtils.checkQueryResultOnly( dbcl, "", "{a : -1}",
                                "{'': 'a'}", reversedRsList );
                    } catch ( BaseException e ) {
                        int actErrCode = e.getErrorCode();
                        if ( actErrCode != -48 && actErrCode != -52
                                && actErrCode != -10 && actErrCode != -199
                                && actErrCode != -47 ) {
                            e.printStackTrace();
                            throw e;
                        }
                    }
                }
            } finally {
                if ( cur != null ) {
                    cur.close();
                }
            }
        }
    }

}