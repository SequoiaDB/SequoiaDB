package com.sequoiadb.bsontypes;

import java.util.Date;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.MinKey;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;

import org.testng.annotations.Test;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * FileName: MinKeyTestHashCode10352.java* test interface: hashCode() TestLink:
 * seqDB-10352:
 * 
 * @author wuyan
 * @Date 2016.10.14
 * @version 1.00
 */
public class MinKeyTestHashCode10352 extends SdbTestBase {
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
            MinKey minkey = new MinKey();
            obj.put( "minkey", minkey );

            // compare yourself
            if ( minkey.hashCode() != minkey.hashCode() ) {
                Assert.assertTrue( false, "compare yourself fail" );
            }

            // comparison of the same value
            if ( minkey.hashCode() != obj.get( "minkey" ).hashCode() ) {
                Assert.assertTrue( false, "compare the same value fail" );
            }

            // comparison of the other bson type
            String expdata = "MinKey()";
            if ( minkey.hashCode() == expdata.hashCode() ) {
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
