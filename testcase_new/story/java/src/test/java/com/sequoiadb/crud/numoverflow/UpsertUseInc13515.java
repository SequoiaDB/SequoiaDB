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
 * FileName: UpsertUseInc13515.java test content:set StrictDataType=true,Numeric
 * value overflow for a character using $inc operation, and $inc value is
 * different data type. testlink case:seqDB-13515
 * 
 * @author luweikang
 * @Date 2017.11.17
 * @version 1.00
 */

public class UpsertUseInc13515 extends SdbTestBase {

    @DataProvider(name = "operData")
    public Object[][] generateDatas() {

        return new Object[][] {

                new Object[] { "no", new Long( -9223372036854775807L ),
                        new Integer( -10 ) },

                new Object[] { "no", new Long( 9223372036854775807L ),
                        new Integer( 10 ) },

                new Object[] { "no.a", new Integer( -2147483648 ),
                        new Integer( -10 ) },

                new Object[] { "no", new Integer( -2147483648 ),
                        new Integer( -10 ) },

                new Object[] { "no.1", new Integer( 2147483647 ),
                        new Integer( 10 ) },

                new Object[] { "no.a.b.c", new Integer( 2147483647 ),
                        new Integer( 10 ) },

        };
    }

    private String clName = "inc_update13515";
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

        String[] records = { "{no:-2147483648}"
                + "{no:{'$numberLong':'-9223372036854775808'}}" };
        NumOverflowUtils.insert( cl, records );
    }

    @Test(dataProvider = "operData")
    public void testInc( String upsertName, Object sValue, Object mValue ) {
        try {
            BSONObject incValue = new BasicBSONObject();
            incValue.put( upsertName, sValue );
            BSONObject matherValue = new BasicBSONObject();
            matherValue.put( upsertName, mValue );
            NumOverflowUtils.upsertIsStrictDataType( cl, incValue,
                    matherValue );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "upsert user StrictDataMode failed,"
                    + e.getErrorCode() + e.getMessage() );
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
