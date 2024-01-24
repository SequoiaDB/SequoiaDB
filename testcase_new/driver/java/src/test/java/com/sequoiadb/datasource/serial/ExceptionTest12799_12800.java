/**
 * Copyright (c) 2017, SequoiaDB Ltd.
 * File Name:ExceptionTest.java
 * 类的详细描述
 *
 *  @author 类创建者姓名
 * Date:2017-9-22下午12:55:20
 *  @version 1.00
 */
package com.sequoiadb.datasource.serial;

import java.net.InetAddress;
import com.sequoiadb.datasource.DataSourceTestBase;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.List;

import org.testng.Assert;
import org.testng.annotations.AfterMethod;
import org.testng.annotations.BeforeMethod;
import org.testng.annotations.Test;
import com.sequoiadb.datasource.SequoiadbDatasource;
import com.sequoiadb.base.Node;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.datasource.DatasourceOptions;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.SdbTestBase;

public class ExceptionTest12799_12800 extends DataSourceTestBase {
    private SequoiadbDatasource ds;

    @BeforeMethod
    void setUp() {
        try {
            DatasourceOptions dsOpt = new DatasourceOptions();
            dsOpt.setMaxCount( 20 );
            dsOpt.setSyncCoordInterval( 1 );
            ds = new SequoiadbDatasource( SdbTestBase.coordUrl, "", "", dsOpt );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
    }

    @AfterMethod
    void tearDown() {
        try {
            ds.close();
        } catch ( BaseException e ) {

        }
    }

    private void getConnToPoolFull( List< Sequoiadb > dbs ) {
        do {
            try {
                dbs.add( ds.getConnection() );
            } catch ( BaseException e ) {
                throw e;
            } catch ( InterruptedException e ) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }
        } while ( true );

    }

    @Test
    void testGetConnByAllCoordInvalid12799() {

        if ( ds != null ) {
            ds.close();
        }
        String invalidUrl = "192.168.10.63:11810";
        DatasourceOptions dsOpt = new DatasourceOptions();
        dsOpt.setMaxCount( 20 );

        ds = new SequoiadbDatasource( invalidUrl, "", "", dsOpt );
        List< Sequoiadb > dbs = new ArrayList< Sequoiadb >();
        try {
            getConnToPoolFull( dbs );
        } catch ( BaseException e ) {
            // TODO Auto-generated catch block
            if ( e.getErrorCode() != SDBError.valueOf( "SDB_NETWORK" )
                    .getErrorCode() ) {
                Assert.fail( e.getMessage() );
            }
        }

        ds.addCoord( SdbTestBase.coordUrl );
        ds.removeCoord( invalidUrl );
        try {
            getConnToPoolFull( dbs );
        } catch ( BaseException e ) {
            // TODO Auto-generated catch block
            if ( e.getErrorCode() != SDBError.valueOf( "SDB_DRIVER_DS_RUNOUT" )
                    .getErrorCode() ) {
                Assert.fail( e.getMessage() );
            }
        }

        Assert.assertEquals( ds.getDatasourceOptions().getMaxCount(),
                dbs.size() );

    }

    @Test
    void testReleConnByConnInvalid12800() {
        // for sync coord
        int alreadTimeLen = 0;
        do {
            try {
                Thread.sleep( 10 );
                alreadTimeLen += 10;
                if ( ds.getNormalAddrNum() != 1
                        || alreadTimeLen / 1000 >= 10 ) {
                    break;
                }
            } catch ( InterruptedException e1 ) {
                // TODO Auto-generated catch block
                e1.printStackTrace();
            }
        } while ( true );
        List< Sequoiadb > dbs = new ArrayList< Sequoiadb >();
        try {
            getConnToPoolFull( dbs );
        } catch ( BaseException e ) {
            // TODO Auto-generated catch block
            if ( e.getErrorCode() != SDBError.valueOf( "SDB_DRIVER_DS_RUNOUT" )
                    .getErrorCode() ) {
                Assert.fail( e.getMessage() );
            }
        }

        Assert.assertEquals( ds.getDatasourceOptions().getMaxCount(),
                dbs.size() );
        if ( ds.getNormalAddrNum() == 1 ) {
            return;
        }

        String addrByStop = null;
        for ( Sequoiadb db : dbs ) {
            String tmpCoordAddr = "";
            try {
                InetAddress[] addrs = InetAddress.getAllByName( db.getHost() );
                tmpCoordAddr = addrs[ 0 ].getHostAddress() + ":" + db.getPort();
            } catch ( UnknownHostException e ) {
                // TODO Auto-generated catch block
                Assert.fail( e.getMessage() );
            }

            String inCoordAddr = "";
            String[] pair = SdbTestBase.coordUrl.split( ":" );
            try {
                InetAddress[] addrs = InetAddress.getAllByName( pair[ 0 ] );
                inCoordAddr = addrs[ 0 ].getHostAddress() + ":" + pair[ 1 ];
            } catch ( UnknownHostException e ) {
                // TODO Auto-generated catch block
                Assert.fail( e.getMessage() );
            }

            if ( !inCoordAddr.equals( tmpCoordAddr ) ) {
                addrByStop = db.getHost() + ":" + db.getPort();
                break;
            }
        }

        Node coordByStop = null;
        if ( addrByStop != null ) {
            int index = 0;
            do {

                Sequoiadb db = dbs.get( index );
                String tmp = db.getHost() + ":" + db.getPort();
                if ( !addrByStop.equals( tmp ) ) {
                    break;
                }
                index++;
            } while ( true );
            coordByStop = dbs.get( index ).getReplicaGroup( 2 )
                    .getNode( addrByStop );
            coordByStop.stop();
        }

        for ( Sequoiadb db : dbs ) {
            ds.releaseConnection( db );
        }
        dbs.clear();

        try {
            getConnToPoolFull( dbs );
        } catch ( BaseException e ) {
            // TODO Auto-generated catch block
            if ( e.getErrorCode() != SDBError.valueOf( "SDB_DRIVER_DS_RUNOUT" )
                    .getErrorCode() ) {
                Assert.fail( e.getMessage() );
            }
        } finally {
            coordByStop.start();
        }
        Assert.assertEquals( ds.getDatasourceOptions().getMaxCount(),
                dbs.size() );

    }

}
