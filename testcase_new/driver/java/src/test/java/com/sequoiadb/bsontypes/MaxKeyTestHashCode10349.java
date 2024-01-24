package com.sequoiadb.bsontypes;

import java.util.Date;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.MaxKey;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;

import org.testng.annotations.Test;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * FileName: MaxKeyTestHashCode10349.java* test interface: hashCode() TestLink:
 * seqDB-10349:
 * 
 * @author wuyan
 * @Date 2016.10.14
 * @version 1.00
 */
public class MaxKeyTestHashCode10349 extends SdbTestBase {
    private static Sequoiadb sdb = null;

    @BeforeClass
    public void setUp() {
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "connect %s failed,"
                    + SdbTestBase.coordUrl + e.getMessage() );
        }
    }

    @Test
    public void testHashCode() {
        try {
            BSONObject obj = new BasicBSONObject();
            MaxKey maxkey = new MaxKey();
            obj.put( "maxkey", maxkey );

            // compare yourself
            if ( maxkey.hashCode() != maxkey.hashCode() ) {
                Assert.assertTrue( false, "compare yourself fail" );
            }

            // comparison of the same value
            if ( maxkey.hashCode() != obj.get( "maxkey" ).hashCode() ) {
                Assert.assertTrue( false, "compare the same value fail" );
            }

            // comparison of the other bson type
            String expdata = "MaxKey()";
            if ( maxkey.hashCode() == expdata.hashCode() ) {
                Assert.assertTrue( false, "compare the other type fail" );
            }
        } catch ( BaseException e ) {
            Assert.assertTrue( false, e.getMessage() + e.getStackTrace() );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            sdb.disconnect();
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "clean up failed:" + e.getMessage() );
        }
    }

}
