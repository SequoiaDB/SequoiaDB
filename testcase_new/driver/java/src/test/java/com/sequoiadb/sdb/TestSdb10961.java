package com.sequoiadb.sdb;

import java.util.Date;

import org.testng.Assert;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

public class TestSdb10961 extends SdbTestBase {
    private Sequoiadb sdb;

    @BeforeClass
    public void setUp() {
        try {
            this.sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

        } catch ( BaseException e ) {
            Assert.fail(
                    "Sequoiadb driver TestSdb10961 setUp error, error description:"
                            + e.getMessage() );
        }
    }

    /**
     * sdb disconnect then closeAllCursor
     */
    @Test
    public void test() {
        try {
            this.sdb.disconnect();
            this.sdb.closeAllCursors();
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
    }
}
