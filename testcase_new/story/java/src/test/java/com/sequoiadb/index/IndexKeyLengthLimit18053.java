package com.sequoiadb.index;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.Test;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.*;

/**
 * FileName: IndexKeyLengthLimit18053.java test content:create index, specify
 * multiple index fields,the index fields value exceeds the limit. testlink
 * case:seqDB-18053 *
 * 
 * @author wuyan
 * @Date 2019.3.28
 * @version 1.00
 */

public class IndexKeyLengthLimit18053 extends SdbTestBase {
    @DataProvider(name = "pagesizeProvider", parallel = true)
    public Object[][] generatePageSize() {
        return new Object[][] {
                // the parameter : Pagesize / the first field value length /
                // second field value length of the index key
                /*
                 * Maximum length of index value--Maximum length of two fields
                 * value: 1024--(1011 - 7);2048--(2035 - 7);4096--(4083 - 7);
                 */
                // pagesize is 0, the maximum length of fields value is
                // 4076,eg:2 + 4074
                new Object[] { 0, 2, 4074 },
                // pagesize is 4096,the maximum length of field value is
                // 1004,eg:1000 + 4
                new Object[] { 4096, 1000, 1004 - 1000 },
                // pagesize is 8192,the maximum length of field value is
                // 2028,eg:2027 + 1
                new Object[] { 8192, 1, 2028 - 1 },
                // pagesize is 16384,the maximum length of field value is
                // 4076,eg:3000 + 1076
                new Object[] { 16384, 3000, 4076 - 3000 },
                // pagesize is 32768,the maximum length of field value is
                // 4076,eg:1 + 4075
                new Object[] { 32768, 1, 4076 - 1 },
                // pagesize is 65536,the maximum length of field value is
                // 4076,eg: 2028 + 2048
                new Object[] { 65536, 2028, 4076 - 2028 }, };
    }

    private String csName = "index_18053";
    private String clName = "index_18053";

    @BeforeClass
    public void setUp() {
    }

    // create index then insert
    @Test(dataProvider = "pagesizeProvider")
    public void testIndexInAnyPageSize( int pageSize, int length1,
            int length2 ) {
        try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" ) ;) {
            String subCSName = csName + "_pagesize" + pageSize;
            DBCollection cl = IndexUtils.createCSAndCL( sdb, subCSName, clName,
                    pageSize );
            cl.createIndex( "testindex", "{'testa':1,'str':-1}", true, false );
            insertData( cl, length1, length2, false );

            // cl record is empty:0
            Assert.assertEquals( cl.getCount(), 0 );

            sdb.dropCollectionSpace( subCSName );
        }
    }

    // insert then create index
    @Test(dataProvider = "pagesizeProvider")
    public void testIndexInAnyPageSize1( int pageSize, int length1,
            int length2 ) {
        try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" ) ;) {
            String subCSName = csName + "_ps" + pageSize;
            DBCollection cl = IndexUtils.createCSAndCL( sdb, subCSName, clName,
                    pageSize );
            // the keyValue length exceeds maximum:maxlength + 1
            insertData( cl, length1, length2, true );

            createIndexFail( cl );
            Assert.assertFalse( cl.isIndexExist( "testindex" ),
                    "check the index not exist!" );

            sdb.dropCollectionSpace( subCSName );
        }
    }

    @AfterClass
    public void tearDown() {
    }

    private void insertData( DBCollection dbcl, int length1, int length2,
            boolean isSuccessflag ) {
        // the keyValue length exceeds maximum:maxlength + 1
        String keyValue1 = IndexUtils.getRandomString( length1 + 1 );
        String keyValue2 = IndexUtils.getRandomString( length2 );
        try {
            BSONObject obj = new BasicBSONObject();
            obj.put( "testa", keyValue1 );
            obj.put( "str", keyValue2 );
            obj.put( "no", 1 );
            dbcl.insert( obj );

            if ( !isSuccessflag ) {
                Assert.fail( "insert should fail!" );
            }
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
            dbcl.createIndex( "testindex", "{'testa':1,'str':-1}", true,
                    false );
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
