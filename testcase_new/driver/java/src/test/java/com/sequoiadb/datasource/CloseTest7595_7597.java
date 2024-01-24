package com.sequoiadb.datasource;

import org.testng.Assert;
import org.testng.annotations.AfterMethod;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.BeforeMethod;

import org.testng.annotations.Test;

import com.sequoiadb.base.Sequoiadb;

import com.sequoiadb.exception.BaseException;

public class CloseTest7595_7597 extends DataSourceTestBase {
    private SequoiadbDatasource datasource = null;
    // private SequoiadbDatasource ds1 = null;

    @BeforeClass
    public void initEnv() {
        boolean retVal = super.init();
        Assert.assertTrue( retVal );
        try {
            getAddrList();
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.assertFalse( true, e.getMessage() );
        }
    }

    @BeforeMethod
    public void createDataSource() {
        try {
            if ( datasource == null ) {
                datasource = new SequoiadbDatasource( this.coordAddr,
                        this.userName, this.password, null );
            }
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        }
    }

    @AfterMethod
    public void closeDataSource() {
        try {
            if ( null != datasource ) {
                datasource.close();
                datasource = null;
            }
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        }
    }

    /**
     * 关闭连接池
     */
    @Test
    public void closeAfterGetConn7595() {
        Sequoiadb sdb = null;
        try {
            sdb = datasource.getConnection();
            datasource.close();
            Assert.assertEquals( sdb.isValid(), false );
            Assert.assertEquals( datasource.getIdleConnNum(), 0 );
            Assert.assertEquals( datasource.getUsedConnNum(), 0 );
        } catch ( InterruptedException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }

        try {
            if ( null != datasource ) {
                datasource.getConnection();
                Assert.fail( "must throw exception!" );
            }
        } catch ( InterruptedException e ) {
            Assert.fail( e.getMessage() );
        } catch ( BaseException e ) {
            judegeErrCode( "SDB_CLIENT_CONNPOOL_CLOSE", e.getErrorCode() );
        }

    }

    /**
     * 关闭禁用的连接池
     */
    @Test
    public void closeAfterDisabled7597() {
        Sequoiadb sdb = null;
        Sequoiadb sdb1 = null;
        try {
            sdb1 = datasource.getConnection();
            datasource.disableDatasource();
            sdb = datasource.getConnection();
            datasource.close();
            Assert.assertEquals( sdb1.isValid(), false );
            Assert.assertEquals( sdb.isValid(), true );
        } catch ( InterruptedException e ) {
            Assert.fail( e.getMessage() );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }

        try {
            datasource.getConnection();
            Assert.fail( "must throw exception!" );
        } catch ( InterruptedException e ) {
            Assert.fail( e.getMessage() );
        } catch ( BaseException e ) {
            judegeErrCode( "SDB_CLIENT_CONNPOOL_CLOSE", e.getErrorCode() );
        }

        try {
            datasource.releaseConnection( sdb );
            Assert.fail( "must throw exception!" );
        } catch ( BaseException e ) {
            judegeErrCode( "SDB_CLIENT_CONNPOOL_CLOSE", e.getErrorCode() );
        }
    }

    /**
     * 重复关闭连接池
     */
    @Test
    public void closeAfterClose7597() {
        try {
            datasource.close();
            datasource.close();
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        }
    }

}
