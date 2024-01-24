package com.sequoiadb.basicoperation;

import java.util.ArrayList;
import java.util.Date;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.AfterTest;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.BeforeTest;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.DBQuery;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @FileName:TestQueryExplain7091 explain (BSONObject matcher, BSONObject
 *                                selector, BSONObject orderBy, BSONObject hint,
 *                                long skipRows, long returnRows, int flag,
 *                                BSONObject options)
 * @author chensiqin
 * @version 1.00
 *
 */

public class TestQueryExplain7091 extends SdbTestBase {
    private Sequoiadb sdb;
    private CollectionSpace cs;
    private DBCollection cl;
    private String clName = "cl7091";
    private ArrayList< BSONObject > insertRecods;

    @BeforeClass
    public void setUp() {
        String coordAddr = SdbTestBase.coordUrl;
        try {
            this.sdb = new Sequoiadb( coordAddr, "", "" );
            this.cs = this.sdb.getCollectionSpace( SdbTestBase.csName );
            createCL();

        } catch ( BaseException e ) {
            Assert.fail(
                    "Sequoiadb driver TestQueryExplain7091 setUp error, error description:"
                            + e.getMessage() );
        }
    }

    public void createCL() {
        if ( this.cs.isCollectionExist( clName ) ) {
            this.cs.dropCollection( clName );
        }
        this.cl = this.cs.createCollection( clName );
        this.cl.createIndex( "ageIndex", ( BSONObject ) JSON.parse( "{age:1}" ),
                false, false );
    }

    @Test
    public void test() {
        insertData();
        checkExplainHint();
        checkExplainNoHint();
    }

    public void checkExplainHint() {
        BSONObject matcher, selector, orderBy, hint;
        long skipRows, returnRows;
        int flag;
        BSONObject options;
        try {
            matcher = ( BSONObject ) JSON.parse( "{_id:{$gt:20}}" );
            selector = ( BSONObject ) JSON.parse( "{num:{$include:0}}" );
            orderBy = ( BSONObject ) JSON.parse( "{_id:-1}" );
            hint = ( BSONObject ) JSON.parse( "{\"\":\"ageIndex\"}" );
            skipRows = 0;
            returnRows = 5;
            flag = DBQuery.FLG_QUERY_FORCE_HINT;
            options = new BasicBSONObject();
            options.put( "Run", true );
            DBCursor cursor = this.cl.explain( matcher, selector, orderBy, hint,
                    skipRows, returnRows, flag, options );
            BSONObject actualObject = new BasicBSONObject();
            BSONObject expectedObject = new BasicBSONObject();
            expectedObject.put( "ScanType", "ixscan" );
            expectedObject.put( "IndexName", "ageIndex" );
            expectedObject.put( "IndexRead", 25 );
            expectedObject.put( "DataRead", 25 );
            expectedObject.put( "ReturnNum", 4 );
            while ( cursor.hasNext() ) {
                BSONObject obj = cursor.getNext();
                actualObject.put( "ScanType", obj.get( "ScanType" ) );
                actualObject.put( "IndexName", obj.get( "IndexName" ) );
                actualObject.put( "IndexRead", obj.get( "IndexRead" ) );
                actualObject.put( "DataRead", obj.get( "DataRead" ) );
                actualObject.put( "ReturnNum", obj.get( "ReturnNum" ) );
            }
            cursor.close();
            Assert.assertEquals( actualObject, expectedObject,
                    "Sequoiadb driver TestQueryExplain7091 checkExplainHint"
                            + "actualList:" + actualObject + "; expectedList:"
                            + expectedObject );
        } catch ( BaseException e ) {
            System.out.println(
                    "Sequoiadb driver TestQueryExplain7091 checkExplainHint error, error description:"
                            + e.getMessage() );
            Assert.fail(
                    "Sequoiadb driver TestQueryExplain7091 checkExplainHint error:"
                            + e.getMessage() );
        }
    }

    public void checkExplainNoHint() {
        BSONObject matcher, selector, orderBy, hint;
        long skipRows, returnRows;
        int flag;
        BSONObject options;
        try {
            matcher = ( BSONObject ) JSON.parse( "{num:{$gt:20}}" );
            selector = ( BSONObject ) JSON.parse( "{num:{$include:0}}" );
            orderBy = ( BSONObject ) JSON.parse( "{num:-1}" );
            hint = ( BSONObject ) JSON.parse( "{\"\":\"\"}" );
            skipRows = 0;
            returnRows = 5;
            flag = DBQuery.FLG_QUERY_WITH_RETURNDATA;
            options = new BasicBSONObject();
            options.put( "Run", true );
            DBCursor cursor = this.cl.explain( matcher, selector, orderBy, hint,
                    skipRows, returnRows, flag, options );
            BSONObject actualObject = new BasicBSONObject();
            BSONObject expectedObject = new BasicBSONObject();
            expectedObject.put( "ScanType", "tbscan" );
            expectedObject.put( "IndexName", "" );
            expectedObject.put( "IndexRead", 0 );
            expectedObject.put( "DataRead", 25 );
            expectedObject.put( "ReturnNum", 4 );
            while ( cursor.hasNext() ) {
                BSONObject obj = cursor.getNext();
                actualObject.put( "ScanType", obj.get( "ScanType" ) );
                actualObject.put( "IndexName", obj.get( "IndexName" ) );
                actualObject.put( "IndexRead", obj.get( "IndexRead" ) );
                actualObject.put( "DataRead", obj.get( "DataRead" ) );
                actualObject.put( "ReturnNum", obj.get( "ReturnNum" ) );
            }
            cursor.close();
            Assert.assertEquals( actualObject, expectedObject,
                    "Sequoiadb driver TestQueryExplain7091 checkExplainNoHint"
                            + "actualList:" + actualObject + "; expectedList:"
                            + expectedObject );
        } catch ( BaseException e ) {
            Assert.fail(
                    "Sequoiadb driver TestQueryExplain7091 checkExplainNoHint error:"
                            + e.getMessage() );
        }
    }

    public void insertData() {
        try {
            BSONObject bson;
            this.insertRecods = new ArrayList< BSONObject >();
            for ( int i = 0; i < 25; i++ ) {
                bson = new BasicBSONObject();
                bson.put( "_id", i );
                bson.put( "name", "zhangsan" + i );
                bson.put( "age", -1 * i );
                bson.put( "num", i );
                bson.put( "height", i );
                this.insertRecods.add( bson );
            }
            this.cl.bulkInsert( this.insertRecods, 0 );
        } catch ( BaseException e ) {
            Assert.fail(
                    "Sequoiadb driver TestQueryExplain7091 insert recods error:"
                            + e.getMessage() );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( this.cs.isCollectionExist( clName ) ) {
                this.cs.dropCollection( clName );
            }
            this.sdb.disconnect();
        } catch ( BaseException e ) {
            Assert.fail( "Sequoiadb driver TestQueryExplain7091 tearDown error:"
                    + e.getMessage() );
        }
    }
}
