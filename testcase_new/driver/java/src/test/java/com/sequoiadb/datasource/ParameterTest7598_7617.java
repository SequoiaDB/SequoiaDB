package com.sequoiadb.datasource;

import com.sequoiadb.exception.SDBError;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.sequoiadb.exception.BaseException;

public class ParameterTest7598_7617 extends DataSourceTestBase {
    private SequoiadbDatasource datasource = null;

    @BeforeClass
    void createDataSource() {
        try {
            super.init();
            if ( datasource == null ) {
                datasource = new SequoiadbDatasource( this.coordAddr,
                        this.userName, this.password, null );
            }
        } catch ( BaseException e ) {
            Assert.assertFalse( true, e.getMessage() );
        }
    }

    @AfterClass
    void closeDataSource() {
        try {
            if ( null != datasource ) {
                datasource.close();
            }
        } catch ( BaseException e ) {
            Assert.assertFalse( true, e.getMessage() );
        }
    }

    @Test
    public void test() throws Exception {
        DatasourceOptions options = new DatasourceOptions();
        try {
            // deltaIncCount < 0
            options.setDeltaIncCount( -1 );
            Assert.fail( "unexpect result" );
        } catch ( BaseException e ) {
            Assert.assertEquals( SDBError.SDB_INVALIDARG.getErrorCode(),
                    e.getErrorCode() );
        }
        try {
            // deltaIncCount = 0
            options.setDeltaIncCount( 0 );
            Assert.fail( "unexpect result" );
        } catch ( BaseException e ) {
            Assert.assertEquals( SDBError.SDB_INVALIDARG.getErrorCode(),
                    e.getErrorCode() );
        }
        // deltaIncCount > maxCount
        options.setDeltaIncCount( 600 );

        try {
            // maxIdleCount < 0
            options.setMaxIdleCount( -1 );
            Assert.fail( "unexpect result" );
        } catch ( BaseException e ) {
            Assert.assertEquals( SDBError.SDB_INVALIDARG.getErrorCode(),
                    e.getErrorCode() );
        }
        // maxIdleCount = 0
        options.setMaxIdleCount( 0 );
        // maxIdleCount > maxCount
        options.setMaxIdleCount( 600 );

        try {
            // maxCount < 0
            options.setMaxCount( -1 );
            Assert.fail( "unexpect result" );
        } catch ( BaseException e ) {
            Assert.assertEquals( SDBError.SDB_INVALIDARG.getErrorCode(),
                    e.getErrorCode() );
        }
        // maxCount = 0
        options.setMaxCount( 0 );

        try {
            // keepAliveTimeout < 0
            options.setKeepAliveTimeout( -1 );
            Assert.fail( "unexpect result" );
        } catch ( BaseException e ) {
            Assert.assertEquals( SDBError.SDB_INVALIDARG.getErrorCode(),
                    e.getErrorCode() );
        }
        // keepAliveTimeout = 0
        options.setKeepAliveTimeout( 0 );

        try {
            // syncCoordInterval < 0
            options.setSyncCoordInterval( -1 );
            Assert.fail( "unexpect result" );
        } catch ( BaseException e ) {
            Assert.assertEquals( SDBError.SDB_INVALIDARG.getErrorCode(),
                    e.getErrorCode() );
        }
        // syncCoordInterval = 0
        options.setSyncCoordInterval( 0 );
    }

    @Test
    void keepAliveTest7598() {
        try {
            // 小于checkInteval
            DatasourceOptions option = new DatasourceOptions();
            option.setKeepAliveTimeout( 5000 );
            datasource.updateDatasourceOptions( option );
            Assert.fail( "must throw exception" );
        } catch ( BaseException e ) {
            super.judegeErrCode( "SDB_INVALIDARG", e.getErrorCode() );
        }

        try {
            DatasourceOptions option = new DatasourceOptions();
            option.setKeepAliveTimeout( -100 );
            datasource.updateDatasourceOptions( option );
            Assert.fail( "must throw exception" );
        } catch ( BaseException e ) {
            super.judegeErrCode( "SDB_INVALIDARG", e.getErrorCode() );
        }
    }

    @Test
    void coordUrlTest7607() {
        try {
            SequoiadbDatasource datasource = new SequoiadbDatasource( null,
                    this.userName, this.password, null );
            Assert.fail( "must throw exception" );
            datasource.close();
        } catch ( BaseException e ) {
            super.judegeErrCode( "SDB_INVALIDARG", e.getErrorCode() );
        }
    }

    @DataProvider(name = "val-provider")
    public Object[][] optionVal() {
        int negative = -1;
        int minVal = 0;
        int maxVal = 500;
        return new Object[][] { { negative }, { minVal }, { maxVal }, };
    }

    @Test
    void addCoordTest7616() {
        datasource.addCoord( this.coordAddr );

        try {
            datasource.addCoord( "" );
            Assert.fail( "must throw exception" );
        } catch ( BaseException e ) {
            super.judegeErrCode( "SDB_INVALIDARG", e.getErrorCode() );
        }

        try {
            datasource.addCoord( null );
            Assert.fail( "must throw exception" );
        } catch ( BaseException e ) {
            super.judegeErrCode( "SDB_INVALIDARG", e.getErrorCode() );
        }
    }

    @Test
    void delCoordTest7617() {
        datasource.removeCoord( this.coordAddr );

        try {
            datasource.removeCoord( "" );
            Assert.fail( "must throw exception" );
        } catch ( BaseException e ) {
            super.judegeErrCode( "SDB_INVALIDARG", e.getErrorCode() );
        }

        try {
            datasource.removeCoord( null );
            Assert.fail( "must throw exception" );
        } catch ( BaseException e ) {
            super.judegeErrCode( "SDB_INVALIDARG", e.getErrorCode() );
        }
    }
}