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
import org.testng.annotations.AfterTest;
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
 * @FileName:testQueryAndUpdate7088 queryAndUpdate (BSONObject matcher,
 *                                  BSONObject selector, BSONObject orderBy,
 *                                  BSONObject hint, BSONObject update, long
 *                                  skipRows, long returnRows, int flag, boolean
 *                                  returnNew) query.update()
 * @author chensiqin
 * @version 1.00
 *
 */

public class testQueryAndUpdate7088 extends SdbTestBase {
    private Sequoiadb sdb;
    private CollectionSpace cs;
    private DBCollection cl;
    private String clName = "cl7088";
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
                    "Sequoiadb driver testQuery7088 setUp error, error description:"
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
        checkQueryAndUpdate();
    }

    public void insertData() {
        try {
            BSONObject bson;
            this.insertRecods = new ArrayList< BSONObject >();
            String str = "33333333";
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
                    bson.put( "name", "zhangsan" + i );
                    bson.put( "age", random.nextDouble() );
                    bson.put( "bin", str.getBytes() );
                    bson.put( "num", random.nextInt( 100 ) );
                    bson.put( "height", random.nextFloat() );
                } else if ( i % 3 == 1 ) {
                    bson.put( "age", random.nextDouble() );
                    bson.put( "bin", str.getBytes() );
                    bson.put( "num", random.nextInt( 100 ) );
                    bson.put( "height", random.nextFloat() );
                    bson.put( "name", "zhangsan" + i );
                } else if ( i % 3 == 2 ) {
                    bson.put( "num", random.nextInt( 100 ) );
                    bson.put( "height", random.nextFloat() );
                    bson.put( "name", "zhangsan" + i );
                    bson.put( "bin", str.getBytes() );
                    bson.put( "age", random.nextDouble() );
                }
                this.insertRecods.add( bson );
            }
            this.cl.bulkInsert( this.insertRecods, 0 );
        } catch ( BaseException e ) {
            Assert.fail(
                    "Sequoiadb driver testQueryAndUpdate7088 insert recods error:"
                            + e.getMessage() );
        }
    }

    public void checkQueryAndUpdate() {
        BSONObject matcher, selector, orderBy, hint, update;
        long skipRows, returnRows;
        int flag;
        boolean returnNew;
        try {
            matcher = ( BSONObject ) JSON
                    .parse( "{$and:[{_id:{$gte:5}},{_id:{$lt:9}}]}" );
            selector = ( BSONObject ) JSON
                    .parse( "{bin:{$include:0},num:{$include:0}}" );
            orderBy = ( BSONObject ) JSON.parse( "{_id:-1}" );
            hint = ( BSONObject ) JSON.parse( "{\"\":\"idIndex\"}" );
            update = ( BSONObject ) JSON.parse( "{$inc:{age:2}}" );
            skipRows = 2;
            returnRows = 0;
            flag = DBQuery.FLG_QUERY_FORCE_HINT;
            returnNew = true;
            DBCursor dbCursor = this.cl.queryAndUpdate( matcher, selector,
                    orderBy, hint, update, skipRows, returnRows, flag,
                    returnNew );
            List< BSONObject > actualList = new ArrayList< BSONObject >();
            while ( dbCursor.hasNext() ) {
                BSONObject obj = dbCursor.getNext();
                actualList.add( obj );
            }
            dbCursor.close();
            List< BSONObject > expectedList = new ArrayList< BSONObject >();
            Assert.assertEquals( actualList, expectedList,
                    "Sequoiadb driver testQueryAndUpdate7088 checkQueryAndUpdate"
                            + "actualList:" + actualList.toString()
                            + "; expectedList:" + expectedList.toString() );

            dbCursor = this.cl.queryAndUpdate( matcher, selector, orderBy, hint,
                    update, skipRows, 1, -100, returnNew );
            while ( dbCursor.hasNext() ) {
                BSONObject obj = dbCursor.getNext();
                obj.removeField( "height" );
                obj.removeField( "age" );
                Assert.assertEquals( obj.toString(),
                        "{ \"_id\" : 6 , \"name\" : \"zhangsan6\" }" );
            }
            dbCursor.close();
        } catch ( BaseException e ) {
            Assert.fail(
                    "Sequoiadb driver testQueryAndUpdate7088 checkQueryAndUpdate error:"
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
                    "Sequoiadb driver testQueryAndUpdate7088 tearDown error:"
                            + e.getMessage() );
        }
    }
}
