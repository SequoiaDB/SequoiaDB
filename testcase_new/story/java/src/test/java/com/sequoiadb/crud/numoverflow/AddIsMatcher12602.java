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
 * FileName: AddIsMatcher12602.java test content:Numeric value overflow for many
 * character using $add operation, and the $add is used as a matcher. testlink
 * case:seqDB-12602
 * 
 * @author wuyan
 * @Date 2017.9.12
 * @version 1.00
 */

public class AddIsMatcher12602 extends SdbTestBase {

    private String clName = "add_matcher12602";
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
                "{'a':-2147483648,'b':{'$numberLong':'-9223372036854775808'},"
                        + "c:{a:{b:{int:123,long:{'$numberLong':'1024819115206086201'}}}},"
                        + "d:[1,2147483647],e:[3,[1,{'$numberLong':'9223372036854775807'}]]}"
                        + "{'a':12,b:{'$numberLong':'-9223372036854775807'},c:{a:b:1},d:1,e:1}" };

        NumOverflowUtils.insert( cl, records );
    }

    @Test
    public void testMultiplyAsMatcher() {
        try {
            String matcher = "{a:{$add:-1,$et:{'$numberLong':'-2147483649'}},"
                    + "b:{$add:-1,$et:{'$decimal':'-9223372036854775809'}},"
                    + "'c.a.b.int':{$add:{'$numberLong':'9223372036854775686'},$et:{'$decimal':'9223372036854775809'}},"
                    + "'d.1':{$add:{'$numberLong':'1'},$et:{'$numberLong':'2147483648'}},"
                    + "'e.1.1':{$add:1,$et:{'$decimal':'9223372036854775808'}}}";
            String[] expRecords = {
                    "{'a':-2147483648,'b':{'$numberLong':'-9223372036854775808'},"
                            + "c:{a:{b:{int:123,long:{'$numberLong':'1024819115206086201'}}}},"
                            + "d:[1,2147483647],e:[3,[1,{'$numberLong':'9223372036854775807'}]]}" };
            String indexKey = "{a:1,b:1,c:1}";
            NumOverflowUtils.multiFieldOperAsMatcher( cl, matcher, expRecords,
                    indexKey );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "add is used as matcher oper failed,"
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
