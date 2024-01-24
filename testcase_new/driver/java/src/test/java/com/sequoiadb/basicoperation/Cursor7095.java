package com.sequoiadb.basicoperation;

import java.util.ArrayList;
import java.util.Date;

import com.sequoiadb.exception.SDBError;
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

public class Cursor7095 extends SdbTestBase {
    private Sequoiadb sdb;
    private String coordAddr;
    private String commCSName;

    @BeforeClass
    public void setUp() {
        this.coordAddr = SdbTestBase.coordUrl;
        this.commCSName = SdbTestBase.csName;
        try {
            sdb = new Sequoiadb( coordAddr, "", "" );
            if ( !sdb.isCollectionSpaceExist( commCSName ) ) {
                sdb.createCollectionSpace( commCSName );
            }
        } catch ( BaseException e ) {
            Assert.fail( "prepare env failed" + e.getMessage() );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            sdb.disconnect();
        } catch ( BaseException e ) {
            Assert.fail( "clear env failed, errMsg:" + e.getMessage() );
        }
    }

    @Test
    public void test() {
        CollectionSpace cs = sdb.getCollectionSpace( commCSName );
        DBCollection cl = null;
        // create cl
        String clName = "cl7095";
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
            cl = cs.createCollection( clName );
        } catch ( BaseException e ) {
            Assert.fail( "create cl:" + clName + " failed, errMsg:"
                    + e.getMessage() );
        }

        // insert data
        int dataNum = 10;
        ArrayList< BSONObject > insertData = new ArrayList< BSONObject >();
        try {
            for ( int i = 0; i < dataNum; i++ ) {
                BSONObject dataObj = new BasicBSONObject();
                dataObj.put( "a", i );
                insertData.add( dataObj );
            }
            cl.bulkInsert( insertData, 0 );
        } catch ( BaseException e ) {
            Assert.fail( "insert data:" + insertData + " failed, errMsg:"
                    + e.getMessage() );
        }

        // check getCurrent() and getNext() get the same record
        BSONObject matcher = ( BSONObject ) JSON.parse( "{a:{$lt:10}}" );
        BSONObject orderBy = ( BSONObject ) JSON.parse( "{a:1}" );
        DBCursor queryCursor = cl.query( matcher, null, orderBy, null );
        ArrayList< BSONObject > actualGetNextData = new ArrayList< BSONObject >();
        ArrayList< BSONObject > actualGetCurrentData = new ArrayList< BSONObject >();
        while ( queryCursor.hasNext() ) {
            actualGetCurrentData.add( queryCursor.getNext() );
            actualGetNextData.add( queryCursor.getCurrent() );
        }
        queryCursor.close();
        try {
            queryCursor.getNext();
            Assert.fail( "expect fail but success." );
        } catch ( BaseException e ) {
            Assert.assertEquals( SDBError.SDB_DMS_CONTEXT_IS_CLOSE.getErrorCode(), e.getErrorCode() );
        }
        Assert.assertEquals( actualGetCurrentData, insertData );
        Assert.assertEquals( actualGetCurrentData, actualGetNextData );
        // clear env
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }
    }
}
