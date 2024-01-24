package com.sequoiadb.auth;

import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @FileName:TestSdbUser16280 Connecting sequoiadb with incorrect password
 * @author wangkexin
 * @Date 2018-10-22
 * @version 1.00
 */

public class TestSdbUser16280 extends SdbTestBase {
    private Sequoiadb sdb;
    private String coordAddr;
    private String userName = "admin16280";

    @BeforeClass
    public void setUp() {
        this.coordAddr = SdbTestBase.coordUrl;
        sdb = new Sequoiadb( this.coordAddr, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "run mode is standalone,test case skip" );
        }
    }

    @Test
    public void test() {
        try {
            sdb.createUser( userName, "admin" );
            Sequoiadb sdb = new Sequoiadb( coordAddr, userName, "" );
            Assert.fail( "exp fail but act success" );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -179 );
        }
    }

    @AfterClass(alwaysRun = true)
    public void tearDown() {
        sdb.removeUser( userName, "admin" );
        sdb.close();
    }
}
