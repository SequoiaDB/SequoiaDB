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
 * FileName: DivideIsSelector12577.java test content:Numeric value overflow for
 * many character using $divide operation, and the $subtract is used as a
 * selector. testlink case:seqDB-12577
 * 
 * @author wuyan
 * @Date 2017.9.11
 * @version 1.00
 */

public class DivideIsSelector12577 extends SdbTestBase {

    private String clName = "divide_selector12577";
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

        String clOption = "{ReplSize:0,Compressed:true, StrictDataMode:false}";
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        cl = NumOverflowUtils.createCL( cs, clName, clOption );

        String[] records = {
                "{'no':-2147483648,'tlong':{'$numberLong':'-9223372036854775808'},"
                        + "'arr':[1,[1,{'$numberLong':'-9223372036854775808'}],2],obj:{a:{b:-2147483648}}}" };
        NumOverflowUtils.insert( cl, records );
    }

    @Test
    public void testDivideAsSelector() {
        try {
            // TODO:SEQUOIADBMAINSTREAM-2795,not supported the
            // arr.$[1].$[1]':{$abs:1}
            // String selector = "{no:{$abs:1},tlong:{$abs:1},"
            // +
            // "'arr.$[1].$[1]':{$abs:1},'obj.a.b':{$abs:1},_id:{$include:0}}";
            // String []expRecords =
            // {"{'no':2147483648,'tlong':{'$decimal':'9223372036854775808'},"
            // +
            // "'arr':[1,[1,{'$decimal':'9223372036854775808'}],2],obj:{a:{b:2147483648}}}"};
            String selector = "{no:{$abs:1},tlong:{$abs:1},'obj.a.b':{$abs:1},_id:{$include:0}}";
            String[] expRecords = {
                    "{'no':2147483648,'tlong':{'$decimal':'9223372036854775808'},"
                            + "'arr':[1,[1,{'$numberLong':'-9223372036854775808'}],2],obj:{a:{b:2147483648}}}" };
            NumOverflowUtils.multipleFieldOper( cl, selector, expRecords );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "divide is used as selector oper failed,"
                    + e.getMessage() + e.getErrorCode() );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( sdb.getCollectionSpace( SdbTestBase.csName )
                    .isCollectionExist( clName ) ) {
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
