package com.sequoiadb.test.datasource;

import com.sequoiadb.base.*;
import com.sequoiadb.datasource.*;
import com.sequoiadb.datasource.SequoiadbDatasource;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.test.common.Constants;
import com.sequoiadb.test.common.Helper;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.junit.*;

import java.io.IOException;
import java.util.*;
import java.util.concurrent.atomic.AtomicLong;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;

public class SequoiadbDatasourceTest {
    private SequoiadbDatasource ds;
    List<String> coords = new ArrayList<String>();

    @BeforeClass
    public static void setConnBeforeClass() throws Exception {

    }

    @AfterClass
    public static void DropConnAfterClass() throws Exception {
    }

    @Before
    public void setUp() throws Exception {
        coords.add(Constants.COOR_NODE_CONN);

        try {
            ds = new SequoiadbDatasource(coords, "", "", null, (DatasourceOptions) null);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    @After
    public void tearDown() throws Exception {
        ds.close();
    }

    @Test
    public void setSessionAttrInDatasource() {
        int threadCount = 50;
        int maxCount = 50;
        ConfigOptions configOptions = new ConfigOptions();
        DatasourceOptions options = new DatasourceOptions();
        options.setMaxCount(maxCount);
        options.setPreferedInstance(Arrays.asList("M", "m", "1", "2", "012"));
        options.setPreferedInstanceMode("ordered");
        options.setSessionTimeout(100);
        SequoiadbDatasource sds = new SequoiadbDatasource(coords, "", "", configOptions, options);
        Sequoiadb[] dbs = new Sequoiadb[threadCount];
        for(int i = 0; i < threadCount; i++) {
            try {
                dbs[i] = sds.getConnection();
            } catch (Exception e) {
                System.out.println("i is: " + i);
                e.printStackTrace();
                Assert.assertFalse(true);
            }
        }
        for(int i = 0; i < threadCount; i++) {
            sds.releaseConnection(dbs[i]);
        }
        sds.close();
    }

    @Test
    @Ignore
    public void jira2136_transactionRollback() throws InterruptedException {
        DatasourceOptions dsOpts = new DatasourceOptions();
        dsOpts.setMaxCount(1);
        dsOpts.setDeltaIncCount(1);
        dsOpts.setMaxIdleCount(1);
        ds.updateDatasourceOptions(dsOpts);
        Sequoiadb db = ds.getConnection();
        String csName = "jira2136";
        String clName = "jira2136";
        CollectionSpace cs = Helper.getOrCreateCollectionSpace(db, csName, null);
        DBCollection cl = Helper.getOrCreateCollection(cs, clName, new BasicBSONObject("ReplSize", 0));
        db.beginTransaction();
        cl.insert(new BasicBSONObject("a", 1));
        ds.releaseConnection(db);
        db = ds.getConnection();
        cl = db.getCollectionSpace(csName).getCollection(clName);
        long recordCount = cl.getCount();
        Assert.assertEquals(0, recordCount);
        db.dropCollectionSpace(csName);
        ds.releaseConnection(db);
    }

    /*
     * connect one
     * */
    @Test
    public void testConnectOne() throws BaseException, InterruptedException {
        Sequoiadb sdb = ds.getConnection();
        CollectionSpace cs;
        if (sdb.isCollectionSpaceExist("ds")) {
            sdb.dropCollectionSpace("ds");
            cs = sdb.createCollectionSpace("ds");
        } else {
            cs = sdb.createCollectionSpace("ds");
        }
        BSONObject conf = new BasicBSONObject();
        conf.put("ReplSize", 0);
        DBCollection cl = cs.createCollection("ds", conf);

        BSONObject obj = new BasicBSONObject();
        obj.put("Id", 10);
        obj.put("Age", 30);

        cl.insert(obj);

        DBCursor cursor = cl.query();
        int i = 0;
        while (cursor.hasNext()) {
            BSONObject record = cursor.getNext();
            System.out.print(record);
            i++;
        }
        assertEquals(1, i);

        sdb.dropCollectionSpace("ds");
        ds.releaseConnection(sdb);
    }

    static AtomicLong l = new AtomicLong(0);

    class ReleaseResourceTestTask implements Runnable {
        Random random = new Random();
        SequoiadbDatasource _ds;

        ReleaseResourceTestTask(SequoiadbDatasource ds) {
            _ds = ds;
        }

        @Override
        public void run() {
            while (true) {
                Sequoiadb sdb = null;
                try {
                    sdb = _ds.getConnection();
                    System.out.println("thread:" + Thread.currentThread().getName() + ", ok - " + l.getAndAdd(1));
                    try {
                        Thread.sleep(random.nextInt(10 * 1000));
                    } catch (InterruptedException e) {
                    }
                } catch (Exception e) {
                    e.printStackTrace();
                }
                if (_ds != null) {
                    int abnormalAddrCount = _ds.getAbnormalAddrNum();
                    int normalAddrCount = _ds.getNormalAddrNum();
                    System.out.println("normal address count is: " + normalAddrCount +
                            ", abnormal address count is: " + abnormalAddrCount);
                }
                if (sdb != null) {
                    _ds.releaseConnection(sdb);
                }
            }
        }
    }

    @Test
    @Ignore
    public void jira_2797_releaseResourceTest() throws InterruptedException {
        List<String> list = new ArrayList<String>();
        list.add("192.168.20.166:11810");
        list.add("192.168.20.166:50000");
        DatasourceOptions options = new DatasourceOptions();
        options.setConnectStrategy(ConnectStrategy.SERIAL);
        options.setMaxCount(40);
        options.setCheckInterval(30 * 1000);
        options.setMaxIdleCount(10);
        options.setValidateConnection(true);
        SequoiadbDatasource ds = new SequoiadbDatasource(list, "", "", null, options);

        int threadCount = 50;
        Thread[] threads = new Thread[threadCount];
        for (int i = 0; i < threadCount; i++) {
            threads[i] = new Thread(new ReleaseResourceTestTask(ds), "" + i);
        }
        for (int i = 0; i < threadCount; i++) {
            threads[i].start();
        }
        for (int i = 0; i < threadCount; i++) {
            threads[i].join();
        }
        try {
            Thread.sleep(300 * 1000);
        } catch (InterruptedException e) {
        }
    }

    @Test
    public void jira_2863_missing_a_connection() throws IOException {
        ArrayList<Sequoiadb> dbs = new ArrayList<Sequoiadb>();
        int poolSize = 0;
        try {
            DatasourceOptions options = new DatasourceOptions();
            options.setConnectStrategy(ConnectStrategy.BALANCE);
            SequoiadbDatasource datasource = new SequoiadbDatasource(coords, "", "", null, options);
            options = (DatasourceOptions) datasource.getDatasourceOptions();
            poolSize = options.getMaxCount();
            for (int i = 0; i < poolSize; ++i) {
                Sequoiadb db = datasource.getConnection();
                Assert.assertEquals(db.isValid(), true);
                dbs.add(db);
            }
            System.out.println("go on.");
            for (Sequoiadb db : dbs) {
                datasource.releaseConnection(db);
            }
        } catch (InterruptedException e) {
            System.out.println("current get connection number " + dbs.size());
            e.printStackTrace();
            assertFalse(e.getMessage(), true);
        } catch (BaseException e) {
            System.out.println("current get connection number " + dbs.size());
            e.printStackTrace();
            throw e;
        }
    }

    @Test
    @Ignore
    public void getConnectionsPerformanceTesting() throws InterruptedException {
        String addr = Constants.COOR_NODE_CONN;
        int connNum = 500;

        Sequoiadb[] dbs = new Sequoiadb[connNum];
        long beginTime = System.currentTimeMillis();
        for (int i = 0; i < connNum; i++) {
            dbs[i] = new Sequoiadb(addr, "", "");
        }
        long endTime = System.currentTimeMillis();
        System.out.println(String.format("create connections directly takes: %dms", endTime - beginTime));

        List<String> coords = new ArrayList<String>();
        coords.add(Constants.COOR_NODE_CONN);
        DatasourceOptions options = new DatasourceOptions();
        SequoiadbDatasource datasource = new SequoiadbDatasource(coords, "", "", null, options);
        beginTime = System.currentTimeMillis();
        for (int i = 0; i < connNum; i++) {
            dbs[i] = datasource.getConnection();
        }
        endTime = System.currentTimeMillis();
        System.out.println(String.format("get connetions from data source takes: %dms", endTime - beginTime));
        for (int i = 0; i < connNum; i++) {
            datasource.releaseConnection(dbs[i]);
        }
        datasource.close();

        options = new DatasourceOptions();
        options.setMaxIdleCount(options.getMaxCount());
        datasource = new SequoiadbDatasource(coords, "", "", null, options);
        for (int i = 0; i < connNum; i++) {
            dbs[i] = datasource.getConnection();
        }
        for (int i = 0; i < connNum; i++) {
            datasource.releaseConnection(dbs[i]);
        }
        Assert.assertEquals(connNum, datasource.getIdleConnNum());
        beginTime = System.currentTimeMillis();
        for (int i = 0; i < connNum; i++) {
            dbs[i] = datasource.getConnection();
        }
        endTime = System.currentTimeMillis();
        System.out.println(String.format("get connetions from data source cache takes: %dms", endTime - beginTime));
        for (int i = 0; i < connNum; i++) {
            datasource.releaseConnection(dbs[i]);
        }
        datasource.close();

    }

}
