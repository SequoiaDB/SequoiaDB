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
 * FileName: DivideIsSelector12588.java test content:Numeric value overflow for
 * many character using $divide operation, and the $divide is used as a
 * selector. testlink case:seqDB-12583
 * 
 * @author luweikang
 * @Date 2017.9.13
 * @version 1.00
 */

public class DivideIsSelector12588 extends SdbTestBase {

    private String clName = "divide_selector12588";
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
                "{'no':-2147483648,'long':{'$numberLong':'-9223372036854775808'},"
                        + "'arr':[2147483647,{'$numberLong':'9223372036854775807'},-1.7e+304],"
                        + "'obj':{a:{b:{'$numberLong':'9223372036854775807'}}}}" };

        NumOverflowUtils.insert( cl, records );
    }

    @Test
    public void testDivide() {
        try {
            String selector = "{no:{$divide:-1},long:{$divide:-1},"
                    + "'arr.$[0]':{$divide:3},'obj.a.b':{$divide:123},_id:{$include:0}}";

            String[] expRecords = {
                    "{'no':2147483648,'long' : { '$decimal' : '9223372036854775808'},"
                            + "'arr':[715827882],'obj':{a:{b:{'$numberLong':'74986764527274600'}}}}" };

            NumOverflowUtils.multipleFieldOper( cl, selector, expRecords );
        } catch ( BaseException e ) {
            Assert.assertTrue( false,
                    "divide data is used as selector oper failed,"
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
