package com.sequoiadb.basicoperation;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.Random;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
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
 * @FileName:TestQuery7086 query (BSONObject matcher, BSONObject selector,
 *                         BSONObject orderBy, BSONObject hint, long skipRows,
 *                         long returnRows, int flag)
 * @author chensiqin
 * @version 1.00
 *
 */
public class TestQuery7086 extends SdbTestBase {
    private Sequoiadb sdb;
    private CollectionSpace cs;
    private DBCollection cl;
    private String clName = "cl7086";
    private ArrayList< BSONObject > insertRecods;

    @BeforeTest
    public void setUp() {
        String coordAddr = SdbTestBase.coordUrl;
        try {
            this.sdb = new Sequoiadb( coordAddr, "", "" );
            this.cs = this.sdb.getCollectionSpace( SdbTestBase.csName );
            createCL();
        } catch ( BaseException e ) {
            Assert.fail(
                    "Sequoiadb driver TestQuery7086 setUp error, error description:"
                            + e.getMessage() );
        }
    }

    public void createCL() {
        if ( this.cs.isCollectionExist( clName ) ) {
            this.cs.dropCollection( clName );
        }
        this.cl = this.cs.createCollection( clName );
        this.cl.createIndex( "ageIndex",
                ( BSONObject ) JSON.parse( "{age:-1}" ), false, false );
    }

    @Test
    public void test() {
        insertData();
        checkQuery();
    }

    public void insertData() {
        try {
            BSONObject bson;
            this.insertRecods = new ArrayList< BSONObject >();
            String str = "22222222";
            while ( str.getBytes().length / 1048576 < 1 ) {
                str += str;
            }
            BSONObject strObject = new BasicBSONObject();
            for ( int j = 1; j < 16; j++ ) {
                strObject.put( "obj" + j, str.getBytes() );
            }
            Random random = new Random();
            for ( int i = 0; i < 25; i++ ) {
                bson = new BasicBSONObject();
                bson.put( "_id", i );
                if ( i % 3 == 0 ) {
                    bson.put( "name", random.nextLong() );
                    bson.put( "age", random.nextDouble() );
                    bson.put( "bin", str.getBytes() );
                    bson.put( "num", random.nextInt( 100 ) );
                    bson.put( "height", 0.1 );
                } else if ( i % 3 == 1 ) {
                    bson.put( "age", random.nextDouble() );
                    bson.put( "bin", str.getBytes() );
                    bson.put( "num", random.nextInt( 100 ) );
                    bson.put( "height", 0.2 );
                    bson.put( "name", random.nextLong() );
                } else if ( i % 3 == 2 ) {
                    bson.put( "num", random.nextInt( 100 ) );
                    bson.put( "height", 0.3 );
                    bson.put( "name", random.nextLong() );
                    bson.put( "bin", str.getBytes() );
                    bson.put( "age", random.nextDouble() );
                }
                this.insertRecods.add( bson );
            }
            this.cl.bulkInsert( this.insertRecods, 0 );
        } catch ( BaseException e ) {
            Assert.fail( "Sequoiadb driver TestQuery7086 insert recods error:"
                    + e.getMessage() );
        }
    }

    public void checkQuery() {
        BSONObject matcher, selector, orderBy, hint;
        long skipRows, returnRows;
        int flag;
        try {
            matcher = ( BSONObject ) JSON.parse( "{_id:{$gte:5}}" );
            selector = ( BSONObject ) JSON.parse( "{bin:{$include:0}}" );
            orderBy = ( BSONObject ) JSON.parse( "{\"_id\":1}" );
            hint = ( BSONObject ) JSON.parse( "{\"\":\"ageIndex\"}" );
            skipRows = 3;
            returnRows = 6;
            flag = DBQuery.FLG_QUERY_FORCE_HINT;
            DBCursor dbCursor = this.cl.query( matcher, selector, orderBy, hint,
                    skipRows, returnRows, flag );
            List< BSONObject > actualList = new ArrayList< BSONObject >();
            while ( dbCursor.hasNext() ) {
                BSONObject object = dbCursor.getNext();
                actualList.add( object );
            }
            dbCursor.close();
            List< BSONObject > expectedList = new ArrayList< BSONObject >();
            for ( int i = 8; i < 14; i++ ) {
                BSONObject obj = this.insertRecods.get( i );
                obj.removeField( "bin" );
                expectedList.add( obj );
            }
            Assert.assertEquals( actualList, expectedList,
                    "actualList:" + actualList.toString() + "; expectedList:"
                            + expectedList.toString() );
            try {
                this.cl.query( matcher, selector, orderBy, hint, skipRows,
                        returnRows, -100 );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -6 && e.getErrorCode() != -288 ) {
                    throw e;
                }
            }
        } catch ( BaseException e ) {
            Assert.fail( "Sequoiadb driver TestQuery7086 checkQuery error:"
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
            Assert.fail( "Sequoiadb driver TestQuery7086 tearDown error:"
                    + e.getMessage() );
        }
    }

}
