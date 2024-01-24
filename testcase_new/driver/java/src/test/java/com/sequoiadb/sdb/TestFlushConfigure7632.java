package com.sequoiadb.sdb;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

public class TestFlushConfigure7632 extends SdbTestBase {

    private Sequoiadb sdb;

    @BeforeClass
    public void setUp() {
        try {
            this.sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
    }

    @Test
    public void testFlushConfigure() {
        try {
            this.sdb.flushConfigure(
                    ( BSONObject ) JSON.parse( "{Global:true}" ) );
        } catch ( BaseException e ) {
            Assert.fail(
                    "Sequoiadb driver TestFlushConfigure7632 testFlushConfigure error, error description:"
                            + e.getMessage() );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            this.sdb.disconnect();
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
    }
}
