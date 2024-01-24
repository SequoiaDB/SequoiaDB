package com.sequoiadb.test.datasource;

import com.sequoiadb.base.ConfigOptions;
import com.sequoiadb.base.Node;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.datasource.ConnectStrategy;
import com.sequoiadb.datasource.DatasourceOptions;
import com.sequoiadb.datasource.SequoiadbDatasource;
import com.sequoiadb.test.common.Constants;
import com.sequoiadb.util.Helper;
import org.junit.*;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;

public class ConcurrencyTest {
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
    private static AtomicInteger connNum;
    private static AtomicBoolean isSuccess;
    private SequoiadbDatasource ds;
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
        options.setMaxCount(50);
        options.setMinIdleCount(5);
        options.setMaxIdleCount(10);
        options.setCheckInterval(100);
        options.setSyncCoordInterval(100);
        options.setSyncLocationInterval(100);
        options.setConnectStrategy(ConnectStrategy.SERIAL);

        coordRG.start();

        connNum = new AtomicInteger(0);
        isSuccess = new AtomicBoolean(true);
    }

    @After
    public void tearDown() {
        if (ds != null) {
            ds.close();
        }
    }

    @Test
    public void test() throws Exception {
        List<String> addressList = new ArrayList<>();
        addressList.add(node1.getNodeName());
        addressList.add(node2.getNodeName());
        addressList.add(node3.getNodeName());
        addressList.add(node4.getNodeName());
        addressList.add(node5.getNodeName());

        ds = SequoiadbDatasource.builder()
                .serverAddress(addressList)
                .configOptions(netConfig)
                .datasourceOptions(options)
                .build();

        // enable
        concurrencyTest(ds, null);

        // disable
        ds.disableDatasource();
        concurrencyTest(ds, null);
    }

    @Test
    public void locationTest() throws Exception {
        String location1 = "guangzhou.nansha";
        String location2 = "guangzhou";
        String location3 = "guangzhou.fanyu";
        String location4 = "shanghai";
        String location5 = "";

        node1.setLocation(location1);
        node2.setLocation(location2);
        node3.setLocation(location3);
        node4.setLocation(location4);
        node5.setLocation(location5);

        List<String> addressList = new ArrayList<>();
        addressList.add(node1.getNodeName());
        addressList.add(node2.getNodeName());
        addressList.add(node3.getNodeName());
        addressList.add(node4.getNodeName());
        addressList.add(node5.getNodeName());

        ds = SequoiadbDatasource.builder()
                .serverAddress(addressList)
                .location(location1)
                .configOptions(netConfig)
                .datasourceOptions(options)
                .build();

        // enable
        concurrencyTest(ds, location1);

        // disable
        ds.disableDatasource();
        concurrencyTest(ds, location1);
    }

    public void concurrencyTest(SequoiadbDatasource ds, String location) throws Exception {
        int threadNum = 5;
        int cycleTime = 100;
        int num = ds.getDatasourceOptions().getMaxCount() / threadNum;
        int sleepTime = 100; // 100ms

        List<Worker> workerList = new ArrayList<>();
        for (int i = 0; i < threadNum; i++) {
            workerList.add(new ConnWorker(ds, node2.getNodeName(), num, cycleTime));
        }
        workerList.add(new AddressWorker(ds, node1.getNodeName(), sleepTime));
        workerList.add(new NodeWorker(node2.getNodeName(), sleepTime));
        if (location != null) {
            workerList.add(new LocationWorker(node3.getNodeName(), location, sleepTime));
        }

        for (Worker worker: workerList) {
            worker.start();
        }
        for (Worker worker: workerList) {
            worker.join();
        }

        Assert.assertTrue("Concurrency test Failed", isSuccess.get());
    }

    static abstract class Worker extends Thread {
        final SequoiadbDatasource ds;
        final String nodeName;
        final int sleepTime;

        Worker(SequoiadbDatasource ds, String nodeName, int sleepTime) {
            this.ds = ds;
            this.nodeName = nodeName;
            this.sleepTime = sleepTime;
        }

        @Override
        public void run() {
            try {
                doWork();
            } catch (Exception e) {
                isSuccess.set(false);
                throw new RuntimeException(e);
            }
        }

        abstract void doWork() throws Exception;
    }

    static class ConnWorker extends Worker {
        private final int num;
        private final int cycleTime;

        ConnWorker(SequoiadbDatasource ds, String nodeName, int num, int cycleTime) {
            super(ds, nodeName, 0);
            this.num = num;
            this.cycleTime = cycleTime;
        }

        void doWork() throws Exception {
            connNum.incrementAndGet();
            try {
                List<Sequoiadb> connList = new ArrayList<>();
                for (int i = 0; i < cycleTime; i++) {
                    try {
                        for (int j = 0; j < num; j++) {
                            Sequoiadb db = ds.getConnection();
                            // The node maybe stop
                            if (!db.isValid()) {
                                Assert.assertEquals(Helper.parseAddress(nodeName), db.getNodeName());
                            }
                            connList.add(db);
                        }
                    } finally {
                        for (Sequoiadb db : connList) {
                            ds.releaseConnection(db);
                        }
                        connList.clear();
                    }
                }
            } finally {
                connNum.decrementAndGet();
            }
        }
    }

    static class AddressWorker extends Worker {
        AddressWorker(SequoiadbDatasource ds, String nodeName, int sleepTime) {
            super(ds, nodeName, sleepTime);
        }

        void doWork() throws Exception {
            Thread.sleep(sleepTime);
            ds.addCoord(nodeName);
            Thread.sleep(sleepTime);
            ds.removeCoord(nodeName);
        }
    }

    static class NodeWorker extends Worker {
        Sequoiadb db;
        Node node;

        NodeWorker(String nodeName, int sleepTime) {
            super(null, nodeName, sleepTime);
            db = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");
            node = db.getReplicaGroup("SYSCoord").getNode(nodeName);
        }

        void doWork() throws Exception {
            try {
                Thread.sleep(sleepTime);
                node.start();
                Thread.sleep(sleepTime);
                node.stop();
            } finally {
                db.close();
            }
        }
    }

    static class LocationWorker extends Worker {
        Sequoiadb db;
        Node node;
        String location;

        LocationWorker(String nodeName, String location, int sleepTime) {
            super(null, nodeName, sleepTime);
            this.location = location;
            db = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");
            node = db.getReplicaGroup("SYSCoord").getNode(nodeName);
        }

        void doWork() throws Exception {
            try {
                Thread.sleep(sleepTime);
                node.setLocation("");
                Thread.sleep(sleepTime);
                node.setLocation(location);
            } finally {
                db.close();
            }
        }
    }
}