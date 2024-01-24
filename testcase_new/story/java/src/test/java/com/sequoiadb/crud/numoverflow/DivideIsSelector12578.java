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
 * FileName: DivideIsSelector12578.java test content:set
 * StrictDataType=true,Numeric value overflow for a character using $divide
 * operation, and the $divide is used as a selector. testlink case:seqDB-12578
 * 
 * @author wuyan
 * @Date 2017.9.4
 * @version 1.00
 */

public class DivideIsSelector12578 extends SdbTestBase {
    @DataProvider(name = "operData")
    public Object[][] generateDatas() {
        return new Object[][] {
                // the parameters: selectorName
                // test int32 type numberflow
                new Object[] { "no", new Integer( -1 ) },
                // test int64 type numberflow
                new Object[] { "tlong", new Long( -1 ) },
                // test arr type numberflow
                new Object[] { "arr.$[0]", new Integer( -1 ) },
                // the arr type
                new Object[] { "arr", new Integer( -1 ) }, };
    }

    private String clName = "divide_selector12578";
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
                "{'no':-2147483648,'tlong':{'$numberLong':'-9223372036854775808'},'arr':[-2147483648,-1.7e+304]}" };
        NumOverflowUtils.insert( cl, records );
    }

    @Test(dataProvider = "operData")
    public void testSubtract( String selectorName, Object sValue ) {
        try {
            BSONObject selector = new BasicBSONObject();
            BSONObject selectorValue = new BasicBSONObject();
            selectorValue.put( "$divide", sValue );
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
            cs = sdb.getCollectionSpace( SdbTestBase.csName );
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
