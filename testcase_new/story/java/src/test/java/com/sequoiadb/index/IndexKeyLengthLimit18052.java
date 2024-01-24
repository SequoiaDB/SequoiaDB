package com.sequoiadb.index;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.Test;

import java.util.concurrent.atomic.AtomicInteger;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.*;

/**
 * FileName: IndexKeyLengthLimit18052.java test content:create index, the index
 * field value exceeds the limit. testlink case:seqDB-18052
 * 
 * @author wuyan
 * @Date 2019.3.28
 * @version 1.00
 */

public class IndexKeyLengthLimit18052 extends SdbTestBase {
    @DataProvider(name = "pagesizeProvider", parallel = true)
    public Object[][] generatePageSize() {
        return new Object[][] {
                // the parameter : Pagesize / the field value length of the
                // index key
                // Maximum length of index value--Maximum length of field value:
                // 1024--1011;2048--2035;4096--4083;
                new Object[] { 0, 4083 },
                // pagesize is 4096,the maximum length of field value is 1011
                new Object[] { 4096, 1011 },
                // pagesize is 8192,the maximum length of field value is 2035
                new Object[] { 8192, 2035 },
                // pagesize is 16384,the maximum length of field value is 4083
                new Object[] { 16384, 4083 },
                // pagesize is 32768,the maximum length of field value is 4083
                new Object[] { 32768, 4083 },
                // pagesize is 65536,the maximum length of field value is 4083
                new Object[] { 65536, 4083 }, };
    }

    private String csName = "index_18052";
    private String clName = "index_18052";
    private AtomicInteger count1 = new AtomicInteger( 10 );

    @BeforeClass
    public void setUp() {
    }

    // create index then insert
    @Test(dataProvider = "pagesizeProvider")
    public void testIndexInAnyPageSize( int pageSize, int length ) {
        try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" ) ;) {
            String subCSName = csName + "_pagesize" + pageSize;
            DBCollection cl = IndexUtils.createCSAndCL( sdb, subCSName, clName,
                    pageSize );

            cl.createIndex( "testindex", "{'testa':1}", true, false );
            insertDataFail( cl, length );

            // cl record is empty:0
            Assert.assertEquals( cl.getCount(), 0 );

            sdb.dropCollectionSpace( subCSName );
        }
    }

    // insert then create index
    @Test(dataProvider = "pagesizeProvider")
    public void testIndexInAnyPageSize1( int pageSize, int length ) {
        try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" ) ;) {
            int no = count1.getAndIncrement();
            String subCSName = csName + "_" + no;
            DBCollection cl = IndexUtils.createCSAndCL( sdb, subCSName, clName,
                    pageSize );

            // the keyValue length exceeds maximum:maxlength + 1
            int recordNum = 1;
            IndexUtils.insertData( cl, recordNum, length + 1 );

            createIndexFail( cl );
            Assert.assertFalse( cl.isIndexExist( "testindex" ),
                    "check the index not exist!" );

            sdb.dropCollectionSpace( subCSName );
        }
    }

    @AfterClass
    public void tearDown() {
    }

    private void insertDataFail( DBCollection dbcl, int length ) {
        // the keyValue length exceeds maximum:maxlength + 1
        String keyValue = IndexUtils.getRandomString( length + 1 );
        try {
            BSONObject obj = new BasicBSONObject();
            obj.put( "testa", keyValue );
            obj.put( "no", 1 );
            dbcl.insert( obj );
            Assert.fail( "insert should fail!" );
        } catch ( BaseException e ) {
            // error 39: index key is too large
            if ( e.getErrorCode() != -39 ) {
                Assert.fail(
                        "insert fail: e=" + e.getErrorCode() + e.getMessage() );
            }
        }
    }

    private void createIndexFail( DBCollection dbcl ) {
        try {
            dbcl.createIndex( "testindex", "{'testa':1}", true, false );
            Assert.fail( "createIndex should fail!" );
        } catch ( BaseException e ) {
            // error 39: index key is too large
            if ( e.getErrorCode() != -39 ) {
                Assert.fail( "createindex fail: e=" + e.getErrorCode()
                        + e.getMessage() );
            }
        }
    }
}
