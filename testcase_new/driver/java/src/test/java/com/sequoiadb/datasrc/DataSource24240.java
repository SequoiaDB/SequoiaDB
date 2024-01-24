package com.sequoiadb.datasrc;

import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description seqDB-24240:数据源接口异常校验
 * @author liuli
 * @Date 2021.05.31
 * @version 1.10
 */

public class DataSource24240 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private Sequoiadb srcdb = null;
    private String dataSrcName = "datasrc_24240";
    private String dataSrcIp = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        if ( sdb.isDataSourceExist( dataSrcName ) ) {
            sdb.dropDataSource( dataSrcName );
        }
    }

    @Test
    public void test() throws Exception {
        dataSrcIp = SdbTestBase.dsHostName + ":" + SdbTestBase.dsServiceName;
        try {
            sdb.createDataSource( null, dataSrcIp, "sdbadmin", "sdbadmin", "",
                    new BasicBSONObject() );
            Assert.fail( "Expected createtion failed" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_INVALIDARG.getErrorCode() ) {
                throw e;
            }
        }

        try {
            sdb.createDataSource( dataSrcName, null, "sdbadmin", "sdbadmin", "",
                    new BasicBSONObject() );
            Assert.fail( "Expected createtion failed" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_INVALIDARG.getErrorCode() ) {
                throw e;
            }
        }

        try {
            sdb.createDataSource( "", dataSrcIp, "sdbadmin", "sdbadmin", "",
                    new BasicBSONObject() );
            Assert.fail( "Expected createtion failed" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_INVALIDARG.getErrorCode() ) {
                throw e;
            }
        }

        try {
            sdb.createDataSource( dataSrcName, "", "sdbadmin", "sdbadmin", "",
                    new BasicBSONObject() );
            Assert.fail( "Expected createtion failed" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_INVALIDARG.getErrorCode() ) {
                throw e;
            }
        }

        try {
            sdb.getDataSource( null );
            Assert.fail( "Expected createtion failed" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_INVALIDARG.getErrorCode() ) {
                throw e;
            }
        }

        try {
            sdb.getDataSource( "" );
            Assert.fail( "Expected createtion failed" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_INVALIDARG.getErrorCode() ) {
                throw e;
            }
        }

        try {
            sdb.isDataSourceExist( null );
            Assert.fail( "Expected createtion failed" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_INVALIDARG.getErrorCode() ) {
                throw e;
            }
        }

        try {
            sdb.isDataSourceExist( "" );
            Assert.fail( "Expected createtion failed" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_INVALIDARG.getErrorCode() ) {
                throw e;
            }
        }

        try {
            sdb.dropDataSource( null );
            Assert.fail( "Expected createtion failed" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_INVALIDARG.getErrorCode() ) {
                throw e;
            }
        }

        try {
            sdb.dropDataSource( "" );
            Assert.fail( "Expected createtion failed" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_INVALIDARG.getErrorCode() ) {
                throw e;
            }
        }
    }

    @AfterClass
    public void tearDown() {

        if ( sdb != null ) {
            sdb.close();
        }
        if ( srcdb != null ) {
            srcdb.close();
        }
    }
}
