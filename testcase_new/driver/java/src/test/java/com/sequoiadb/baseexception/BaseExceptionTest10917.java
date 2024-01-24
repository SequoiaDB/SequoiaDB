package com.sequoiadb.baseexception;

import java.util.Date;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @FileName: BaseExceptionTest10917
 * @description: Test Java BaseException
 * @Interface: BaseException(SDBError error, Exception e, String detail)
 *             BaseException(SDBError error, String detail)
 *             BaseException(SDBError error) BaseException(int errCode)
 *             BaseException(int errCode, String detail)
 * @author zhaoyu
 * @Date 2017-01-03
 * @version 1.00
 */

public class BaseExceptionTest10917 extends SdbTestBase {
    private Sequoiadb sdb;

    @BeforeClass
    public void setUp() {
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        } catch ( BaseException e ) {
            Assert.fail( "prepare env failed" + e.getMessage() );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            sdb.disconnect();
        } catch ( BaseException e ) {
            Assert.fail( "clear env failed" + e.getMessage() );
        }
    }

    @Test
    public void test1() {
        try {
            sdb.getCollectionSpace( "a" );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(),
                    SDBError.SDB_DMS_CS_NOTEXIST.getErrorCode() );
        }
        Exception e = new Exception( "e" );
        String detail = "this is a test for exception";
        BaseException err = new BaseException( SDBError.SDB_APP_DISCONNECT,
                detail, e );
        Assert.assertEquals( err.getErrorCode(),
                SDBError.SDB_APP_DISCONNECT.getErrorCode() );
        Assert.assertEquals( err.getErrorType(),
                SDBError.SDB_APP_DISCONNECT.getErrorType() );
        Assert.assertEquals( err.getMessage(),
                "SDB_APP_DISCONNECT(-117): Application is disconnected, detail: this is a test for exception" );

    }

    @Test
    public void test2() {
        try {
            sdb.getCollectionSpace( "a" );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(),
                    SDBError.SDB_DMS_CS_NOTEXIST.getErrorCode() );
        }
        Exception e = new Exception( "e" );
        BaseException err = new BaseException( SDBError.SDB_APP_DISCONNECT, e );
        Assert.assertEquals( err.getErrorCode(),
                SDBError.SDB_APP_DISCONNECT.getErrorCode() );
        Assert.assertEquals( err.getErrorType(),
                SDBError.SDB_APP_DISCONNECT.getErrorType() );
        Assert.assertEquals( err.getMessage(),
                "SDB_APP_DISCONNECT(-117): Application is disconnected, detail: e" );
    }

    @Test
    public void test3() {
        try {
            sdb.getCollectionSpace( "a" );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(),
                    SDBError.SDB_DMS_CS_NOTEXIST.getErrorCode() );
        }
        String detail = "this is a test for exception";
        BaseException err = new BaseException( SDBError.SDB_APP_DISCONNECT,
                detail );
        Assert.assertEquals( err.getErrorCode(),
                SDBError.SDB_APP_DISCONNECT.getErrorCode() );
        Assert.assertEquals( err.getErrorType(),
                SDBError.SDB_APP_DISCONNECT.getErrorType() );
        Assert.assertEquals( err.getMessage(),
                "SDB_APP_DISCONNECT(-117): Application is disconnected, detail: this is a test for exception" );

    }

    @Test
    public void test4() {
        try {
            sdb.getCollectionSpace( "a" );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(),
                    SDBError.SDB_DMS_CS_NOTEXIST.getErrorCode() );
        }
        BaseException err = new BaseException( SDBError.SDB_APP_DISCONNECT );
        Assert.assertEquals( err.getErrorCode(),
                SDBError.SDB_APP_DISCONNECT.getErrorCode() );
        Assert.assertEquals( err.getErrorType(),
                SDBError.SDB_APP_DISCONNECT.getErrorType() );
        Assert.assertEquals( err.getMessage(),
                "SDB_APP_DISCONNECT(-117): Application is disconnected" );
    }

    @Test
    public void test5() {
        try {
            sdb.getCollectionSpace( "a" );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(),
                    SDBError.SDB_DMS_CS_NOTEXIST.getErrorCode() );
        }
        int errCode = -117;
        BaseException err = new BaseException( errCode );
        Assert.assertEquals( err.getErrorCode(),
                SDBError.SDB_APP_DISCONNECT.getErrorCode() );
        Assert.assertEquals( err.getErrorType(),
                SDBError.SDB_APP_DISCONNECT.getErrorType() );
        Assert.assertEquals( err.getMessage(),
                "SDB_APP_DISCONNECT(-117): Application is disconnected" );
    }

    @Test
    public void test6() {
        try {
            sdb.getCollectionSpace( "a" );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(),
                    SDBError.SDB_DMS_CS_NOTEXIST.getErrorCode() );
        }
        String detail = "this is a test for exception";
        int errCode = -117;
        BaseException err = new BaseException( errCode, detail );
        Assert.assertEquals( err.getErrorCode(), errCode );
        Assert.assertEquals( err.getErrorType(),
                SDBError.SDB_APP_DISCONNECT.getErrorType() );
        Assert.assertEquals( err.getMessage(),
                "SDB_APP_DISCONNECT(-117): Application is disconnected, detail: this is a test for exception" );
    }

    @Test
    public void test7() {
        try {
            sdb.getCollectionSpace( "a" );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(),
                    SDBError.SDB_DMS_CS_NOTEXIST.getErrorCode() );
        }
        String detail = "this is a test for exception";
        int errCode = -1000;
        BaseException err = new BaseException( errCode, detail );

    }
}
