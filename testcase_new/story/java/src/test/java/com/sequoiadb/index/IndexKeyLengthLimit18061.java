package com.sequoiadb.index;

import java.util.ArrayList;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * FileName: IndexKeyLengthLimit18061.java test content:Insert index key length
 * is maximum. then create index, specify multiple index fields testlink
 * case:seqDB-18061
 * 
 * @author wuyan
 * @Date 2019.3.28
 * @version 1.00
 */

public class IndexKeyLengthLimit18061 extends SdbTestBase {
    @DataProvider(name = "pagesizeProvider")
    public Object[][] generatePageSize() {
        return new Object[][] {
                // the parameter : Pagesize / the first / second /third field
                // value length of the index key
                /*
                 * Maximum length of index value--Maximum length of two fields
                 * value: 1024--(1011 - 7*2);2048--(2035 - 7*2);4096--(4083 -
                 * 7*2);
                 */
                // pagesize is 0, the maximum length of fields value is
                // 4069,eg:2 + 4000 + 67
                new Object[] { 0, 2, 4000, 67 },
                // pagesize is 4096,the maximum length of field value is
                // 997,eg:100 + 400 + 497
                new Object[] { 4096, 100, 400, 497 },
                // pagesize is 8192,the maximum length of field value is
                // 2021,eg:2020 + 1 + 0
                new Object[] { 8192, 1, 2020, 0 },
                // pagesize is 16384,the maximum length of field value is
                // 4069,eg:3000 + 1068 + 1
                new Object[] { 16384, 3000, 1068, 1 },
                // pagesize is 32768,the maximum length of field value is
                // 4069,eg:1000 + 2000 + 1069
                new Object[] { 32768, 1000, 2000, 1069 },
                // pagesize is 65536,the maximum length of field value is
                // 4069,eg: 2021 + 2000 + 48
                new Object[] { 65536, 2021, 2000, 48 }, };
    }

    private String csName = "index_18061";
    private String clName = "index_18061";

    @BeforeClass
    public void setUp() {
    }

    @Test(dataProvider = "pagesizeProvider")
    public void testIndexInAnyPageSize( int pageSize, int length1, int length2,
            int length3 ) {
        try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" ) ;) {
            String subCSName = csName + "_pagesize" + pageSize;
            DBCollection cl = IndexUtils.createCSAndCL( sdb, subCSName, clName,
                    pageSize );

            int recordNum = 20000;
            ArrayList< BSONObject > insertRecords = insertData( cl, recordNum,
                    length1, length2, length3 );

            cl.createIndex( "testindex", "{'testa':1,'str':-1,'stra':1}", true,
                    false );
            Assert.assertTrue( cl.isIndexExist( "testindex" ) );
            IndexUtils.checkRecords( cl, insertRecords,
                    "{'testa':{ '$exists': 1 } }", "{'':'testindex'}" );

            sdb.dropCollectionSpace( subCSName );
        }
    }

    @AfterClass
    public void tearDown() {
    }

    private ArrayList< BSONObject > insertData( DBCollection dbcl,
            int recordNum, int length1, int length2, int length3 ) {
        ArrayList< BSONObject > insertRecords = new ArrayList< BSONObject >();
        int batchNums = 10000;
        int count = 0;
        for ( int i = 0; i < recordNum / batchNums; i++ ) {
            ArrayList< BSONObject > insertRecord = new ArrayList< BSONObject >();
            for ( int j = 0; j < batchNums; j++ ) {
                String keyValue1 = IndexUtils.getRandomString( length1 );
                String keyValue2 = IndexUtils.getRandomString( length2 );
                String keyValue3 = IndexUtils.getRandomString( length3 );
                BSONObject obj = new BasicBSONObject();
                obj.put( "testa", keyValue1 );
                obj.put( "str", keyValue2 );
                obj.put( "stra", keyValue3 );
                obj.put( "no", count++ );
                insertRecord.add( obj );
                insertRecords.add( obj );
            }
            dbcl.insert( insertRecord );
            insertRecord = null;
        }
        return insertRecords;
    }
}
