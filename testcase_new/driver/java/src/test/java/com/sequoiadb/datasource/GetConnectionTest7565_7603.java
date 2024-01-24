package com.sequoiadb.datasource;

import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.Random;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.AfterMethod;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.BeforeMethod;

import org.testng.annotations.Test;

import com.sequoiadb.base.Node;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;

public class GetConnectionTest7565_7603 extends DataSourceTestBase {
    private SequoiadbDatasource datasource;
    private Sequoiadb sdb;
    Random random = new Random();

    @BeforeClass
    void initEnv() {
        super.init();
    }

    @BeforeMethod
    synchronized void createDataSource() {
        try {
            if ( datasource == null ) {
                DatasourceOptions option = new DatasourceOptions();
                option.setMaxCount( 200 );
                datasource = new SequoiadbDatasource( this.coordAddr,
                        this.userName, this.password, option );
            }
        } catch ( BaseException e ) {
            Assert.assertFalse( true, e.getMessage() );
        }
    }

    @AfterMethod
    synchronized void closeDataSource() {
        try {
            if ( null != datasource ) {
                datasource.close();
                datasource = null;
            }
        } catch ( BaseException e ) {
            Assert.assertFalse( true, e.getMessage() );
        }
    }

    @AfterClass
    void fini() {

    }

    @Test
    public void getOfContinuous() {
        try {
            // 申请一个连接
            Sequoiadb sdb1 = datasource.getConnection();
            Assert.assertEquals( sdb1.isValid(), true );
            Assert.assertEquals( datasource.getUsedConnNum(), 1 );

            // 再次申请一个连接
            Sequoiadb sdb2 = datasource.getConnection();
            Assert.assertEquals( sdb2.isValid(), true );
            Thread.sleep( 100 );
            Assert.assertEquals( datasource.getUsedConnNum(), 2 );

            Thread.sleep( 100 );
            // 关闭连接
            datasource.releaseConnection( sdb2 );
            datasource.releaseConnection( sdb1 );
            Assert.assertEquals( datasource.getUsedConnNum(), 0 );

        } catch ( InterruptedException e ) {
            Assert.fail( e.getMessage() );
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.assertFalse( true, e.getMessage() );
        }
    }

    private InetSocketAddress getAnyAddrNotEqualSpecialAddr(
            ArrayList< InetSocketAddress > exceptAddrList ) {
        InetSocketAddress addr = null;
        InetAddress localAddr = null;
        try {
            localAddr = InetAddress.getLocalHost();
        } catch ( UnknownHostException e1 ) {
            // TODO Auto-generated catch block
            e1.printStackTrace();
        }

        for ( int i = 0; i < addrList.size(); ++i ) {
            addr = addrList.get( i );
            InetAddress ipAddr = null;
            String hostName = null;
            try {
                hostName = addr.getHostName();
                ipAddr = InetAddress.getByName( hostName );
            } catch ( UnknownHostException e ) {
                // TODO Auto-generated catch block
                e.printStackTrace();
                return null;
            }
            int j = 0;
            for ( ; j < exceptAddrList.size(); ++j ) {
                System.out.println( "exceptAddr: " + exceptAddrList.get( j )
                        .getAddress().getHostAddress() );
                System.out.println( "addrList IP:" + ipAddr.getHostAddress() );
                System.out.println( "addr HostName:" + addr.getHostName() );
                System.out.println( "except HostName"
                        + exceptAddrList.get( j ).getHostName() );
                System.out.println( "local IP:" + localAddr.getHostAddress() );

                if ( !exceptAddrList.get( j ).getAddress().getHostAddress()
                        .equals( "" )
                        && !ipAddr.getHostAddress().equals( exceptAddrList
                                .get( j ).getAddress().getHostAddress() ) ) {
                    continue;
                } else if ( exceptAddrList.get( j ).getHostName()
                        .equals( "localhost" )
                        && !localAddr.getHostAddress()
                                .equals( ipAddr.getHostAddress() ) ) {
                    continue;
                } else if ( addr.getPort() == exceptAddrList.get( j )
                        .getPort() ) {
                    continue;
                } else {
                    break;
                }
            }

            if ( j == exceptAddrList.size() ) {
                break;
            }

        }
        return addr;
    }

    @Test
    void getConnectionOfValid() {
        InetSocketAddress addr = null;
        Sequoiadb db = null;
        InetSocketAddress inAddr = null;
        try {
            System.out.println( "getConnectionOfValid ..." );
            if ( datasource == null )
                return;
            sdb = datasource.getConnection();
            Assert.assertEquals( sdb.isValid(), true );

            getAddrList();
            if ( addrList.size() <= 1 )
                return;
            if ( sdb.getReplicaGroup( 2 )
                    .getNodeNum( Node.NodeStatus.SDB_NODE_ALL ) <= 1 )
                return;

            inAddr = getInCoordAddr();
            if ( inAddr == null )
                return;
            ArrayList< InetSocketAddress > tmpAddList = new ArrayList< InetSocketAddress >();
            tmpAddList.add( inAddr );
            System.out.println( "getOtherCoordAddr" );
            addr = getAnyAddrNotEqualSpecialAddr( tmpAddList );
            if ( addr == null || addr.getHostName() != null )
                return;
            db = new Sequoiadb( addr.getHostName(), addr.getPort(), userName,
                    password );
            ReplicaGroup group = db.getReplicaGroup( 2 );
            if ( group == null )
                return;
            Node node = group.getNode( inAddr.getHostName(), inAddr.getPort() );
            if ( node == null )
                return;
            node.stop();
            System.out.println( "release connection" );
            datasource.releaseConnection( sdb );
            System.out.println( "get connection" );
            sdb = datasource.getConnection();
            Assert.assertEquals( sdb.isValid(), false );
            DatasourceOptions option = new DatasourceOptions();
            option.setValidateConnection( true );
            datasource.updateDatasourceOptions( option );
            datasource.releaseConnection( sdb );

            sdb = datasource.getConnection();
            Assert.assertEquals( sdb.isValid(), true );
        } catch ( InterruptedException e ) {
            Assert.fail( e.getMessage() );
        } catch ( BaseException e ) {
            try {
                if ( e.getErrorCode() == SDBError.SDB_CLS_GRP_NOT_EXIST
                        .getErrorCode()
                        || e.getErrorCode() == SDBError.SDB_NETWORK
                                .getErrorCode() ) {
                    return;
                }
            } catch ( Exception e1 ) {
                // TODO Auto-generated catch block
                e1.printStackTrace();
            }

            judegeErrCode( "SDB_INVALIDARG", e.getErrorCode() );
            // e.printStackTrace();
            // Assert.assertTrue(false, e.getMessage());
        } finally {
            if ( addrList.size() > 1 && db != null
                    && db.getReplicaGroup( 2 ) != null ) {
                db.getReplicaGroup( 2 )
                        .getNode( inAddr.getHostName(), inAddr.getPort() )
                        .start();
            }
        }

        try {
            if ( null != addr ) {
                datasource
                        .addCoord( addr.getHostName() + ":" + addr.getPort() );
                sdb = datasource.getConnection();
                Assert.assertEquals( sdb.isValid(), true );
            }
        } catch ( InterruptedException e ) {
            Assert.fail( e.getMessage() );
        } catch ( BaseException e ) {
            // judegeErrCode("", e.getErrorCode());
            Assert.fail( e.getMessage() );
        }
        System.out.println( "getConnectionOfValid end" );
    }

    @Test
    void getConnectionAfterUpdateMaxCount() throws InterruptedException {
        ArrayList< Sequoiadb > dbs = new ArrayList<>();
        DatasourceOptions option = datasource.getDatasourceOptions();
        int oldPoolSize = option.getMaxCount();
        try {
            // 申请到池满(maxCount=200)
            for ( int i = 0; i < oldPoolSize; ++i ) {
                Sequoiadb db = datasource.getConnection();
                Assert.assertEquals( db.isValid(), true );
                dbs.add( db );
            }
        } catch ( BaseException e ) {
            System.out.println( "current get connection number " + dbs.size() );
            // SEQUOIADBMAINSTREAM-3625 暂时屏蔽该测试点
            if ( e.getErrorCode() != -254 ) {
                Assert.fail( e.getMessage() );
            }
        }
        // 修改连接池配置，调整maxCount=100、checkInterval=100
        int usedConnNum = datasource.getUsedConnNum();
        option.setCheckInterval( 100 );
        option.setMaxCount( oldPoolSize - 100 );
        datasource.updateDatasourceOptions( option );
        Assert.assertEquals( datasource.getDatasourceOptions().getMaxCount(),
                oldPoolSize - 100 );
        // 检查已经分配出去的连接是否可用
        for ( int k = 0; k < dbs.size(); ++k ) {
            Sequoiadb db = dbs.get( k );
            Assert.assertTrue( db.isValid() );
        }
        Assert.assertEquals( datasource.getUsedConnNum(), usedConnNum );

        // 检查是否可以再分配连接，预期报错
        try {
            datasource.getConnection();
            Assert.fail( "except fail but success" );
        } catch ( BaseException e ) {
            super.judegeErrCode( "SDB_DRIVER_DS_RUNOUT", e.getErrorCode() );
        }
        // 归还109个连接，剩余91个
        for ( int k = 0; k < 109; ++k ) {
            datasource.releaseConnection( dbs.get( k ) );
        }
        // 申请一个连接，验证当前连接池统计连接正确性
        datasource.getConnection();
        Assert.assertTrue( usedConnNum - 109 + 1 <= datasource.getIdleConnNum()
                + datasource.getUsedConnNum() );
        // 释放剩余91个连接，剩余1个

        for ( int k = 109; k < usedConnNum; ++k ) {
            datasource.releaseConnection( dbs.get( k ) );
        }
        Assert.assertEquals( dbs.size(), oldPoolSize );

        // 检查已使用连接是否等于设置值1
        Assert.assertEquals( datasource.getUsedConnNum(), 1 );

        // 再次重新分配到池满(maxCount=100)
        dbs.clear();
        try {
            for ( int i = 0; i < oldPoolSize; ++i ) {
                Sequoiadb db = datasource.getConnection();
                Assert.assertTrue( db.isValid() );
                dbs.add( db );
            }
            Assert.fail( "except fail but success" );
        } catch ( BaseException e ) {
            super.judegeErrCode( "SDB_DRIVER_DS_RUNOUT", e.getErrorCode() );
        }
        // 检查分配出的连接是否=maxCount()
        // 因问题单SEQUOIADBMAINSTREAM-7972，暂时屏蔽该测试点
        // Assert.assertEquals( dbs.size(),
        // datasource.getDatasourceOptions().getMaxCount() - 1 );
    }

    void getOfBalance() {
        if ( addrList.isEmpty() )
            return;

        int coordNumber = addrList.size();
        if ( !isContainAddr( getInCoordAddr() ) ) {
            coordNumber += 1;
        }

        Map< String, Integer > addr2Number = new HashMap< String, Integer >();
        try {
            for ( int i = 0; i < coordNumber * 10; ++i ) {
                Sequoiadb sdb = datasource.getConnection();
                Assert.assertEquals( sdb.isValid(), true );

                String srvAddr = sdb.getServerAddress().toString();

                if ( addr2Number.containsKey( srvAddr ) ) {
                    addr2Number.put( srvAddr, addr2Number.get( srvAddr ) + 1 );
                } else {
                    addr2Number.put( srvAddr, 1 );
                }
            }
        } catch ( InterruptedException e ) {
            Assert.assertFalse( true, e.getMessage() );
        } catch ( BaseException e ) {
            Assert.assertFalse( true, e.getMessage() );
        }

        Iterator iter = addr2Number.entrySet().iterator();
        while ( iter.hasNext() ) {
            Map.Entry entry = ( Map.Entry ) iter.next();
            Integer val = ( Integer ) entry.getValue();
            // Assert.assertEquals(val.intValue(), 10);
        }
    }

    void getOfBalance( DatasourceOptions option ) {
        if ( addrList.isEmpty() )
            return;
        InetSocketAddress inAddr = null;
        InetSocketAddress selectorAddr = null;
        InetSocketAddress expectAddr = null;
        ArrayList< InetSocketAddress > exceptAddrList = null;
        try {
            inAddr = super.getInCoordAddr();
            exceptAddrList = new ArrayList< InetSocketAddress >();
            exceptAddrList.add( inAddr );
            selectorAddr = getAnyAddrNotEqualSpecialAddr( exceptAddrList );
            if ( null == selectorAddr )
                return;
            datasource.addCoord(
                    selectorAddr.getHostName() + ":" + selectorAddr.getPort() );
        } catch ( BaseException e ) {
            Assert.assertFalse( true, e.getMessage() );
        }

        try {
            int curIdleNum = datasource.getIdleConnNum();
            Sequoiadb db = datasource.getConnection();
            Assert.assertEquals( db.isValid(), true );
            if ( db.getServerAddress().getHostAddress().equals( inAddr ) ) {
                expectAddr = selectorAddr;
            } else {
                expectAddr = inAddr;
            }
            datasource.releaseConnection( db );

            for ( int i = 0; i < curIdleNum; ++i ) {
                db = datasource.getConnection();
                Assert.assertEquals( db.isValid(), true );
                // Assert.assertTrue(db.getServerAddress().getHostAddress().equals(expectAddr));

                if ( db.getServerAddress().getHostAddress().equals( inAddr ) ) {
                    expectAddr = selectorAddr;
                } else {
                    expectAddr = inAddr;
                }
            }

            exceptAddrList.add( selectorAddr );
            selectorAddr = getAnyAddrNotEqualSpecialAddr( exceptAddrList );
            if ( null == selectorAddr )
                return;
            db = datasource.getConnection();
            Assert.assertEquals( db.isValid(), true );

            db = datasource.getConnection();
            Assert.assertEquals( db.isValid(), true );
            // Assert.assertTrue(db.getServerAddress().getHostAddress().equals(selectorAddr));

        } catch ( InterruptedException e ) {
            Assert.assertFalse( true, e.getMessage() );
        } catch ( BaseException e ) {
            Assert.assertFalse( true, e.getMessage() );
        }
        System.out.println( "getOfBalance" );
    }

    void getOfLocal( DatasourceOptions option ) {
        if ( addrList.isEmpty() )
            return;
        try {
            System.out.println( "getOfLocal Test ..." );
            String localIP = InetAddress.getLocalHost().getHostAddress();
            InetSocketAddress localAddr = null;
            InetSocketAddress addr = null;
            InetSocketAddress inAddr = null;
            for ( int i = 0; i < addrList.size(); ++i ) {
                localAddr = addrList.get( i );
                if ( localAddr.getHostName().equals( "localhost" ) || localAddr
                        .getAddress().getHostAddress().equals( localIP ) ) {
                    break;
                }
                localAddr = null;
            }
            if ( localAddr == null )
                return;
            System.out.println( "localAddr:" + localAddr.toString() );
            datasource.addCoord(
                    localAddr.getHostName() + ":" + localAddr.getPort() );
            inAddr = super.getInCoordAddr();
            ArrayList< InetSocketAddress > exceptAddrList = new ArrayList< InetSocketAddress >();
            exceptAddrList.add( inAddr );
            exceptAddrList.add( localAddr );
            addr = getAnyAddrNotEqualSpecialAddr( exceptAddrList );
            datasource.addCoord( addr.getHostName() + ":" + addr.getPort() );

            Sequoiadb sdb = datasource.getConnection();
            Assert.assertEquals( sdb.isValid(), true,
                    "getConnection is inValid" );
            int localAddrNum = datasource.getLocalAddrNum();
            if ( localAddrNum != 1 && localAddrNum != 2 ) {
                Assert.fail( "getLocalAddrNum is not 1 or 2, getLocalAddrNum : "
                        + localAddrNum );
            }

            for ( int i = 0; i < 10; ++i ) {
                sdb = datasource.getConnection();
                InetSocketAddress localHostAddr = new InetSocketAddress(
                        "127.0.0.1", localAddr.getPort() );
                boolean ret = sdb.getServerAddress().getHostAddress()
                        .equals( localHostAddr );
                ret |= sdb.getServerAddress().getHostAddress()
                        .equals( localAddr );
                Assert.assertTrue( ret,
                        sdb.getServerAddress().getHostAddress().toString()
                                + localAddr.toString() );
            }
        } catch ( UnknownHostException e ) {
            // TODO Auto-generated catch block
            e.printStackTrace();
            Assert.assertFalse( true, e.getMessage() );
        } catch ( InterruptedException e ) {
            Assert.assertFalse( true, e.getMessage() );
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.assertFalse( true, e.getMessage() );
        }
        System.out.println( "getOfLocal" );
    }

    void getOfSerial( DatasourceOptions option ) {
        if ( addrList.size() <= 1 )
            return;
        InetSocketAddress inAddr = null;
        InetSocketAddress selectorAddr = null;
        InetSocketAddress expectAddr = null;
        ArrayList< InetSocketAddress > exceptAddrList = null;
        try {
            inAddr = super.getInCoordAddr();
            exceptAddrList = new ArrayList< InetSocketAddress >();
            exceptAddrList.add( inAddr );
            selectorAddr = getAnyAddrNotEqualSpecialAddr( exceptAddrList );
            if ( null == selectorAddr )
                return;
            datasource.addCoord(
                    selectorAddr.getHostName() + ":" + selectorAddr.getPort() );
        } catch ( BaseException e ) {
            Assert.assertFalse( true, e.getMessage() );
        }

        try {
            Sequoiadb sdb = datasource.getConnection();
            Assert.assertEquals( sdb.isValid(), true );
            expectAddr = selectorAddr;

            exceptAddrList.add( selectorAddr );
            selectorAddr = getAnyAddrNotEqualSpecialAddr( exceptAddrList );
            if ( null == selectorAddr )
                return;
            datasource.addCoord(
                    selectorAddr.getHostName() + ":" + selectorAddr.getPort() );
            sdb = datasource.getConnection();
            // Assert.assertTrue(sdb.getServerAddress().getHostAddress().equals(expectAddr));
            Thread.sleep( 10 );

            sdb = datasource.getConnection();
            Assert.assertEquals( sdb.isValid(), true );
            // Assert.assertTrue(sdb.getServerAddress().getHostAddress().equals(selectorAddr));
        } catch ( InterruptedException e ) {
            Assert.assertFalse( true, e.getMessage() );
        } catch ( BaseException e ) {
            Assert.assertFalse( true, e.getMessage() );
        }
        System.out.println( "getOfSerial" );
    }

    void getOfRandom( DatasourceOptions option ) {
        if ( addrList.size() <= 1 )
            return;
        Map< InetSocketAddress, Integer > addr2Number = new HashMap< InetSocketAddress, Integer >();

        InetSocketAddress addr = null;
        for ( int i = 0; i < addrList.size(); ++i ) {
            addr = addrList.get( i );
            datasource.addCoord( addr.getHostName() + ":" + addr.getPort() );
        }

        for ( int i = 0; i < option.getMaxCount() - 1; ++i ) {
            try {
                Sequoiadb sdb = datasource.getConnection();
                Assert.assertEquals( sdb.isValid(), true );
                addr = sdb.getServerAddress().getHostAddress();
                if ( addr2Number.containsKey( addr ) ) {
                    addr2Number.put( addr, addr2Number.get( addr ) + 1 );
                } else {
                    addr2Number.put( addr, 1 );
                }
            } catch ( BaseException e ) {
                // TODO Auto-generated catch block
                e.printStackTrace();
                Assert.assertFalse( true, e.getMessage() );
            } catch ( InterruptedException e ) {
                // TODO Auto-generated catch block
                e.printStackTrace();
                Assert.assertFalse( true, e.getMessage() );
            }
        }

        int addrCount = addr2Number.size();
        Iterator< ? > iter = addr2Number.entrySet().iterator();
        while ( iter.hasNext() ) {
            Map.Entry entry = ( Map.Entry ) iter.next();
            Integer val = ( Integer ) entry.getValue();
            float expectRatio = 1.0f / addrCount;
            float ratio = ( float ) ( val.intValue() * 1.0
                    / option.getMaxCount() );
            System.out.println( "ratio=" + ratio );
            System.out.println( "expectRatio=" + expectRatio );
            // Assert.assertEquals(ratio, expectRatio, 0.1f);
        }
        System.out.println( "getOfRandom" );
    }

    @Test(dataProvider = "option-provider", dataProviderClass = SdbTestOptionFactory.class)
    void getTest7569_7572( DatasourceOptions option ) {
        try {
            sdb = datasource.getConnection();
            if ( addrList.isEmpty() ) {
                super.getAddrList();
            }

            datasource.updateDatasourceOptions( option );
            if ( option.getConnectStrategy() == ConnectStrategy.BALANCE ) {
                if ( option.getSyncCoordInterval() != 0 ) {
                    getOfBalance();
                } else {
                    getOfBalance( option );
                }
            } else if ( option.getConnectStrategy() == ConnectStrategy.LOCAL ) {
                getOfLocal( option );
            } else if ( option
                    .getConnectStrategy() == ConnectStrategy.SERIAL ) {
                getOfSerial( option );
            } else if ( option
                    .getConnectStrategy() == ConnectStrategy.RANDOM ) {
                getOfRandom( option );
            }
            datasource.releaseConnection( sdb );
        } catch ( InterruptedException e ) {
            Assert.fail( e.getMessage() );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
    }
}
