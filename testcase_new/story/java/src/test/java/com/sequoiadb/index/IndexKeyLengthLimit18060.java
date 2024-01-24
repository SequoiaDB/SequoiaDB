package com.sequoiadb.index;

import org.testng.annotations.AfterClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import org.bson.BSONObject;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.*;

/**
 * FileName: IndexKeyLengthLimit18060.java test content:Insert index key length
 * is maximum. then create index testlink case:seqDB-18060
 * 
 * @author wuyan
 * @Date 2019.3.28
 * @version 1.00
 */

public class IndexKeyLengthLimit18060 extends SdbTestBase {
    @DataProvider(name = "pagesizeProvider", parallel = true)
    public Object[][] generatePageSize() {
        return new Object[][] {
                // the parameter : Pagesize and the field value length of the
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

    private String csName = "index_18060";
    private String clName = "index_18060";

    @BeforeClass
    public void setUp() {
    }

    @Test(dataProvider = "pagesizeProvider")
    public void testIndexInAnyPageSize( int pageSize, int length ) {
        try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" ) ;) {
            String subCSName = csName + "_pagesize" + pageSize;
            DBCollection cl = IndexUtils.createCSAndCL( sdb, subCSName, clName,
                    pageSize );

            int recordNum = 5000;
            ArrayList< BSONObject > insertRecords = IndexUtils.insertData( cl,
                    recordNum, length );

            cl.createIndex( "testindex", "{'testa':-1}", false, false );
            IndexUtils.checkRecords( cl, insertRecords,
                    "{'testa':{ '$exists': 1 } }", "{'':'testindex'}" );

            sdb.dropCollectionSpace( subCSName );
        }
    }

    @AfterClass
    public void tearDown() {
    }
}
