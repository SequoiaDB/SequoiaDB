package com.sequoiadb.crud.numoverflow;

import java.util.Date;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.crud.numoverflow.NumOverflowUtils;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * FileName: AdsIsSelector12571.java test content:Numeric value overflow for
 * many character using $abs operation, and the $abs is used as a selector.
 * testlink case:seqDB-12571
 * 
 * @author wuyan
 * @Date 2017.9.1
 * @version 1.00
 */

public class AdsIsSelector12571 extends SdbTestBase {

    private String clName = "abs_selector12571";
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private static DBCollection cl = null;

    @BeforeClass
    public void setUp() {
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        } catch ( BaseException e ) {
            Assert.assertTrue( false,
                    "connect %s failed," + coordUrl + e.getMessage() );
        }

        String clOption = "{ShardingKey:{no:1},ShardingType:'hash',Partition:1024,"
                + "ReplSize:0,Compressed:true, StrictDataMode:false}";
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        cl = NumOverflowUtils.createCL( cs, clName, clOption );

        String[] records = {
                "{'no':-2147483648,'tlong':{'$numberLong':'-9223372036854775808'},'arr':[-2147483648,{'$numberLong':'-9223372036854775808'}]}" };
        NumOverflowUtils.insert( cl, records );
    }

    @Test
    public void testAds() {
        String selector = "{no:{$abs:1},tlong:{$abs:1},arr:{$abs:1},_id:{$include:0}}";
        String[] expRecords = {
                "{'no':2147483648,'tlong':{'$decimal':'9223372036854775808'},'arr':[2147483648,{'$decimal':'9223372036854775808'}]}" };

        NumOverflowUtils.multipleFieldOper( cl, selector, expRecords );
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
        } catch ( BaseException e ) {
            Assert.fail( "clear env failed, errMsg:" + e.getMessage() );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }
}
