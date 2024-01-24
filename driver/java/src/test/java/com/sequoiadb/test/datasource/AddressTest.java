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
import java.util.List;
import java.util.TreeSet;

public class AddressTest {
    private final static int PORT1 = 51000;
    private final static int PORT2 = 52000;
    private final static int PORT3 = 53000;
    private final static int PORT4 = 54000;
    private final static int PORT5 = 55000;
    private static Sequoiadb db;
    private static ReplicaGroup coordRG;
    private static Node node1;
    private static Node node2;
    private static Node node3;
    private static Node node4;
    private static Node node5;
    private SequoiadbDatasource ds;
    private String normalAddress;
    private String abnormalAddress;
    private ConfigOptions netConfig;
    private DatasourceOptions options;

    @BeforeClass
    public static void prepareBeforeClass() {
        db = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");
        coordRG = db.getReplicaGroup("SYSCoord");

        String dbPath = Constants.DB_PATH + "/coord/";
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
            try {
                coordRG.removeNode(Constants.HOST, PORT5, null);
            } catch (BaseException e) {
                if (e.getErrorCode() != SDBError.SDB_CLS_NODE_NOT_EXIST.getErrorCode()) {
                    throw e;
                }
            }
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
        options.setConnectStrategy(ConnectStrategy.SERIAL);

        coordRG.start();
        node2.stop();

        normalAddress = Constants.HOST + ":" + PORT1;
        abnormalAddress = Constants.HOST + ":" + PORT2;
    }

    @After
    public void tearDown() {
        if (ds != null) {
            ds.close();
        }
    }

    @Test
    public void addressTest() throws Exception {
        // error address
        createDSByErrorAddress("");
        createDSByErrorAddress(null);
        createDSByErrorAddress("errorHost");
        createDSByErrorAddress("::");
        createDSByErrorAddress("errorHost:");
        createDSByErrorAddress(Constants.HOST + ":errorPort");

        // error address list
        List<String> list1 = new ArrayList<>();
        list1.add("");
        list1.add(null);
        try {
            ds = SequoiadbDatasource.builder()
                    .serverAddress(list1)
                    .build();
            Assert.fail("Can't builder ds with error address");
        } catch (BaseException e) {
            Assert.assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode());
        }

        // normal address
        List<String> list2 = new ArrayList<>();
        list2.add(normalAddress);
        list2.add(" " + Constants.HOST + "  :  " + PORT1 + " "); // repeat address with normalAddress
        list2.add(abnormalAddress);

        ds = createDSByAddress(list2);
        Assert.assertEquals(1, ds.getNormalAddrNum());
        Assert.assertEquals(1, ds.getAbnormalAddrNum());
    }

    private void createDSByErrorAddress(String address) {
        try {
            ds = SequoiadbDatasource.builder()
                    .serverAddress(address)
                    .build();
            Assert.fail("Can't builder ds with error address");
        } catch (BaseException e) {
            Assert.assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode());
        }
    }

    private SequoiadbDatasource createDSByAddress(List<String> addressList) throws Exception {
        SequoiadbDatasource ds = SequoiadbDatasource.builder()
                .serverAddress(addressList)
                .configOptions(netConfig)
                .datasourceOptions(options)
                .build();

        List<Sequoiadb> connList = new ArrayList<>();
        for (int i = 0; i < options.getMaxCount(); i++) {
            connList.add(ds.getConnection());
        }
        for (Sequoiadb conn : connList) {
            ds.releaseConnection(conn);
        }
        return ds;
    }

    @Test
    public void addAddressTest() throws Exception {
        ds = SequoiadbDatasource.builder()
                .serverAddress(Constants.COOR_NODE_CONN)
                .configOptions(netConfig)
                .datasourceOptions(options)
                .build();
        Assert.assertEquals(1, ds.getNormalAddrNum());

        // error address
        addErrorAddress(ds, "");
        addErrorAddress(ds, null);

        // repeat address
        ds.addCoord(Constants.COOR_NODE_CONN);
        Assert.assertEquals(1, ds.getNormalAddrNum());

        // normal address
        ds.addCoord(normalAddress);
        Assert.assertEquals(2, ds.getNormalAddrNum());

        // abnormal address
        ds.addCoord(abnormalAddress);
        List<Sequoiadb> connList = new ArrayList<>();
        for (int i = 0; i < options.getMaxCount(); i++) {
            connList.add(ds.getConnection());
        }
        for (Sequoiadb conn : connList) {
            ds.releaseConnection(conn);
        }
        Assert.assertEquals(2, ds.getNormalAddrNum());
        Assert.assertEquals(1, ds.getAbnormalAddrNum());
    }

    private void addErrorAddress(SequoiadbDatasource ds, String address) {
        try {
            ds.addCoord(address);
            Assert.fail("Can't add error address");
        } catch (BaseException e) {
            Assert.assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode());
        }
    }

    @Test
    public void removeAddressTest() throws Exception {
        List<String> addressList = new ArrayList<>();
        addressList.add(normalAddress);
        addressList.add(abnormalAddress);

        ds = createDSByAddress(addressList);
        Assert.assertEquals(1, ds.getNormalAddrNum());
        Assert.assertEquals(1, ds.getAbnormalAddrNum());

        // error address
        removeErrorAddress(ds, "");
        removeErrorAddress(ds, null);

        ds.removeCoord("127.0.0.1:123");
        Assert.assertEquals(1, ds.getNormalAddrNum());
        Assert.assertEquals(1, ds.getAbnormalAddrNum());

        // normal address
        ds.removeCoord(normalAddress);
        Assert.assertEquals(0, ds.getNormalAddrNum());

        // abnormal address
        ds.removeCoord(abnormalAddress);
        Assert.assertEquals(0, ds.getAbnormalAddrNum());
    }

    private void removeErrorAddress(SequoiadbDatasource ds, String address) {
        try {
            ds.removeCoord(address);
            Assert.fail("Can't remove error address");
        } catch (BaseException e) {
            Assert.assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode());
        }
    }

    @Test
    public void retryAddressTest() throws Exception {
        List<String> addressList = new ArrayList<>();
        addressList.add(normalAddress);
        addressList.add(abnormalAddress);

        ds = createDSByAddress(addressList);
        Assert.assertEquals(1, ds.getNormalAddrNum());
        Assert.assertEquals(1, ds.getAbnormalAddrNum());

        coordRG.start();
        // Waite RetrieveAddressTask run
        Thread.sleep( 61 * 1000);

        Assert.assertEquals(2, ds.getNormalAddrNum());
        Assert.assertEquals(0, ds.getAbnormalAddrNum());
    }

    @Test
    public void syncAddressTest() throws Exception {
        List<String> addressList = new ArrayList<>();
        addressList.add(node1.getNodeName());
        addressList.add(node2.getNodeName());
        addressList.add(node3.getNodeName());
        addressList.add(node5.getNodeName());

        ds = SequoiadbDatasource.builder()
                .configOptions(netConfig)
                .serverAddress(addressList)
                .datasourceOptions(options)
                .build();

        List<Sequoiadb> connList = new ArrayList<>();
        for (int i = 0; i < options.getMaxCount(); i++) {
            connList.add(ds.getConnection());
        }
        for (Sequoiadb conn : connList) {
            ds.releaseConnection(conn);
        }
        connList.clear();
        Assert.assertEquals(3, ds.getNormalAddrNum());
        Assert.assertEquals(1, ds.getAbnormalAddrNum());

        node1.stop();
        node2.start();
        coordRG.removeNode(node5.getHostName(), PORT5, null);
        options.setSyncCoordInterval(2 * 1000);
        ds.updateDatasourceOptions(options);

        // waite SynchronizeAddressTask run
        Thread.sleep( 3 * 1000);

        //Assert.assertEquals(4, ds.getNormalAddrNum());
        Assert.assertEquals(1, ds.getAbnormalAddrNum());
        for (int i = 0; i < options.getMaxCount(); i++) {
            connList.add(ds.getConnection());
        }
        TreeSet<String> addrSet = new TreeSet<>();
        for (Sequoiadb conn : connList) {
            addrSet.add(conn.getNodeName());
            ds.releaseConnection(conn);
        }
        Assert.assertFalse(addrSet.contains(Helper.parseAddress(node5.getNodeName())));
        Assert.assertTrue(addrSet.contains(Helper.parseAddress(node4.getNodeName())));
    }

    @Test
    public void strategyTest() throws Exception {
        List<String> addressList = new ArrayList<>();
        addressList.add(node1.getNodeName());
        addressList.add(node3.getNodeName());
        addressList.add(node4.getNodeName());

        String addr1 = Helper.parseAddress(node1.getNodeName());
        String addr2 = Helper.parseAddress(node3.getNodeName());
        String addr3 = Helper.parseAddress(node4.getNodeName());

        int maxCount = 20;
        options.setMaxCount(maxCount);
        options.setConnectStrategy(ConnectStrategy.SERIAL);
        options.setSyncCoordInterval(0);
        ds = SequoiadbDatasource.builder()
                .configOptions(netConfig)
                .serverAddress(addressList)
                .datasourceOptions(options)
                .build();

        List<Sequoiadb> connList = new ArrayList<>();
        for (int i = 0; i < maxCount; i++) {
            connList.add(ds.getConnection());
        }
        Assert.assertEquals(3, ds.getNormalAddrNum());

        int count1 = 0;
        int count2 = 0;
        int count3 = 0;
        int average = maxCount / ds.getNormalAddrNum();
        for (Sequoiadb db : connList) {
            if (db.getNodeName().equals(addr1)) {
                count1++;
            } else if (db.getNodeName().equals(addr2)) {
                count2++;
            } else if (db.getNodeName().equals(addr3)) {
                count3++;
            }
        }
        if (count1 < average || count2 < average || count3 < average) {
            Assert.fail("serial strategy not work, conn info: " + node1.getNodeName() + " = " + count1 +
                    ", " + node3.getNodeName() + " = " + count2 +
                    ", " + node4.getNodeName() + " = " + count3);
        }

        ds.removeCoord(node4.getNodeName());
        Assert.assertEquals(2, ds.getNormalAddrNum());
        for (Sequoiadb db : connList) {
            ds.releaseConnection(db);
        }
        connList.clear();

        for (int i = 0; i < maxCount; i++) {
            connList.add(ds.getConnection());
        }
        count1 = 0;
        count2 = 0;
        average = maxCount / ds.getNormalAddrNum();
        for (Sequoiadb db : connList) {
            if (db.getNodeName().equals(addr1)) {
                count1++;
            } else if (db.getNodeName().equals(addr2)) {
                count2++;
            }
            Assert.assertNotEquals(addr3, db.getNodeName());
        }
        if (count1 < average || count2 < average) {
            Assert.fail("serial strategy not work, conn info: " + node1.getNodeName() + " = " + count1 +
                    ", " + node3.getNodeName() + " = " + count2);
        }

        // update strategy
        options.setConnectStrategy(ConnectStrategy.RANDOM);
        ds.updateDatasourceOptions(options);
        for (Sequoiadb db : connList) {
            ds.releaseConnection(db);
        }

        List<Sequoiadb> connList2 = new ArrayList<>();
        for (int i = 0; i < maxCount; i++) {
            connList2.add(ds.getConnection());
        }
        for (Sequoiadb db : connList2) {
            Assert.assertFalse(connList.contains(db));
        }

        count1 = 0;
        count2 = 0;
        average = maxCount / ds.getNormalAddrNum();
        for (Sequoiadb db : connList2) {
            if (db.getNodeName().equals(addr1)) {
                count1++;
            } else if (db.getNodeName().equals(addr2)) {
                count2++;
            }
        }
        if (count1 < average || count2 < average) {
            Assert.fail("random strategy not work, conn info: " + node1.getNodeName() + " = " + count1 +
                    ", " + node3.getNodeName() + " = " + count2);
        }

        for (Sequoiadb db : connList2) {
            ds.releaseConnection(db);
        }
    }

    @Test
    public void unavailableAddressTest() throws Exception {
        node1.stop();
        // node2 had stopped
        node3.stop();

        List<String> addressList = new ArrayList<>();
        // unavailable address
        addressList.add(node1.getNodeName());
        addressList.add(node2.getNodeName());
        addressList.add(node3.getNodeName());
        // normal address
        addressList.add(node4.getNodeName());

        String nodeAddr = Helper.parseAddress(node4.getNodeName());

        // case 1: enable conn pool
        unavailableAddressWithStatusTest(true, addressList, nodeAddr);

        // case 2: disable conn pool
        unavailableAddressWithStatusTest(false, addressList, nodeAddr);
    }

    private void unavailableAddressWithStatusTest(boolean enable, List<String> addressList, String expectedAddr) throws Exception {

        int connTimeOut = 3 * 1000; // 3s
        int maxRetryTime = 3 * 1000; // 3s

        ConfigOptions netConfig = new ConfigOptions();
        netConfig.setConnectTimeout(connTimeOut);
        netConfig.setMaxAutoConnectRetryTime(maxRetryTime);

        int maxCount = 20;
        options.setMaxCount(maxCount);
        options.setConnectStrategy(ConnectStrategy.SERIAL);
        options.setSyncCoordInterval(0);

        ds = SequoiadbDatasource.builder()
                .configOptions(netConfig)
                .serverAddress(addressList)
                .datasourceOptions(options)
                .build();

        if (!enable) {
            ds.disableDatasource();
        }

        long t1 = System.currentTimeMillis();
        Sequoiadb db = ds.getConnection(2000); // 2s
        long t2 = System.currentTimeMillis();
        System.out.println("get conn time: " + (t2 - t1) + "ms");

        // check conn
        Assert.assertEquals(addressList.size(), ds.getNormalAddrNum());
        Assert.assertNotNull(db);
        Assert.assertTrue(db.isValid());
        Assert.assertEquals(expectedAddr, db.getNodeName());

        ds.releaseConnection(db);
        ds.close();
    }
}
