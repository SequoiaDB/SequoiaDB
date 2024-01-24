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
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * FileName: MultiplyIsSelector12581.java test content:set
 * StrictDataType=true,Numeric value overflow for single character using
 * $multiply operation, and the $multiply is used as a selector. testlink
 * case:seqDB-12581
 * 
 * @author luweikang
 * @Date 2017.9.12
 * @version 1.00
 */

public class MultiplyIsSelector12581 extends SdbTestBase {

    private String clName = "multiply_selector12581";
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private static DBCollection cl = null;

    @DataProvider(name = "operData")
    public Object[][] generateIntDatas() {

        return new Object[][] {
                // the parameters: subValue,selectorName
                // test int32 type numberflow
                new Object[] { new Integer( -1 ), "no" },
                // test int64 type numberflow
                new Object[] { new Long( -1 ), "tlong" },
                // test arr type numberflow
                new Object[] { new Integer( -1 ), "arr.$[0]" },
                // the arr type
                new Object[] { new Integer( -1 ), "arr" }, };
    }

    @BeforeClass
    public void setUp() {
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        } catch ( BaseException e ) {
            Assert.assertTrue( false,
                    "connect %s failed," + coordUrl + e.getMessage() );
        }

        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        cl = NumOverflowUtils.createCL( cs, clName, "{StrictDataMode:true}" );

        String[] records = {
                "{'no':-2147483648,'tlong':{'$numberLong':'-9223372036854775808'},'arr':[-2147483648,-1.7e+304]}" };

        NumOverflowUtils.insert( cl, records );
    }

    @Test(dataProvider = "operData")
    public void testMultiply( Object mulValue, String selectorName ) {
        try {
            BSONObject mValue = new BasicBSONObject();
            BSONObject selector = new BasicBSONObject();
            mValue.put( "$multiply", mulValue );
            selector.put( selectorName, mValue );
            NumOverflowUtils.isStrictDataTypeOper( cl, selector );

        } catch ( BaseException e ) {
            Assert.assertTrue( false,
                    "multiply data is used as selector oper failed,"
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
