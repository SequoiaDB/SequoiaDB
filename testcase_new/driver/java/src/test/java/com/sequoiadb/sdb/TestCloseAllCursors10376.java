package com.sequoiadb.sdb;

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
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

public class TestCloseAllCursors10376 extends SdbTestBase {
    private Sequoiadb sdb;
    private CollectionSpace cs;
    private DBCollection cl;
    private String clName = "cl10376";
    private ArrayList< BSONObject > insertRecods;

    @BeforeClass
    public void setUp() {
        try {
            this.sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            this.cs = this.sdb.getCollectionSpace( SdbTestBase.csName );
            createCL();

        } catch ( BaseException e ) {
            Assert.fail(
                    "Sequoiadb driver TestCloseAllCursors10376 setUp error, error description:"
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
    public void testCloseAllCursors() {
        this.insertData();
        try {
            DBCursor cursor1 = this.cl.query();
            DBCursor cursor2 = this.cl.query();
            DBCursor cursor3 = this.cl.query();
            DBCursor cursor4 = this.cl.query();
            this.sdb.closeAllCursors();
            while ( cursor1.hasNext() || cursor2.hasNext() || cursor3.hasNext()
                    || cursor4.hasNext() ) {
                cursor1.getNext();
                cursor2.getNext();
                cursor3.getNext();
                cursor4.getNext();
            }
            Assert.fail(
                    "Sequoiadb driver TestCloseAllCursors10376 testServerAddress should fail!" );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -31 );
        }

        try {
            DBCursor cursor1 = this.cl.query();
            DBCursor cursor2 = this.cl.query();
            DBCursor cursor3 = this.cl.query();
            DBCursor cursor4 = this.cl.query();
            this.sdb.releaseResource();
            while ( cursor1.hasNext() || cursor2.hasNext() || cursor3.hasNext()
                    || cursor4.hasNext() ) {
                cursor1.getNext();
                cursor2.getNext();
                cursor3.getNext();
                cursor4.getNext();
            }
            Assert.fail(
                    "Sequoiadb driver TestCloseAllCursors10376 testReleaseResource should fail!" );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -31 );
        }
    }

    public void insertData() {
        try {
            BSONObject bson;
            this.insertRecods = new ArrayList< BSONObject >();
            for ( int i = 0; i < 10; i++ ) {
                bson = new BasicBSONObject();
                bson.put( "name", "zhangsan" + i );
                bson.put( "age", i );
                bson.put( "num", i );
                this.insertRecods.add( bson );
            }
            this.cl.bulkInsert( this.insertRecods, 0 );
        } catch ( BaseException e ) {
            Assert.fail(
                    "Sequoiadb driver TestCloseAllCursors10376 insertData error, error description:"
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
            e.printStackTrace();
        }
    }
}