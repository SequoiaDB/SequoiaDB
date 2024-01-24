package com.sequoiadb.crud.numoverflow;

import java.util.Date;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.crud.numoverflow.NumOverflowUtils;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * FileName: AddIsSelector12584.java test content:set
 * StrictDataType=true,Numeric value overflow for a character using $Add
 * operation, and the $Add is used as a selector. testlink case:seqDB-12584
 * 
 * @author wuyan
 * @Date 2017.9.4
 * @version 1.00
 */

public class AddIsSelector12584 extends SdbTestBase {
    @DataProvider(name = "operData")
    public Object[][] generateDatas() {
        return new Object[][] {
                // the parameters: selectorName
                // test int32 type numberflow(boundary value)
                new Object[] { "no", new Integer( 1 ) },
                new Object[] { "test", new Integer( 2147483600 ) },
                // test int64 type numberflow(boundary value)
                new Object[] { "tlong", new Long( 1 ) },
                // test arr type numberflow
                new Object[] { "arr.$[0]", new Integer( -1 ) },
                // the arr type
                new Object[] { "arr", new Long( 1 ) }, };
    }

    private String clName = "add_selector12584";
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

        String clOption = "{StrictDataMode:true}";
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        cl = NumOverflowUtils.createCL( cs, clName, clOption );

        String[] records = {
                "{'no':2147483647,'tlong':{'$numberLong':'9223372036854775807'},"
                        + "'arr':[-2147483648,{'$numberLong':'9223372036854775807'}],test:1234,"
                        + "testlong:{'$numberLong':'1223372036854775807'}}" };
        NumOverflowUtils.insert( cl, records );
    }

    @Test(dataProvider = "operData")
    public void testSubtract( String selectorName, Object sValue ) {
        try {
            BSONObject selector = new BasicBSONObject();
            BSONObject selectorValue = new BasicBSONObject();
            selectorValue.put( "$add", sValue );
            selector.put( selectorName, selectorValue );
            NumOverflowUtils.isStrictDataTypeOper( cl, selector );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "divide is used as selector oper failed,"
                    + e.getMessage() );
        }
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
