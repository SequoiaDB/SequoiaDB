package com.sequoiadb.crud.basicoperation;

import java.util.Random;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @TestLink: seqDB-11070
 * @describe: 副本数为0，带_id条件并发Upsert
 * @author wangkexin
 * @Date 2019.02.22
 * @version 1.00
 */
public class TestUpsert11070 extends SdbTestBase {
    private String clName = "cl11070";
    private Sequoiadb sdb = null;

    @DataProvider(name = "upsertData", parallel = true)
    public Object[][] generateIntDatas() {
        int _id = 11070;

        return new Object[][] { new Object[] { _id, getRandomString( 5455 ) },
                new Object[] { _id, getRandomString( 5471 ) },
                new Object[] { _id, getRandomString( 5500 ) },
                new Object[] { _id, getRandomString( 6000 ) },
                new Object[] { _id, getRandomString( 7500 ) }, };
    }

    @BeforeClass
    public void setup() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        BSONObject options = new BasicBSONObject( "ReplSize", 0 );
        sdb.getCollectionSpace( SdbTestBase.csName ).createCollection( clName,
                options );
    }

    @Test(dataProvider = "upsertData")
    public void test( int id, String value ) {
        try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" )) {
            DBCollection cl = db.getCollectionSpace( SdbTestBase.csName )
                    .getCollection( clName );
            BSONObject matcher = new BasicBSONObject();
            BSONObject modifer = new BasicBSONObject();
            BSONObject data = new BasicBSONObject();

            matcher.put( "_id", id );
            data.put( "a", value );
            modifer.put( "$set", data );

            cl.upsert( matcher, modifer, null );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -38 ) {
                e.printStackTrace();
                Assert.fail( "insert failed", e );
            }
        }
    }

    @AfterClass
    public void tearDown() {
        sdb.getCollectionSpace( csName ).dropCollection( clName );
        sdb.close();
    }

    private String getRandomString( int length ) {
        String str = "zxcvbnmlkjhgfdsaqwertyuiopQWERTYUIOPLKJHGFDSAZXCVBNM1234567890";
        // 由Random生成随机数
        Random random = new Random();
        StringBuffer sb = new StringBuffer();
        for ( int i = 0; i < length; ++i ) {
            int number = random.nextInt( 62 );
            sb.append( str.charAt( number ) );
        }
        return sb.toString();
    }
}