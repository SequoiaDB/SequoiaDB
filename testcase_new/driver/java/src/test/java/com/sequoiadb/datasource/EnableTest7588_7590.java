package com.sequoiadb.datasource;

import org.testng.Assert;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;

public class EnableTest7588_7590 extends DataSourceTestBase {

    @BeforeClass
    public void initEnv() {
        boolean retVal = super.init();
        Assert.assertTrue( retVal );
    }

    /**
     * 启用创建的连接池
     */
    @Test
    public void EnableAfterCreated7588() {
        SequoiadbDatasource datasource = null;
        try {
            datasource = new SequoiadbDatasource( this.coordAddr, userName,
                    password, null );
            datasource.enableDatasource();
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            datasource.close();
        }
    }

    /**
     * 启用禁用的连接池
     */
    @Test
    public void EnableAfterDisabled7589() {
        SequoiadbDatasource datasource = null;
        try {
            datasource = new SequoiadbDatasource( this.coordAddr, userName,
                    password, null );
            Sequoiadb sdb;
            sdb = datasource.getConnection();
            datasource.disableDatasource();
            datasource.enableDatasource();
            Assert.assertEquals( datasource.getUsedConnNum(), 1 );
            Sequoiadb sdb1 = datasource.getConnection();
            Assert.assertEquals( datasource.getUsedConnNum(), 2 );
            Thread.sleep( 10 );
            // Assert.assertTrue(datasource.getIdleConnNum() >= 0);
            datasource.releaseConnection( sdb );
            Assert.assertEquals( datasource.getUsedConnNum(), 1 );
            datasource.releaseConnection( sdb1 );
            Assert.assertEquals( datasource.getUsedConnNum(), 0 );

        } catch ( InterruptedException e ) {
            Assert.fail( e.getMessage() );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            datasource.close();
        }
    }

    @Test
    public void EnableAfterDisabledByOption7590() {
        SequoiadbDatasource datasource = null;
        try {
            DatasourceOptions option = new DatasourceOptions();
            option.setMaxCount( 0 );
            datasource = new SequoiadbDatasource( this.coordAddr, userName,
                    password, option );
            datasource.enableDatasource();

            Sequoiadb sdb = datasource.getConnection();
            Assert.assertEquals( sdb.isValid(), true );
        } catch ( InterruptedException e ) {
            Assert.fail( e.getMessage() );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            datasource.close();
        }
    }

    /**
     * 启用关闭的连接池
     */
    @Test
    public void EnableAfterClosed7590() {
        SequoiadbDatasource datasource = null;
        try {
            datasource = new SequoiadbDatasource( this.coordAddr, userName,
                    password, null );
            datasource.close();
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }

        try {
            datasource.enableDatasource();
            Assert.fail("must throw exception!");
        } catch ( BaseException e ) {
            judegeErrCode( "SDB_CLIENT_CONNPOOL_CLOSE", e.getErrorCode() );
        }
    }
}
