package com.sequoiadb.transaction.rc;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.DBQuery;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @FileName:TestTransaction16303 test query() without QUERY_FLG_FOR_UPDATE
 * @author wangkexin
 * @Date 2018-10-30
 * @version 1.00
 */
@Test(groups = "rc")
public class Transaction16303 extends SdbTestBase {
    private Sequoiadb sdb;
    private Sequoiadb sdb2;
    private CollectionSpace cs;
    private DBCollection cl;
    private String clName = "cl16303";
    private String commCSName;
    private ArrayList< BSONObject > insertRecods;

    @BeforeClass
    public void setUp() {
        String coordAddr = SdbTestBase.coordUrl;
        commCSName = SdbTestBase.csName;
        sdb = new Sequoiadb( coordAddr, "", "" );
        sdb2 = new Sequoiadb( coordAddr, "", "" );
        cs = sdb.getCollectionSpace( commCSName );
        cl = cs.createCollection( clName );
        insertData();
    }

    @Test
    private void testTrans16303() {
        String flagCom = "commit";
        String flagRoll = "rollback";

        // test query + update
        testQueryAndUpdate( flagCom );
        // clean up cl and insert data
        cleanAndInsert();

        testQueryAndUpdate( flagRoll );
        cleanAndInsert();

        // test queryOne + update
        testQueryOneAndUpdate( flagCom );
        cleanAndInsert();

        testQueryOneAndUpdate( flagRoll );
    }

    private void testQueryAndUpdate( String flag ) {
        try {
            TransUtils.beginTransaction( sdb );
            TransUtils.beginTransaction( sdb2 );
            DBCollection cl1 = sdb.getCollectionSpace( commCSName )
                    .getCollection( clName );
            DBCollection cl2 = sdb2.getCollectionSpace( commCSName )
                    .getCollection( clName );
            DBCursor cursor = cl1.query();
            DBQuery query = new DBQuery();
            query.setModifier( ( BSONObject ) JSON.parse( "{$set:{age:22}}" ) );
            checkQueryResult( cursor );
            cl2.update( query );
            sdb.commit();
            switch ( flag ) {
            case "commit":
                sdb2.commit();
                DBCursor cursor2 = cl2.query();
                checkResultAfterUpdate( cursor2 );
                break;
            case "rollback":
                sdb2.rollback();
                DBCursor cursor3 = cl2.query();
                checkQueryResult( cursor3 );
                break;
            default:
                Assert.fail(
                        "The parameter is not commit or rollback,please check it again!" );
                break;
            }
        } catch ( BaseException e ) {
            Assert.fail(
                    "Sequoiadb driver TestTransaction16303 testQuery error, error description:"
                            + e.getMessage() );
        } finally {
            sdb.commit();
            sdb2.rollback();
        }
    }

    private void testQueryOneAndUpdate( String flag ) {
        try {
            TransUtils.beginTransaction( sdb );
            TransUtils.beginTransaction( sdb2 );
            DBCollection cl1 = sdb.getCollectionSpace( commCSName )
                    .getCollection( clName );
            DBCollection cl2 = sdb2.getCollectionSpace( commCSName )
                    .getCollection( clName );
            BSONObject obj = cl1.queryOne();
            DBQuery query = new DBQuery();
            query.setModifier( ( BSONObject ) JSON.parse( "{$set:{age:22}}" ) );
            cl2.update( query );
            sdb.commit();
            // check result
            BSONObject expobj = new BasicBSONObject();
            expobj.put( "_id", 0 );
            expobj.put( "age", 0 );
            Assert.assertEquals( obj, expobj );

            switch ( flag ) {
            case "commit":
                sdb2.commit();
                DBCursor cursor2 = cl2.query();
                checkResultAfterUpdate( cursor2 );
                break;
            case "rollback":
                sdb2.rollback();
                DBCursor cursor3 = cl2.query();
                checkQueryResult( cursor3 );
                break;

            default:
                Assert.fail(
                        "The parameter is not commit or rollback,please check it again!" );
                break;
            }
        } catch ( BaseException e ) {
            Assert.fail(
                    "Sequoiadb driver TestTransaction16303 testQueryOne error, error description:"
                            + e.getMessage() );
        } finally {
            sdb.commit();
            sdb2.rollback();
        }
    }

    private void checkQueryResult( DBCursor cursor ) {
        List< BSONObject > actualList = new ArrayList< BSONObject >();
        while ( cursor.hasNext() ) {
            actualList.add( cursor.getNext() );
        }
        cursor.close();
        Assert.assertEquals( actualList, insertRecods );
    }

    private void checkResultAfterUpdate( DBCursor cursor ) {
        List< BSONObject > actualList = new ArrayList< BSONObject >();
        while ( cursor.hasNext() ) {
            actualList.add( cursor.getNext() );
        }
        cursor.close();

        List< BSONObject > expectedList = new ArrayList< BSONObject >();
        for ( int i = 0; i < insertRecods.size(); i++ ) {
            BSONObject obj = new BasicBSONObject();
            obj = insertRecods.get( i );
            obj.put( "age", 22 );
            expectedList.add( obj );
        }
        Assert.assertEquals( actualList, expectedList );
    }

    private void cleanAndInsert() {
        cl.truncate();
        insertData();
    }

    private void insertData() {
        try {
            BSONObject bson;
            insertRecods = new ArrayList< BSONObject >();
            for ( int i = 0; i < 100; i++ ) {
                bson = new BasicBSONObject();
                bson.put( "_id", i );
                bson.put( "age", i );
                insertRecods.add( bson );
            }
            cl.insert( insertRecods, 0 );
        } catch ( BaseException e ) {
            Assert.fail(
                    "Sequoiadb driver TestTransaction16303 insertData error, error description:"
                            + e.getMessage() );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            CollectionSpace cs = sdb.getCollectionSpace( csName );
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            sdb.close();
            sdb2.close();
        }
    }
}
