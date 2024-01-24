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
 * FileName: SubtractIsMatcher12598.java test content:different types of
 * numerical operations, the result of the operation is converted to a
 * high-level type testlink case:seqDB-12598
 * 
 * @author wuyan
 * @Date 2017.9.4
 * @version 1.00
 */

public class SubtractIsMatcher12598 extends SdbTestBase {

    private String clName = "subtract_matcher12598";
    private CollectionSpace cs = null;
    private Sequoiadb sdb = null;
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
                "{'a':-214748364,long:{'$numberLong':'-1024819115206086201'},"
                        + "arr:[{'$numberLong':'-9223372036854775808'},3],obj:{a:{'$numberLong':'922337203685'}},"
                        + "mobj:{arr:[1,3,2147483647]},b:1}",
                "{a:1,long:123456,arr:[{'$numberLong':'9223372036854775800'},3],"
                        + "obj:{a:{'$numberLong':'9223372036854775807'}},mobj:123}" };

        NumOverflowUtils.insert( cl, records );
    }

    @Test
    public void testSubtractAsMatcher() {
        try {
            // int32 - int64
            String matcher = "{a:{$subtract:{'$numberLong':'-1'},$et:{'$numberLong':'-214748363'}},"
                    // int32(array) - decimal
                    + "'arr.0':{$subtract:-1,$et:{'$decimal':'-9223372036854775807'}},"
                    // int64(obj) - double
                    + "'obj.a':{$subtract:2.01,$et:922337203682.99},"
                    // int32(obj.arr) - int64
                    + "'mobj.arr.2':{$subtract:{'$numberLong':'647'},$et:{'$numberLong':'2147483000'}}}";
            String[] expRecords = {
                    "{'a':-214748364,long:{'$numberLong':'-1024819115206086201'},"
                            + "arr:[{'$numberLong':'-9223372036854775808'},3],obj:{a:{'$numberLong':'922337203685'}},"
                            + "mobj:{arr:[1,3,2147483647]},b:1}" };
            String indexKey = "{a:1,long:-1,arr:1}";
            NumOverflowUtils.multiFieldOperAsMatcher( cl, matcher, expRecords,
                    indexKey );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "subtract is used as matcher oper failed,"
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
