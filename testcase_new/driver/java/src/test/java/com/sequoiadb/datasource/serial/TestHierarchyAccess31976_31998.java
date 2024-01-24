package com.sequoiadb.datasource.serial;

import com.sequoiadb.base.ConfigOptions;
import com.sequoiadb.base.Node;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.datasource.ConnectStrategy;
import com.sequoiadb.datasource.DatasourceOptions;
import com.sequoiadb.datasource.SequoiadbDatasource;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.util.Helper;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.*;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

/**
 * @version 1.10
 * @Description seqDB-31989:初始化连接池设置location接口参数校验
 *              seqDB-31990:设置同步时间setSyncLocationIterval()接口参数校验
 *              seqDB-31976:最高亲和性等级为low，查看连接池访问
 *              seqDB-31977:最高亲和性等级为medium，查看连接池访问
 *              seqDB-31978:最高亲和性等级为high，查看连接池访问
 *              seqDB-31979:停止部分最高亲和性的节点，查看连接池访问
 *              seqDB-31984:更新唯一的当前最高亲和性等级节点的location为下一级亲和性等级
 *              seqDB-31985:更新唯一的当前最高亲和性等级节点的location为同级亲和性等级
 *              seqDB-31986:更新唯一的当前最高亲和性等级节点的location为上一级亲和性等级
 *              seqDB-31987:更新部分最高亲和性等级的节点为下一级亲和性等级
 *              seqDB-31988:更新较低亲和性等级的节点为最高亲和性等级
 *              seqDB-31998:更新coord节点地址集，查看连接池访问
 * @Author Cheng Jingjing
 * @Date 2023.06.06
 * @UpdateAuthor Cheng Jingjing
 * @UpdateDate 2023.06.06
 */

public class TestHierarchyAccess31976_31998 extends SdbTestBase {
    private static int PORT1;
    private static int PORT2;
    private static int PORT3;
    private static int PORT4;
    private static int PORT5;

    private static Sequoiadb db;
    private static ReplicaGroup coordRG;
    private static String dbPath;
    private static Node node1;
    private static Node node2;
    private static Node node3;
    private static Node node4;
    private static Node node5;
    private SequoiadbDatasource ds;
    private ConfigOptions netConfig;
    private DatasourceOptions options;

    @BeforeClass
    public static void prepareBeforeClass() {
        db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( db ) ) {
            throw new SkipException( "skip standalone." );
        }
        coordRG = db.getReplicaGroup( "SYSCoord" );

        PORT1 = SdbTestBase.reservedPortBegin + 10;
        PORT2 = SdbTestBase.reservedPortBegin + 20;
        PORT3 = SdbTestBase.reservedPortBegin + 30;
        PORT4 = SdbTestBase.reservedPortBegin + 40;
        PORT5 = SdbTestBase.reservedPortBegin + 50;

        // create coord group
        dbPath = SdbTestBase.reservedDir + "/coord/";

        node1 = coordRG.createNode( SdbTestBase.hostName, PORT1,
                dbPath + PORT1 );
        node2 = coordRG.createNode( SdbTestBase.hostName, PORT2,
                dbPath + PORT2 );
        node3 = coordRG.createNode( SdbTestBase.hostName, PORT3,
                dbPath + PORT3 );
        node4 = coordRG.createNode( SdbTestBase.hostName, PORT4,
                dbPath + PORT4 );
        node5 = coordRG.createNode( SdbTestBase.hostName, PORT5,
                dbPath + PORT5 );
    }

    @AfterClass
    public static void cleanAfterClass() {
        try {
            coordRG.removeNode( SdbTestBase.hostName, PORT1, null );
            coordRG.removeNode( SdbTestBase.hostName, PORT2, null );
            coordRG.removeNode( SdbTestBase.hostName, PORT3, null );
            coordRG.removeNode( SdbTestBase.hostName, PORT4, null );
            coordRG.removeNode( SdbTestBase.hostName, PORT5, null );
        } finally {
            db.close();
        }
    }

    @BeforeMethod
    public void setUp() {
        netConfig = new ConfigOptions();
        netConfig.setConnectTimeout( 100 );
        netConfig.setMaxAutoConnectRetryTime( 200 );

        options = new DatasourceOptions();
        options.setMaxCount( 10 );
        options.setMinIdleCount( 3 );
        options.setMaxIdleCount( 5 );
        options.setConnectStrategy( ConnectStrategy.SERIAL );
        options.setSyncCoordInterval( 0 );

        coordRG.start();

        node1.setLocation( "" );
        node2.setLocation( "" );
        node3.setLocation( "" );
        node4.setLocation( "" );
        node5.setLocation( "" );
    }

    @AfterMethod
    public void tearDown() {
        if ( ds != null ) {
            ds.close();
        }
    }

    // seqDB-31989:初始化连接池设置location接口参数校验
    @Test
    public void locationNameTest31989() throws Exception {
        try {
            createDSWithLocation( null );
            Assert.fail( "should fail but success!" );
        } catch ( BaseException e ) {
            Assert.assertEquals( SDBError.SDB_INVALIDARG.getErrorCode(),
                    e.getErrorCode() );
        }

        createDSWithLocation( "guangzhou" );

        createDSWithLocation( "guangzhou.NANSHA" );

        createDSWithLocation( "" );
    }

    // seqDB-31990:设置同步时间setSyncLocationIterval()接口参数校验
    @Test
    public void syncLocationTest31990() throws Exception {
        // check default value
        Assert.assertEquals( options.getSyncLocationInterval(), 60 * 1000 );

        // error value
        try {
            options.setSyncCoordInterval( -1 );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(),
                    SDBError.SDB_INVALIDARG.getErrorCode() );
        }

        String location = "guangzhou.nansha";

        // invalid interval, not sync location information
        checkSyncLocation( location, 0, false );

        // invalid location, not sync location information
        checkSyncLocation( "", 100, false );

        // location and inteval are both valid, sync location information
        checkSyncLocation( location, 100, true );
    }

    // seqDB-31976:最高亲和性等级为low，查看连接池访问
    @Test
    public void locationPriorityTest31976() throws Exception {
        node1.setLocation( "shanghai" );
        node2.setLocation( "nansha" );
        node3.setLocation( "" );

        List< String > addrLst = new ArrayList<>();
        addrLst.add( node1.getNodeName() );
        addrLst.add( node2.getNodeName() );
        addrLst.add( node3.getNodeName() );
        addrLst.add( node4.getNodeName() );

        // locationPriority is low
        ds = createDS( addrLst, "guangzhou.nansha", options );
        checkConn( ds, addrLst );
    }

    // seqDB-31977:最高亲和性等级为medium，查看连接池访问
    @Test
    public void locationPriorityTest31977() throws Exception {
        node1.setLocation( "guangzhou.panyu" );
        node2.setLocation( "nansha" );
        node3.setLocation( "" );

        List< String > addrLst = new ArrayList<>();
        addrLst.add( node1.getNodeName() );
        addrLst.add( node2.getNodeName() );
        addrLst.add( node3.getNodeName() );
        addrLst.add( node4.getNodeName() );

        // locationPriority is medium
        ds = createDS( addrLst, "guangzhou.nansha", options );
        List< String > connList = new ArrayList<>();
        connList.add( node1.getNodeName() );
        checkConn( ds, connList );
    }

    // seqDB-31978:最高亲和性等级为high，查看连接池访问
    @Test
    public void locationPriorityTest31978() throws Exception {
        node1.setLocation( "guangzhou.nansha" );
        node2.setLocation( "GUANGZHOU.nansha" );
        node3.setLocation( "guangzhou.baiyun" );
        node4.setLocation( "shanghai" );

        List< String > addrLst = new ArrayList<>();
        addrLst.add( node1.getNodeName() );
        addrLst.add( node2.getNodeName() );
        addrLst.add( node3.getNodeName() );
        addrLst.add( node4.getNodeName() );

        // locationPriority is high
        ds = createDS( addrLst, "guangzhou.nansha", options );
        List< String > connList = new ArrayList<>();
        connList.add( node1.getNodeName() );
        checkConn( ds, connList );
    }

    // seqDB-31979:停止部分最高亲和性的节点，查看连接池访问
    @Test
    public void syncAddressTest31979() throws Exception {
        node1.setLocation( "guangzhou.panyu" );
        node2.setLocation( "guangzhou" );
        node3.setLocation( "nansha" );
        node4.setLocation( "" );

        List< String > addrLst = new ArrayList<>();
        addrLst.add( node1.getNodeName() );
        addrLst.add( node2.getNodeName() );
        addrLst.add( node3.getNodeName() );
        addrLst.add( node4.getNodeName() );
        addrLst.add( node5.getNodeName() );

        // locationPriority is medium
        // options.setSyncLocationInterval( 1 * 100 );
        ds = createDS( addrLst, "guangzhou.nansha", options );
        List< String > connList1 = new ArrayList<>();
        connList1.add( node1.getNodeName() );
        connList1.add( node2.getNodeName() );
        checkConn( ds, connList1 );

        // stop node2
        node2.stop();
        try {
            List< String > connList2 = new ArrayList<>();
            connList2.add( node1.getNodeName() );
            checkConn( ds, connList2 );
        } finally {
            node2.start();
        }

        // stop node1
        node1.stop();
        try {
            List< String > connList3 = new ArrayList<>();
            connList3.add( node3.getNodeName() );
            connList3.add( node4.getNodeName() );
            connList3.add( node5.getNodeName() );
            checkConn( ds, connList3 );
        } finally {
            node1.start();
        }
    }

    // seqDB-31984:更新唯一的当前最高亲和性等级节点的location为下一级亲和性等级
    // seqDB-31985:更新唯一的当前最高亲和性等级节点的location为同级亲和性等级
    // seqDB-31986:更新唯一的当前最高亲和性等级节点的location为上一级亲和性等级
    @Test
    public void updateLocationTest31984() throws Exception {
        node1.setLocation( "panyu" );
        node2.setLocation( "guangzhou" );
        node3.setLocation( "nansha" );
        node4.setLocation( "NANSHA" );

        List< String > addrLst = new ArrayList<>();
        addrLst.add( node1.getNodeName() );
        addrLst.add( node2.getNodeName() );
        addrLst.add( node3.getNodeName() );
        addrLst.add( node4.getNodeName() );

        // locationPriority is high
        options.setSyncLocationInterval( 1 * 1000 );
        ds = createDS( addrLst, "guangzhou.nansha", options );
        List< String > connList1 = new ArrayList<>();
        connList1.add( node2.getNodeName() );
        checkConn( ds, connList1 );

        // update node2 location( seqDB-31984 )
        node2.setLocation( "china.guangzhou" );

        // wait SynchronizeAddressTask run
        Thread.sleep( 2 * 1000 );
        try {
            List< String > connList2 = new ArrayList<>();
            connList2.add( node1.getNodeName() );
            connList2.add( node2.getNodeName() );
            connList2.add( node3.getNodeName() );
            connList2.add( node4.getNodeName() );
            checkConn( ds, connList2 );
        } finally {
            node2.setLocation( "guangzhou" );
        }

        // update node2 location( seqDB-31985 )
        node2.setLocation( "guangzhou.panyu" );

        // wait SynchronizeAddressTask run
        Thread.sleep( 2 * 1000 );
        try {
            checkConn( ds, connList1 );
        } finally {
            node2.setLocation( "guangzhou" );
        }

        // update node2 location( seqDB-31986 )
        node2.setLocation( "guangzhou.nansha" );

        // wait SynchronizeAddressTask run
        Thread.sleep( 2 * 1000 );
        checkConn( ds, connList1 );
    }

    // seqDB-31987:更新部分最高亲和性等级的节点为下一级亲和性等级
    // seqDB-31988:更新较低亲和性等级的节点为最高亲和性等级
    @Test
    public void updateLocationTest31987() throws Exception {
        node1.setLocation( "guangzhou.panyu" );
        node2.setLocation( "guangzhou" );
        node3.setLocation( "guangzhou.NANSHA" );
        node4.setLocation( "shanghai" );
        node5.setLocation( "" );

        List< String > addrLst = new ArrayList<>();
        addrLst.add( node1.getNodeName() );
        addrLst.add( node2.getNodeName() );
        addrLst.add( node3.getNodeName() );
        addrLst.add( node4.getNodeName() );
        addrLst.add( node5.getNodeName() );

        // locationPriority is medium
        options.setSyncLocationInterval( 1 * 1000 );
        ds = createDS( addrLst, "guangzhou.nansha", options );
        List< String > connList1 = new ArrayList<>();
        connList1.add( node1.getNodeName() );
        connList1.add( node2.getNodeName() );
        connList1.add( node3.getNodeName() );
        checkConn( ds, connList1 );

        // update node1 location( seqDB-31987 )
        node1.setLocation( "panyu" );

        // wait SynchronizeAddressTask run
        Thread.sleep( 2 * 1000 );
        List< String > connList2 = new ArrayList<>();
        connList2.add( node2.getNodeName() );
        connList2.add( node3.getNodeName() );
        checkConn( ds, connList2 );

        // update node1 location( seqDB-31988 )
        node1.setLocation( "guangzhou.nansha" );

        // wait SynchronizeAddressTask run
        Thread.sleep( 2 * 1000 );
        List< String > connList3 = new ArrayList<>();
        connList3.add( node1.getNodeName() );
        checkConn( ds, connList3 );
    }

    // seqDB-31998:更新coord节点地址集，查看连接池访问
    @Test
    public void addAndRemoveAddressTest31998() throws Exception {
        node1.setLocation( "guangzhou.nansha" );
        node2.setLocation( "guangzhou" );
        node3.setLocation( "shanghai" );

        List< String > addrLst = new ArrayList<>();
        addrLst.add( node2.getNodeName() );

        options.setSyncLocationInterval( 1 * 1000 );
        ds = createDS( addrLst, "guangzhou.nansha", options );
        List< String > connList1 = new ArrayList<>();
        connList1.add( node2.getNodeName() );
        checkConn( ds, connList1 );

        // add node1、node3
        ds.addCoord( node1.getNodeName() );
        ds.addCoord( node3.getNodeName() );
        // wait SynchronizeAddressTask run
        Thread.sleep( 1 * 1000 );
        List< String > connList2 = new ArrayList<>();
        connList2.add( node1.getNodeName() );
        checkConn( ds, connList2 );

        // remove node1、node2
        ds.removeCoord( node1.getNodeName() );
        ds.removeCoord( node2.getNodeName() );
        // wait SynchronizeAddressTask run
        Thread.sleep( 1 * 1000 );
        List< String > connList3 = new ArrayList<>();
        connList3.add( node3.getNodeName() );
        checkConn( ds, connList3 );
    }

    private void checkSyncLocation( String location, int syncLocationInterval,
            boolean sync ) throws Exception {
        List< String > addrLst = new ArrayList<>();
        addrLst.add( node1.getNodeName() );
        addrLst.add( node2.getNodeName() );
        addrLst.add( node3.getNodeName() );

        options.setSyncLocationInterval( syncLocationInterval );
        SequoiadbDatasource ds = createDS( addrLst, location, options );
        try {
            node1.setLocation( location );
            node2.setLocation( location );
            node2.stop();
            Thread.sleep( syncLocationInterval + 100 );
            if ( sync ) {
                addrLst.remove( node3.getNodeName() );
            }
            addrLst.remove( node2.getNodeName() );
            checkConn( ds, addrLst );
        } finally {
            node2.start();
            node1.setLocation( "" );
            node2.setLocation( "" );
            ds.close();
        }
    }

    private void createDSWithLocation( String location ) throws Exception {
        List< String > addrLst = new ArrayList<>();
        addrLst.add( node1.getNodeName() );
        ds = createDS( addrLst, location, options );
        Assert.assertEquals( location, ds.getLocation() );
    }

    private SequoiadbDatasource createDS( List< String > addrLst,
            String location, DatasourceOptions options ) throws Exception {
        SequoiadbDatasource ds = SequoiadbDatasource.builder()
                .serverAddress( addrLst ).location( location )
                .datasourceOptions( options ).configOptions( netConfig )
                .build();

        List< Sequoiadb > connList = new ArrayList<>();
        for ( int i = 0; i < ds.getDatasourceOptions().getMaxCount(); i++ ) {
            connList.add( ds.getConnection() );
        }
        for ( Sequoiadb db : connList ) {
            ds.releaseConnection( db );
        }

        return ds;
    }

    private void checkConn( SequoiadbDatasource ds, List< String > addressList )
            throws Exception {
        List< Sequoiadb > connList = new ArrayList<>();
        Set< String > addrSet = new HashSet<>();
        // clear old conn
        for ( int i = 0; i < ds.getDatasourceOptions().getMaxCount(); i++ ) {
            connList.add( ds.getConnection() );
        }
        for ( Sequoiadb db : connList ) {
            try {
                db.close();
            } catch ( BaseException e ) {
                // ignore
            }
            ds.releaseConnection( db );
        }
        connList.clear();

        // get new conn
        for ( int i = 0; i < ds.getDatasourceOptions().getMaxCount(); i++ ) {
            Sequoiadb db = ds.getConnection();
            connList.add( db );
            addrSet.add( db.getNodeName() );
        }
        for ( Sequoiadb db : connList ) {
            ds.releaseConnection( db );
        }

        // parse hostname:port to ip:port
        List< String > addrLst = new ArrayList<>();
        for ( String addr : addressList ) {
            addrLst.add( Helper.parseAddress( addr ) );
        }

        // check
        String errMsg = "\n" + "Expected: " + addrLst + "\n" + "Actual: "
                + addrSet;
        for ( String addr : addrLst ) {
            Assert.assertTrue( addrSet.contains( addr ), errMsg );
        }
        for ( String addr : addrSet ) {
            Assert.assertTrue( addrLst.contains( addr ), errMsg );
        }
    }
}
