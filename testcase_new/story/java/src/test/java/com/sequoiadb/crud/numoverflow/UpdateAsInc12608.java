package com.sequoiadb.crud.numoverflow;

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
 * FileName: UpdateAsInc12608.java test content:set StrictDataType=true,Numeric
 * value overflow for a character using $inc operation, and $inc value is
 * different data type. testlink case:seqDB-12608
 * 
 * @author wuyan
 * @Date 2017.9.14
 * @version 1.00
 */

public class UpdateAsInc12608 extends SdbTestBase {
    @DataProvider(name = "operData")
    public Object[][] generateDatas() {
        return new Object[][] {
                // the parameters: updateFieldName, incValue
                // int32 + int64:'no':-2147483648---> decimal
                new Object[] { "no", new Long( -9223372034707292161L ) },
                // int64 +
                // int32:'tlong':{'$numberLong':'-9223372036854775808'}--->decimal
                new Object[] { "tlong", new Integer( -1 ) },
                // int32(arr) + int64
                // TODO:SEQUOIADBMAINSTREAM-2825
                new Object[] { "arr.0", new Long( 9223372034707292161L ) },
                new Object[] { "arr.1.1.1", new Long( 9223372034707292208L ) },
                // int64(obj) + int32
                new Object[] { "tlong.a.b.c", new Integer( 808 ) }, };
    }

    private String clName = "inc_update12608";
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
