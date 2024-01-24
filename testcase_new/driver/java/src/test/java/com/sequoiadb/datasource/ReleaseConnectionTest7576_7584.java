package com.sequoiadb.datasource;

import org.testng.Assert;
import org.testng.annotations.AfterMethod;
import org.testng.annotations.BeforeMethod;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;

public class ReleaseConnectionTest7576_7584 extends DataSourceTestBase {
    private SequoiadbDatasource datasource;

    @BeforeMethod
    public void createDatasource() {
        boolean retVal = super.init();
        Assert.assertTrue( retVal );
        try {
            if ( datasource == null ) {
                datasource = new SequoiadbDatasource( this.coordAddr, userName,
                        password, null );
            }
        } catch ( BaseException e ) {
            Assert.assertFalse( true, e.getMessage() );
        }
    }

    @AfterMethod
    public void CloseDatasource() {
        try {
            if ( datasource != null ) {
                datasource.close();
                datasource = null;
            }
        } catch ( BaseException e ) {
            Assert.assertFalse( true, e.getMessage() );
        }
    }

    /**
     * 归还不属于连接池的连接
     */
    @Test
    public void releaseNotBelong7576() {
        int priorNum = 0;
        Sequoiadb sdb = null;
        try {
            sdb = new Sequoiadb( this.coordAddr, userName, password );
            priorNum = datasource.getIdleConnNum();
            datasource.releaseConnection( sdb );
            Assert.fail( "must throw exception!" );
        } catch ( BaseException e ) {
            super.judegeErrCode( "SDB_INVALIDARG", -6 );

        }

        try {
            int laterNum = datasource.getIdleConnNum();
            Thread.sleep( 10 );
            Assert.assertEquals( laterNum, priorNum );
            Assert.assertEquals( sdb.isValid(), true );
        } catch ( InterruptedException e ) {
            Assert.fail( e.getMessage() );
        }
    }

    /**
     * 归还使用且超出空闲时间的连接
     */
    @Test
    public void releaseUsedConn7577() {
        try {
            DatasourceOptions option = new DatasourceOptions();
            option.setKeepAliveTimeout( 50 );// ms
            option.setCheckInterval( 20 );// ms
            datasource.updateDatasourceOptions( option );
            Sequoiadb sdb = datasource.getConnection();

            // 使用sdb超过空闲时间keepAliveTimeout
            for ( int i = 0; i < 6; i++ ) {
                Thread.sleep( 10 );
                sdb.listCollections();
            }
            /*
             * // 异步扩展连接池，让一次扩展操作完成 do{ if (datasource.getIdleConnNum() !=
             * option.getDeltaIncCount()){ Thread.sleep(1); }else break;
             * }while(true);
             */
            int priorNum = datasource.getIdleConnNum();
            datasource.releaseConnection( sdb );
            int laterNum = datasource.getIdleConnNum();

            // Assert.assertEquals(laterNum, priorNum+1);
            Thread.sleep( 10 );
            // Assert.assertEquals(sdb.isValid(), false);
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } catch ( InterruptedException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        }
    }

    /**
     * 归还闲置且超出空闲时间的连接
     */
    @Test
    public void releaseUnusedConn7578() {
        try {
            DatasourceOptions option = new DatasourceOptions();
            option.setKeepAliveTimeout( 50 );// ms
            option.setCheckInterval( 20 );// ms
            datasource.updateDatasourceOptions( option );
            Sequoiadb sdb = datasource.getConnection();

            Thread.sleep( 60 );

            /*
             * do{ if (datasource.getIdleConnNum() !=
             * option.getDeltaIncCount()){ Thread.sleep(1); }else break;
             * }while(true);
             */
            int priorNum = datasource.getIdleConnNum();
            datasource.releaseConnection( sdb );
            int laterNum = datasource.getIdleConnNum();
            // Assert.assertEquals(laterNum, priorNum);

            Thread.sleep( 40 );
            // Assert.assertEquals(sdb.isValid(), false);
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } catch ( InterruptedException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        }
    }

    /**
     * 空闲连接数大于应保留连接数
     */
    @Test
    public void checkRerveConn7579() {
        try {
            DatasourceOptions option = new DatasourceOptions();
            int maxIdleCount = option.getMaxIdleCount();
            SequoiadbDatasource datasource = new SequoiadbDatasource(
                    this.coordAddr, userName, password, option );
            Sequoiadb sdb = datasource.getConnection();
            Assert.assertEquals( sdb.isValid(), true );
            Thread.sleep( 100 );
            int laterMaxIdleCount = datasource.getIdleConnNum();
            Assert.assertTrue( laterMaxIdleCount <= maxIdleCount );
            Assert.assertTrue( laterMaxIdleCount >= 0 );
        } catch ( InterruptedException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        } finally {
            datasource.close();
        }
    }

    /**
     * 归还旧版本的连接
     */
    @Test(timeOut = 20000)
    public void releaseOlderVersion7580() {
        try {
            long start = System.currentTimeMillis();
            Sequoiadb sdb = datasource.getConnection();
            int totalTimeLen = 10000;
            int alreadySleepTime = 0;
            do {
                int incNum = datasource.getDatasourceOptions()
                        .getDeltaIncCount();
                if ( datasource.getIdleConnNum() != incNum ) {
                    Thread.sleep( 10 );
                    alreadySleepTime += 10;
                } else
                    break;
            } while ( alreadySleepTime <= totalTimeLen );

            long end = System.currentTimeMillis();
            long findish1 = end - start;
            System.out.println( "step 1 findish" + findish1 );
            start = end;
            int priorNum = datasource.getIdleConnNum();
            DatasourceOptions option = new DatasourceOptions();
            option.setCheckInterval( 50 );
            option.setConnectStrategy( ConnectStrategy.RANDOM );
            datasource.updateDatasourceOptions( option );
            end = System.currentTimeMillis();
            long findish2 = end - start;
            System.out.println( "step 2 findish" + findish2 );
            start = end;

            datasource.releaseConnection( sdb );
            Thread.sleep( 100 );
            int laterNum = datasource.getIdleConnNum();
            if ( findish1 < 3000 && findish2 < 3000 ) {
                Assert.assertEquals( laterNum, priorNum );
            }
            end = System.currentTimeMillis();
            System.out.println( "step 3 findish" + ( end - start ) );
        } catch ( InterruptedException e ) {
            Assert.fail( e.getMessage() );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
    }

    /**
     * 关闭连接池后归还连接
     */
    @Test
    public void releaseAfterClose7581() {
        try {
            Sequoiadb sdb = datasource.getConnection();
            datasource.close();

            Assert.assertEquals( sdb.isValid(), false );
            datasource.releaseConnection( sdb );
            Assert.fail( "must throw exception!" );
        } catch ( InterruptedException e ) {
            Assert.fail( e.getMessage() );
        } catch ( BaseException e ) {
            // TODO: handle exception
            super.judegeErrCode( "SDB_CLIENT_CONNPOOL_CLOSE", e.getErrorCode() );
        }
    }

    /**
     * 禁用连接池后归还连接
     */
    @Test
    public void releaseAfterDisable7582() {
        try {
            Sequoiadb sdb = datasource.getConnection();
            datasource.disableDatasource();
            datasource.releaseConnection( sdb );
            Assert.assertEquals( sdb.isValid(), false );
            Assert.assertEquals( datasource.getUsedConnNum(), 0 );
        } catch ( InterruptedException e ) {
            // TODO Auto-generated catch block
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
    }

    /**
     * 重复归还连接
     */
    @Test
    public void releaseAfterRelease7583() {
        Sequoiadb sdb = null;
        try {
            DatasourceOptions option = new DatasourceOptions();
            option.setCheckInterval( 50 );
            datasource.updateDatasourceOptions( option );
            sdb = datasource.getConnection();
            datasource.releaseConnection( sdb );
            datasource.releaseConnection( sdb );
            Assert.fail( "must throw exception!" );
        } catch ( InterruptedException e ) {
            // TODO Auto-generated catch block
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        } catch ( BaseException e ) {
            super.judegeErrCode( "SDB_INVALIDARG", -6 );
        }

        try {
            for ( int i = 0; i < datasource.getIdleConnNum(); i++ ) {
                sdb = datasource.getConnection();
                Assert.assertEquals( sdb.isValid(), true );
            }
        } catch ( InterruptedException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
    }

    /**
     * 归还连接后对连接持有资源的处理
     */
    @Test
    public void checkResourceAfterRelease7584() throws InterruptedException {
        if ( isStandAlone() )
            return;
        Sequoiadb sdb;
        sdb = datasource.getConnection();
        DBCursor cursor = sdb.listReplicaGroups();
        datasource.releaseConnection( sdb );
        try {
            cursor.getNext();
            Assert.fail( "expect fail but success." );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -31 );
        }
        cursor.close();
    }
}
