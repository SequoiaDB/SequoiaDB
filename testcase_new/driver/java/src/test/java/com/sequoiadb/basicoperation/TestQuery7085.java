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
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @FileName:TestQuery7085
 * @author chensiqin
 * @Date 2016-09-19
 * @version 1.00
 *
 */

public class TestQuery7085 extends SdbTestBase {
    private Sequoiadb sdb;
    private CollectionSpace cs;
    private DBCollection cl;
    private String clName = "cl7085";
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
                    "Sequoiadb driver TestQuery7085 setUp error, error description:"
                            + e.getMessage() );
        }
    }

    public void createCL() {
        if ( this.cs.isCollectionExist( clName ) ) {
            this.cs.dropCollection( clName );
        }
        this.cl = this.cs.createCollection( clName );
        this.cl.createIndex( "nameIndex",
                ( BSONObject ) JSON.parse( "{name:1}" ), false, false );
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
            String str = "11111111";
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
                if ( i % 4 == 0 ) {
                    bson.put( "name", random.nextInt( 100 ) );
                    bson.put( "age", random.nextInt( 100 ) );
                    bson.put( "bin", str.getBytes() );
                    bson.put( "num", random.nextInt( 100 ) );
                    bson.put( "height", 168 + i );
                } else if ( i % 4 == 1 ) {
                    bson.put( "age", random.nextInt( 100 ) );
                    bson.put( "bin", str.getBytes() );
                    bson.put( "num", random.nextInt( 100 ) );
                    bson.put( "height", 168 + i );
                    bson.put( "name", random.nextInt( 100 ) );
                } else if ( i % 4 == 2 ) {
                    bson.put( "num", random.nextInt( 100 ) );
                    bson.put( "height", 168 + i );
                    bson.put( "name", random.nextInt( 100 ) );
                    bson.put( "bin", str.getBytes() );
                    bson.put( "age", random.nextInt( 100 ) );
                } else {
                    bson.put( "name", random.nextInt( 100 ) );
                    bson.put( "age", random.nextInt( 100 ) );
                    bson.put( "num", random.nextInt( 100 ) );
                    bson.put( "height", 168 + i );
                    bson.put( "bin", str.getBytes() );
                }
                this.insertRecods.add( bson );
            }
            this.cl.bulkInsert( this.insertRecods, 0 );
        } catch ( BaseException e ) {
            Assert.fail( "Sequoiadb driver TestQuery7085 insert recods error:"
                    + e.getMessage() );
        }
    }

    /**
     * query.limit/skip/sort
     */
    public void checkQuery() {
        String matcher, selector, orderBy, hint;
        long skipRows, returnRows;
        try {
            matcher = "{_id:{$gte:10}}";
            selector = "{bin:{$include:0}}";
            orderBy = "{\"_id\":-1}";
            hint = "{\"\":\"nameIndex\"}";
            skipRows = 1;
            returnRows = 5;
            DBCursor cursor = this.cl.query( matcher, selector, orderBy, hint,
                    skipRows, returnRows );
            List< BSONObject > actualList = new ArrayList< BSONObject >();
            while ( cursor.hasNext() ) {
                BSONObject object = cursor.getNext();
                actualList.add( object );
            }
            cursor.close();
            List< BSONObject > expectedList = new ArrayList< BSONObject >();
            int expectedListLen = this.insertRecods.size();
            for ( int i = expectedListLen - 2; i > expectedListLen - 7; i-- ) {
                BSONObject obj = this.insertRecods.get( i );
                obj.removeField( "bin" );
                expectedList.add( obj );
            }

            Assert.assertEquals( actualList, expectedList,
                    "actualList:" + actualList.toString() + "; expectedList:"
                            + expectedList.toString() );
        } catch ( BaseException e ) {
            Assert.fail( "Sequoiadb driver TestQuery7085 checkQuery error :"
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
            Assert.fail( "Sequoiadb driver TestQuery7085 tearDown error:"
                    + e.getMessage() );
        }
    }
}
