package com.sequoiadb.crud.numoverflow;

import java.util.Date;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * FileName: AbsIsMatcher12592.java test content:Numeric value overflow for many
 * character using $abs operation, and the $abs is used as a matcher. testlink
 * case:seqDB-12592
 * 
 * @author luweikang
 * @Date 2017.9.13
 * @version 1.00
 */

public class AbsIsMatcher12592 extends SdbTestBase {

    private String clName = "abs_matcher12592";
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

        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        cl = NumOverflowUtils.createCL( cs, clName );
        String[] records = {
                "{'int32':-2147483648,'long':{'$numberLong':'-9223372036854775808'},"
                        + "'arr':[-2147483648,{'$numberLong':'-9223372036854775808'}],"
                        + "obj:{a:{b:{'$numberLong':'-9223372036854775808'}}}}",
                "{'int32':-21473648,'long':{'$numberLong':'-92233724775808'},"
                        + "'arr':[-214743648,{'$numberLong':'-9223336854775808'}],"
                        + "obj:{a:{b:{'$numberLong':'-92233754775808'}}}}" };

        NumOverflowUtils.insert( cl, records );
    }

    @Test
    public void testAbs() {

        String matcher = "{'int32':{'$abs':1,'$et':{'$numberLong':'2147483648'}},'long':{'$abs':1,"
                + "'$et':{'$decimal':'9223372036854775808'}},"
                + "'arr.0':{'$abs':1,'$et':{'$numberLong':'2147483648'}},"
                + "'obj.a.b':{'$abs':1,'$et':{'$decimal':'9223372036854775808'}}}";
        String[] expRecords = {
                "{'int32':-2147483648,'long':{'$numberLong':'-9223372036854775808'},"
                        + "'arr':[-2147483648,{'$numberLong':'-9223372036854775808'}],"
                        + "obj:{a:{b:{'$numberLong':'-9223372036854775808'}}}}" };
        String indexKey = "{int32:-1,long:1,arr:1,obj:-1}";
        try {
            NumOverflowUtils.multiFieldOperAsMatcher( cl, matcher, expRecords,
                    indexKey );

        } catch ( BaseException e ) {
            Assert.assertTrue( false, "abs data is used as matcher oper failed,"
                    + e.getMessage() );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            CollectionSpace cs = sdb.getCollectionSpace( SdbTestBase.csName );
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
