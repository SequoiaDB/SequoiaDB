package com.sequoiadb.basicoperation;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import org.bson.BSONCallback;
import org.bson.BSONDecoder;
import org.bson.BSONObject;
import org.bson.BasicBSONCallback;
import org.bson.BasicBSONDecoder;
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
 * @FileName:TestQuery7086 query (String matcher, String selector, String
 *                         orderBy, String hint, int flag)
 * @author chensiqin
 * @version 1.00
 *
 */

public class TestQuery7087 extends SdbTestBase {
    private Sequoiadb sdb;
    private CollectionSpace cs;
    private DBCollection cl;
    private String clName = "cl7087";
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
                    "Sequoiadb driver TestQuery7087 setUp error, error description:"
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
        checkQueryWithFlag1();
        checkQueryWithFlag128();
        checkQueryWithFlag256();
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
            Assert.fail( "Sequoiadb driver TestQuery7086 insert recods error:"
                    + e.getMessage() );
        }
    }

    public void checkQueryWithFlag128() {
        String matcher, selector, orderBy, hint;
        try {
            matcher = "{_id:{$gt:20}}";
            selector = "{num:{$include:0}}";
            orderBy = "{_id:-1}";
            hint = "{\"\":\"ageIndex\"}";
            DBCursor cursor = this.cl.query( matcher, selector, orderBy, hint,
                    DBQuery.FLG_QUERY_FORCE_HINT );
            int actual = 0;
            while ( cursor.hasNext() ) {
                cursor.getNext();
                actual++;
            }
            cursor.close();
            Assert.assertEquals( actual, 4,
                    "TestQuery7087 flag = DBQuery.FLG_QUERY_PARALLED actual:"
                            + actual + "; expected:" + 4 );
        } catch ( BaseException e ) {
            Assert.fail(
                    "Sequoiadb driver TestQuery7087 checkQueryWithFlag1 error:"
                            + e.getMessage() );
        }
    }

    public void checkQueryWithFlag1() {
        String matcher, selector, orderBy, hint;
        try {
            matcher = "{_id:{$et:20}}";
            selector = "{num:{$include:1},age:{$include:1}}";
            orderBy = "{age:1}";
            hint = "{\"\":\"ageIndex\"}";
            DBCursor cursor = this.cl.query( matcher, selector, orderBy, hint,
                    DBQuery.FLG_QUERY_STRINGOUT );
            BSONObject actual = new BasicBSONObject();
            BSONObject expected = new BasicBSONObject();
            while ( cursor.hasNextRaw() ) {
                byte[] bytes = cursor.getNextRaw();
                actual = byteArrayToBSONObject( bytes );
            }
            cursor.close();
            expected.put( "", "20|-20" );
            Assert.assertEquals( actual, expected,
                    "TestQuery7087 flag = DBQuery.FLG_QUERY_PARALLED actual:"
                            + actual + "; expected:" + expected );
        } catch ( BaseException e ) {
            Assert.fail(
                    "Sequoiadb driver TestQuery7087 checkQueryWithFlag128 error:"
                            + e.getMessage() );
        }
    }

    public void checkQueryWithFlag256() {
        String matcher, selector, orderBy, hint;
        try {
            matcher = "{}";
            selector = "{}";
            orderBy = "{}";
            hint = "{}";
            DBCursor cursor = this.cl.query( matcher, selector, orderBy, hint,
                    DBQuery.FLG_QUERY_PARALLED );
            List< BSONObject > actualList = new ArrayList< BSONObject >();
            while ( cursor.hasNext() ) {
                BSONObject obj = cursor.getNext();
                actualList.add( obj );
            }
            cursor.close();
            Assert.assertEquals( actualList, this.insertRecods,
                    "TestQuery7087 flag = DBQuery.FLG_QUERY_PARALLED actual:"
                            + actualList + "; expected:" + this.insertRecods );
        } catch ( BaseException e ) {
            Assert.fail(
                    "Sequoiadb driver TestQuery7087 checkQueryWithFlag256 error:"
                            + e.getMessage() );
        }
    }

    public BSONObject byteArrayToBSONObject( byte[] array )
            throws BaseException {
        if ( array == null || array.length == 0 )
            return null;

        BSONDecoder d = new BasicBSONDecoder();
        BSONCallback cb = new BasicBSONCallback();
        try {
            d.decode( new ByteArrayInputStream( array, 0, array.length ), cb );
            BSONObject o1 = ( BSONObject ) cb.get();
            return o1;
        } catch ( IOException e ) {
            throw new BaseException( "SDB_INVALIDARG", e );
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
            Assert.fail( "Sequoiadb driver TestQuery7087 tearDown error:"
                    + e.getMessage() );
        }
    }
}
