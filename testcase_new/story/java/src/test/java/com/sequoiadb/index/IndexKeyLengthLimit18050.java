package com.sequoiadb.index;

import org.testng.annotations.AfterClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.*;

/**
 * FileName: IndexKeyLengthLimit18050.java test content:create index, specify
 * multiple index fields,Insert index key length is maximum. testlink
 * case:seqDB-18050
 * 
 * @author wuyan
 * @Date 2019.3.28
 * @version 1.00
 */

public class IndexKeyLengthLimit18050 extends SdbTestBase {
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

    private String csName = "index_18050";
    private String clName = "index_18050";

    @BeforeClass
    public void setUp() {
    }

    @Test(dataProvider = "pagesizeProvider")
    public void testIndexInAnyPageSize( int pageSize, int length1,
            int length2 ) {
        try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" ) ;) {
            String subCSName = csName + "_pagesize" + pageSize;
            DBCollection cl = IndexUtils.createCSAndCL( sdb, subCSName, clName,
                    pageSize );

            cl.createIndex( "testindex", "{'testa':1,'str':-1}", true, false );
            int recordNum = ( int ) ( 1 + Math.random() * 100 );
            ArrayList< BSONObject > insertRecords = insertData( cl, recordNum,
                    length1, length2 );
            IndexUtils.checkRecords( cl, insertRecords,
                    "{'testa':{ '$exists': 1 } }", "{'':'testindex'}" );

            sdb.dropCollectionSpace( subCSName );
        }
    }

    @AfterClass
    public void tearDown() {
    }

    private ArrayList< BSONObject > insertData( DBCollection dbcl,
            int recordNum, int length1, int length2 ) {
        ArrayList< BSONObject > insertRecord = new ArrayList< BSONObject >();
        for ( int i = 0; i < recordNum; i++ ) {
            int firstLen = length1 - i;
            // insert only one empty string
            if ( firstLen < 0 ) {
                firstLen = length1;
            }
            String keyValue1 = IndexUtils.getRandomString( firstLen );
            String keyValue2 = IndexUtils.getRandomString( length2 );
            BSONObject obj = new BasicBSONObject();
            obj.put( "testa", keyValue1 );
            obj.put( "str", keyValue2 );
            obj.put( "no", i );
            insertRecord.add( obj );
        }
        dbcl.insert( insertRecord );
        return insertRecord;
    }
}
