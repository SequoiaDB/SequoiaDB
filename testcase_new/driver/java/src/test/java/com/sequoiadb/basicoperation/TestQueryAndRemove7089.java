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
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.DBQuery;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @FileName:TestQueryAndRemove7089 queryAndRemove (BSONObject matcher,
 *                                  BSONObject selector, BSONObject orderBy,
 *                                  BSONObject hint, long skipRows, long
 *                                  returnRows, int flag)
 * @author chensiqin
 * @version 1.00
 *
 */

public class TestQueryAndRemove7089 extends SdbTestBase {
    private Sequoiadb sdb;
    private CollectionSpace cs;
    private DBCollection cl;
    private String clName = "cl7089";
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
                    "Sequoiadb driver TestQueryAndRemove7089 setUp error, error description:"
                            + e.getMessage() );
        }
    }

    public void createCL() {
        if ( this.cs.isCollectionExist( clName ) ) {
            this.cs.dropCollection( clName );
        }
        this.cl = this.cs.createCollection( clName );
        this.cl.createIndex( "idIndex", ( BSONObject ) JSON.parse( "{_id:-1}" ),
                false, false );
    }

    @Test
    public void test() {
        insertData();
        checkQueryAndRemove();
    }

    public void insertData() {
        try {
            BSONObject bson;
            this.insertRecods = new ArrayList< BSONObject >();
            String str = "44444444";
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
                    bson.put( "name", i + "lisi" );
                    bson.put( "age", random.nextDouble() );
                    bson.put( "bin", str.getBytes() );
                    bson.put( "num", random.nextInt( 100 ) );
                    bson.put( "weight", 0.1314 );
                } else if ( i % 3 == 1 ) {
                    bson.put( "age", random.nextDouble() );
                    bson.put( "bin", str.getBytes() );
                    bson.put( "num", random.nextInt( 100 ) );
                    bson.put( "weight", 3.1415 );
                    bson.put( "name", i + "lisi" );
                } else if ( i % 3 == 2 ) {
                    bson.put( "num", random.nextInt( 100 ) );
                    bson.put( "weight", 6.999 );
                    bson.put( "name", i + "lisi" );
                    bson.put( "bin", str.getBytes() );
                    bson.put( "age", random.nextDouble() );
                }
                this.insertRecods.add( bson );
            }
            this.cl.bulkInsert( this.insertRecods, 0 );
        } catch ( BaseException e ) {
            Assert.fail(
                    "Sequoiadb driver TestQueryAndRemove7089 insert recods error:"
                            + e.getMessage() );
        }
    }

    public void checkQueryAndRemove() {
        BSONObject matcher, selector, orderBy, hint;
        long skipRows, returnRows;
        try {
            matcher = ( BSONObject ) JSON.parse( "{_id:{$gte:7}}" );
            selector = ( BSONObject ) JSON.parse( "{bin:{$include:0}}" );
            orderBy = ( BSONObject ) JSON.parse( "{_id:1}" );
            hint = ( BSONObject ) JSON.parse( "{\"\":\"idIndex\"}" );
            skipRows = 2;
            returnRows = 5;
            DBCursor cursor = this.cl.queryAndRemove( matcher, selector,
                    orderBy, hint, skipRows, returnRows,
                    DBQuery.FLG_QUERY_FORCE_HINT );
            List< BSONObject > actualList = new ArrayList< BSONObject >();
            List< BSONObject > expectedList = new ArrayList< BSONObject >();
            while ( cursor.hasNext() ) {
                BSONObject object = cursor.getNext();
                actualList.add( object );
            }
            cursor.close();
            for ( int i = 9; i <= 13; i++ ) {
                BSONObject obj = this.insertRecods.get( i );
                obj.removeField( "bin" );
                expectedList.add( obj );
            }
            Assert.assertEquals( actualList, expectedList,
                    "Sequoiadb driver TestQueryAndRemove7089 checkQueryAndRemove"
                            + "actualList:" + actualList.toString()
                            + "; expectedList:" + expectedList.toString() );
            cursor = this.cl.queryAndRemove( matcher, selector, orderBy, hint,
                    skipRows, 2, -100 );
            int actual = 0;
            while ( cursor.hasNext() ) {
                cursor.getNext();
                actual++;
            }
            cursor.close();
            Assert.assertEquals( actual, 2 );

        } catch ( BaseException e ) {
            Assert.fail(
                    "Sequoiadb driver TestQueryAndRemove7089 checkQueryAndRemove error:"
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
            Assert.fail(
                    "Sequoiadb driver TestQueryAndRemove7089 tearDown error:"
                            + e.getMessage() );
        }
    }
}
