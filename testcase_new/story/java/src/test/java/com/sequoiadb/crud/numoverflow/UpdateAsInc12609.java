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
 * FileName: UpdateAsInc12609.java test content:set StrictDataType=true,Numeric
 * value overflow for a character using $inc operation, and $inc value is same
 * data type. testlink case:seqDB-12609
 * 
 * @author wuyan
 * @Date 2017.9.14
 * @version 1.00
 */

public class UpdateAsInc12609 extends SdbTestBase {
    @DataProvider(name = "operData")
    public Object[][] generateDatas() {
        return new Object[][] {
                // the parameters: updateFieldName, incValue
                // int32 + int32:'no':-2147483648---> -2147483649L
                new Object[] { "no", new Integer( -1 ) },
                // int64 +
                // int64:'tlong':{'$numberLong':'-9223372036854775808'}--->decimal
                new Object[] { "tlong", new Long( -1 ) },
                // int32(arr) + int32
                // TODO:SEQUOIADBMAINSTREAM-2825
                // new Object[]{"arr.0", new Integer(1)},
                // new Object[]{"arr.1.1.1", new Integer(48)},
                // int64(obj) + int64
                // new Object[]{"tlong.a.b.c", new Long(809)},
        };
    }

    private String clName = "inc_update12609";
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
                "{'no':-2147483648,'tlong':{'$numberLong':'-9223372036854775808'},'arr':[2147483647,-1.7e+304]}",
                "{'no':1,'tlong':{a:{b:{c:{'$numberLong':'9223372036854775000'}}}},'arr':[1,[2,[3,2147483600],4]]}" };
        NumOverflowUtils.insert( cl, records );
    }

    @Test(dataProvider = "operData")
    public void testInc( String updateName, Object sValue ) {
        try {
            BSONObject incValue = new BasicBSONObject();
            incValue.put( updateName, sValue );
            NumOverflowUtils.updateIsStrictDataType( cl, incValue );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "update user StrictDataMode failed,"
                    + e.getErrorCode() + e.getMessage() );
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
