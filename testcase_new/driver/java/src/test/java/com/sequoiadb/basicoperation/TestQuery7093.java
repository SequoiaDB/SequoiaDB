package com.sequoiadb.basicoperation;

import java.util.ArrayList;
import java.util.Date;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.AfterTest;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.BeforeTest;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @FileName:TestQueryExplain7091
 * @author chensiqin
 * @version 1.00
 *
 */

public class TestQuery7093 extends SdbTestBase {
    private Sequoiadb sdb;
    private CollectionSpace cs;
    private DBCollection cl;
    private String clName = "cl7093";
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
                    "Sequoiadb driver TestQuery7093 setUp error, error description:"
                            + e.getMessage() );
        }
    }

    public void createCL() {
        if ( this.cs.isCollectionExist( clName ) ) {
            this.cs.dropCollection( clName );
        }
        this.cl = this.cs.createCollection( clName );
    }

    @Test
    public void test() {
        insertData();
        checkQueryConError();
    }

    public void checkQueryConError() {
        try {
            this.cl.query( "{name:{$include:0}}", "{_id:{$gt:2}}", "{_id:-1}",
                    "{\"\":\"\"}", 0, 2 );
            Assert.fail(
                    "Sequoiadb driver TestQuery7093 checkQueryConError should fail!" );
        } catch ( BaseException e ) {
            BSONObject actual = new BasicBSONObject();
            BSONObject expected = new BasicBSONObject();
            expected.put( "code", -6 );
            // expected.put("message", "errorCode:-6,Invalid Argument");
            actual.put( "code", e.getErrorCode() );
            // actual.put("message", e.getMessage());
            Assert.assertEquals( actual, expected );
        }
    }

    public void insertData() {
        try {
            BSONObject bson;
            this.insertRecods = new ArrayList< BSONObject >();
            for ( int i = 0; i < 10; i++ ) {
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
            Assert.fail( "Sequoiadb driver TestQuery7093 insert recods error:"
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
            Assert.fail( "Sequoiadb driver TestQuery7093 tearDown error:"
                    + e.getMessage() );
        }
    }
}
