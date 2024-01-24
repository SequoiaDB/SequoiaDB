package com.sequoiadb.test.datasource;

import com.sequoiadb.base.ConfigOptions;
import com.sequoiadb.base.Node;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.datasource.ConnectStrategy;
import com.sequoiadb.datasource.DatasourceOptions;
import com.sequoiadb.datasource.SequoiadbDatasource;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.test.common.Constants;
import com.sequoiadb.util.Helper;
import org.junit.*;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

public class LocationTest {
    private final static int PORT1 = 51000;
    private final static int PORT2 = 52000;
    private final static int PORT3 = 53000;
    private final static int PORT4 = 54000;
    private final static int PORT5 = 55000;
    private static String dbPath;
    private static Sequoiadb db;
    private static ReplicaGroup coordRG;
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
        db = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");
        coordRG = db.getReplicaGroup("SYSCoord");

        dbPath = Constants.DB_PATH + "/coord/";
        node1 = coordRG.createNode(Constants.HOST, PORT1, dbPath + PORT1);
        node2 = coordRG.createNode(Constants.HOST, PORT2, dbPath + PORT2);
        node3 = coordRG.createNode(Constants.HOST, PORT3, dbPath + PORT3);
        node4 = coordRG.createNode(Constants.HOST, PORT4, dbPath + PORT4);
        node5 = coordRG.createNode(Constants.HOST, PORT5, dbPath + PORT5);
    }

    @AfterClass
    public static void cleanAfterClass() {
        try {
            coordRG.removeNode(Constants.HOST, PORT1, null);
            coordRG.removeNode(Constants.HOST, PORT2, null);
            coordRG.removeNode(Constants.HOST, PORT3, null);
            coordRG.removeNode(Constants.HOST, PORT4, null);
            coordRG.removeNode(Constants.HOST, PORT5, null);
        } finally {
            db.close();
        }
    }

    @Before
    public void setUp() {
        netConfig = new ConfigOptions();
        netConfig.setConnectTimeout(100);
        netConfig.setMaxAutoConnectRetryTime(200);

        options = new DatasourceOptions();
        options.setMaxCount(10);
        options.setMinIdleCount(3);
        options.setMaxIdleCount(5);
        options.setConnectStrategy(ConnectStrategy.SERIAL);
        options.setSyncCoordInterval(0);

        coordRG.start();

        node1.setLocation("");
        node2.setLocation("");
        node3.setLocation("");
        node4.setLocation("");
        node5.setLocation("");
    }

    @After
    public void tearDown() {
        if (ds != null) {
            ds.close();
        }
    }

    @Test
    public void locationNameTest() throws Exception {
        try {
            createDSWithLocation(null);
        } catch (BaseException e) {
            Assert.assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode());
        }

        createDSWithLocation("guangzhou.nansha");

        createDSWithLocation("guangzhou.NANSHA");

        createDSWithLocation("");
    }

    private void createDSWithLocation(String location) throws Exception {
        List<String> addrLst = new ArrayList<>();
        addrLst.add(node1.getNodeName());
        ds = createDS(addrLst, location, options);
        Assert.assertEquals(location, ds.getLocation());
    }

    @Test
    public void syncLocationTest() throws Exception {
        // check default value
        Assert.assertEquals(60 * 1000, options.getSyncLocationInterval());

        // error value
        try {
            options.setSyncLocationInterval(-1);
        } catch (BaseException e) {
            Assert.assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode());
        }

        String location = "guangzhou.nansha";

        // invalid interval, not sync location information
        checkSyncLocation(location, 0, false);

        // invalid location, not sync location information
        checkSyncLocation("", 100, false);

        // location and interval is valid, sync location information
        checkSyncLocation(location, 100, true);
    }

    private void checkSyncLocation(String location, int syncLocationInterval, boolean sync) throws Exception {
        List<String> addrLst = new ArrayList<>();
        addrLst.add(node1.getNodeName());
        addrLst.add(node2.getNodeName());
        addrLst.add(node3.getNodeName());

        options.setSyncLocationInterval(syncLocationInterval);
        SequoiadbDatasource ds = createDS(addrLst, location, options);
        try {
            node1.setLocation(location);
            node2.setLocation(location);
            node2.stop();
            Thread.sleep(syncLocationInterval + 100);
            if (sync) {
                addrLst.remove(node3.getNodeName());
            }
            addrLst.remove(node2.getNodeName());
            checkConn(ds, addrLst);
        } finally {
            node2.start();
            node1.setLocation("");
            node2.setLocation("");
            ds.close();
        }
    }

    @Test
    public void syncAddressTest() throws Exception {
        String location1 = "guangzhou.nansha";
        String location2 = "guangzhou.fanyu";

        node1.setLocation(location1);
        node2.setLocation(location2);
        node3.setLocation(location2);
        node4.setLocation(location2);
        node5.setLocation(location2);

        node3.stop();
        node5.stop();

        List<String> addrLst = new ArrayList<>();
        addrLst.add(node1.getNodeName());
        addrLst.add(node2.getNodeName());
        addrLst.add(node3.getNodeName());

        // only sync address
        options.setSyncLocationInterval(0);
        options.setSyncCoordInterval(2 * 1000);

        ds = createDS(addrLst, location1, options);
        List<String> addrLst1 = new ArrayList<>();
        addrLst1.add(node1.getNodeName());
        checkConn(ds, addrLst1);

        coordRG.removeNode(Constants.HOST, PORT1, null);
        try {
            // waite SynchronizeAddressTask run
            Thread.sleep( 3 * 1000);
            List<String> addrLst2 = new ArrayList<>();
            addrLst2.add(node2.getNodeName());
            addrLst2.add(node4.getNodeName());
            checkConn(ds, addrLst2);
        } finally {
            node1 = coordRG.createNode(Constants.HOST, PORT1, dbPath + PORT1);
            node1.start();
        }
    }

    @Test
    public void locationPriorityTest() throws Exception {
        String location1 = "guangzhou.nansha";
        String location2 = "GUANGZHOU.nansha";
        String location3 = "guangzhou.fanyu";
        String location4 = "shanghai";
        String location5 = "";

        node1.setLocation(location4);
        node2.setLocation(location4);
        node3.setLocation(location4);
        node4.setLocation(location4);
        node5.setLocation(location4);

        List<String> addrLst = new ArrayList<>();
        addrLst.add(node1.getNodeName());
        addrLst.add(node2.getNodeName());
        addrLst.add(node3.getNodeName());
        addrLst.add(node4.getNodeName());
        addrLst.add(node5.getNodeName());
        options.setSyncLocationInterval(1000);

        // locationPriority: LOW
        ds = createDS(addrLst, location1, options);
        checkConn(ds, addrLst);

        node1.setLocation(location1);
        node2.setLocation(location2);
        node3.setLocation(location3);
        node5.setLocation(location5);
        node3.stop();
        Thread.sleep(1000);

        // locationPriority: HIGH
        List<String> addrLst1 = new ArrayList<>();
        addrLst1.add(node1.getNodeName());
        checkConn(ds, addrLst1);

        // address disable, locationPriority: MIDDLE
        node1.stop();
        Thread.sleep(1000);
        List<String> addrLst2 = new ArrayList<>();
        addrLst2.add(node2.getNodeName());
        checkConn(ds, addrLst2);

        // address enable, locationPriority: HIGH
        node1.start();
        // wait for RetrieveAddressTask run
        Thread.sleep(60 * 1000 + 1000);
        checkConn(ds, addrLst1);

        // locationPriority: LOW
        node1.stop();
        ds.removeCoord(node2.getNodeName());
        Thread.sleep(1000);
        List<String> addrLst3 = new ArrayList<>();
        addrLst3.add(node4.getNodeName());
        addrLst3.add(node5.getNodeName());
        checkConn(ds, addrLst3);

        // address enable, locationPriority: HIGH
        node1.start();
        // wait for RetrieveAddressTask run
        Thread.sleep(60 * 1000 + 1000);
        checkConn(ds, addrLst1);
    }

    @Test
    public void addAndRemoveAddressTest() throws Exception {
        String location1 = "guangzhou.nansha";
        String location2 = "guangzhou.fanyu";

        node1.setLocation(location1);
        node2.setLocation(location2);
        node3.setLocation(location2);

        List<String> addrLst = new ArrayList<>();
        addrLst.add(node1.getNodeName());

        options.setSyncLocationInterval(0);
        ds = createDS(addrLst, location2, options);

        // sync location interval is 0, not sync location information
        ds.addCoord(node2.getNodeName());
        checkConn(ds, addrLst);
        ds.removeCoord(node2.getNodeName());

        // sync location info
        options.setSyncLocationInterval(1000);
        ds.updateDatasourceOptions(options);

        node3.stop();
        // add normal address
        ds.addCoord(node2.getNodeName());
        // add abnormal address
        ds.addCoord(node3.getNodeName());
        Thread.sleep(1000);
        addrLst.remove(node1.getNodeName());
        addrLst.add(node2.getNodeName());
        checkConn(ds, addrLst);

        // remove normal address
        ds.removeCoord(node2.getNodeName());
        addrLst.remove(node2.getNodeName());
        addrLst.add(node1.getNodeName());
        checkConn(ds, addrLst);

        // remove abnormal address
        ds.removeCoord(node3.getNodeName());
        checkConn(ds, addrLst);
    }

    @Test
    public void disableTest() throws Exception {
        String location = "guangzhou.nansha";

        node1.setLocation(location);
        node2.setLocation(location);

        List<String> addrLst = new ArrayList<>();
        addrLst.add(node1.getNodeName());
        addrLst.add(node2.getNodeName());

        options.setSyncLocationInterval(0);
        ds = createDS(addrLst, location, options);
        checkConn(ds, addrLst);

        ds.disableDatasource();
        checkConn(ds, addrLst);

        ds.removeCoord(node2.getNodeName());
        addrLst.remove(node2.getNodeName());
        checkConn(ds, addrLst);

        ds.addCoord(node2.getNodeName());
        checkConn(ds, addrLst);

        node1.stop();
        Thread.sleep(1000);
        addrLst.remove(node1.getNodeName());
        addrLst.add(node2.getNodeName());
        checkConn(ds, addrLst);
    }

    private SequoiadbDatasource createDS(List<String> addrLst, String location, DatasourceOptions options) throws Exception {
        SequoiadbDatasource ds = SequoiadbDatasource.builder()
                .serverAddress(addrLst)
                .location(location)
                .datasourceOptions(options)
                .configOptions(netConfig)
                .build();

        List<Sequoiadb> connList = new ArrayList<>();
        for (int i = 0; i < ds.getDatasourceOptions().getMaxCount(); i++) {
            connList.add(ds.getConnection());
        }
        for (Sequoiadb db : connList) {
            ds.releaseConnection(db);
        }

        return ds;
    }

    private void checkConn(SequoiadbDatasource ds, List<String> addressList) throws Exception {
        List<Sequoiadb> connList = new ArrayList<>();
        Set<String> addrSet = new HashSet<>();

        // clear old conn
        for (int i = 0; i < ds.getDatasourceOptions().getMaxCount(); i++) {
            connList.add(ds.getConnection());
        }
        for (Sequoiadb db : connList) {
            try {
                db.close();
            } catch (BaseException e) {
                // ignore
            }
            ds.releaseConnection(db);
        }
        connList.clear();

        // get new conn
        for (int i = 0; i < ds.getDatasourceOptions().getMaxCount(); i++) {
            Sequoiadb db = ds.getConnection();
            connList.add(db);
            addrSet.add(db.getNodeName());
        }
        for (Sequoiadb db : connList) {
            ds.releaseConnection(db);
        }

        // parse hostname:port to ip:port
        List<String> addrLst = new ArrayList<>();
        for (String addr : addressList) {
            addrLst.add(Helper.parseAddress(addr));
        }

        // check
        String errMsg = "Expected: " + addrLst + "\n" + "Actual: " + addrSet;
        for (String addr : addrLst) {
            Assert.assertTrue(errMsg, addrSet.contains(addr));
        }
        for (String addr : addrSet) {
            Assert.assertTrue(errMsg, addrLst.contains(addr));
        }
    }
}
