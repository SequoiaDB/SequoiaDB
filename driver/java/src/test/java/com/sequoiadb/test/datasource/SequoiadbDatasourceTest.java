package com.sequoiadb.test.datasource;

import com.sequoiadb.base.*;
import com.sequoiadb.datasource.*;
import com.sequoiadb.datasource.SequoiadbDatasource;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
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

    private void runQuery(Sequoiadb sdb) {
        sdb.isCollectionSpaceExist("aaa");
    }

    @Test
    public void setSessionAttrInDatasource() {
        int threadCount = 50;
        int maxCount = 50;
        ConfigOptions configOptions = new ConfigOptions();
        DatasourceOptions options = new DatasourceOptions();
        options.setMaxCount(maxCount);
        options.setPreferredInstance(Arrays.asList("M", "1", "2", "012"));
        options.setPreferredInstanceMode("ordered");
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

    // jira-2136
    @Test
    public void jira2136_transactionRollback() throws InterruptedException {
        DatasourceOptions dsOpts = new DatasourceOptions();
        dsOpts.setMaxCount(1);
        dsOpts.setDeltaIncCount(1);
        dsOpts.setMaxIdleCount(1);
        dsOpts.setMinIdleCount(1);
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
        list.add("192.168.30.166:11810");
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
//            List<String> coords = new ArrayList<String>();
//            coords.add("rhel64-test9:11810");
//            coords.add("192.168.20.166:50000");
//            coords.add("192.168.20.166:51000");
//            coords.add("192.168.20.166:52000");
            DatasourceOptions options = new DatasourceOptions();
            options.setConnectStrategy(ConnectStrategy.BALANCE);
            SequoiadbDatasource datasource = new SequoiadbDatasource(coords, "", "", null, options);
            options = (DatasourceOptions) datasource.getDatasourceOptions();
            poolSize = options.getMaxCount();
            //申请到池满
            for (int i = 0; i < poolSize; ++i) {
                Sequoiadb db = datasource.getConnection();
                Assert.assertEquals(db.isValid(), true);
                dbs.add(db);
            }
//            System.out.println(String.format("Total: %d, create directly: %d", poolSize, datasource.aInt.get()));
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

        // case 1: create connection directly
        Sequoiadb[] dbs = new Sequoiadb[connNum];
        long beginTime = System.currentTimeMillis();
        for (int i = 0; i < connNum; i++) {
            dbs[i] = new Sequoiadb(addr, "", "");
        }
        long endTime = System.currentTimeMillis();
        System.out.println(String.format("create connections directly takes: %dms", endTime - beginTime));

        // case 2: get connections from data source
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
        // release connections
        for (int i = 0; i < connNum; i++) {
            datasource.releaseConnection(dbs[i]);
        }
        datasource.close();

        // case 3:
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
        // release connections
        for (int i = 0; i < connNum; i++) {
            datasource.releaseConnection(dbs[i]);
        }
        datasource.close();

    }

    /*
    * check:
    * 1. datasource keep the idle connection num or not
    * */
    @Test
    @Ignore
    public void jira_6721_features1() {
        DatasourceOptions dsOpt = new DatasourceOptions();
        dsOpt.setCheckInterval(10 * 1000);
        SequoiadbDatasource datasource = new SequoiadbDatasource(coords, "", "", null, dsOpt);
        Sequoiadb sdb = null;
        int sleepTime = 240 * 1000;
        try {
            try {
                sdb = datasource.getConnection();
            } finally {
                datasource.releaseConnection(sdb);
            }
        } catch (InterruptedException e) {
            e.printStackTrace();
            Assert.fail();
        }
        while (sleepTime > 0) {
            int interval = 5 * 1000;
            try {
                Thread.sleep(interval);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
            Assert.assertTrue("datasource.getIdleConnNum(): " + datasource.getIdleConnNum() +
                    ", dsOpt.getMinIdleCount(): " + dsOpt.getMinIdleCount(),datasource.getIdleConnNum() >= dsOpt.getMinIdleCount());
            sleepTime -= interval;
        }
        Assert.assertTrue("datasource.getIdleConnNum(): " + datasource.getIdleConnNum() +
                ", dsOpt.getMaxIdleCount(): " + dsOpt.getMaxIdleCount(),datasource.getIdleConnNum() <= dsOpt.getMaxIdleCount());
    }

    /*
     * check:
     * 1. cacheLimit works or not
     * 2. background thread create connection or not(minIdleCount == maxIdleCount)
     * */
    @Test
    @Ignore
    public void jira_6721_features2() {
        DatasourceOptions dsOpt = new DatasourceOptions();
        dsOpt.setCheckInterval(10 * 1000);
        dsOpt.setCacheLimit(10);
        SequoiadbDatasource datasource = new SequoiadbDatasource(coords, "", "", null, dsOpt);
        List<Sequoiadb> sdbList = new ArrayList<Sequoiadb>();
        int sleepTime = 240 * 1000;
        try {
            int i = 0;
            try {
                while(i < dsOpt.getMaxCount()) {
                    sdbList.add(datasource.getConnection());
                    i++;
                }
            } finally {
                while (sdbList.size() > 0) {
                    runQuery(sdbList.get(0));
                    datasource.releaseConnection(sdbList.get(0));
                    sdbList.remove(0);
                }
            }
            Assert.assertTrue("datasource.getIdleConnNum(): " + datasource.getIdleConnNum(),
                    datasource.getIdleConnNum() == 0);
        } catch (InterruptedException e) {
            e.printStackTrace();
            Assert.fail();
        }
        while (sleepTime > 0) {
            int interval = 15 * 1000;
            try {
                Thread.sleep(interval);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
            Assert.assertTrue("datasource.getIdleConnNum(): " + datasource.getIdleConnNum() +
                    ", dsOpt.getMinIdleCount(): " + dsOpt.getMinIdleCount(),datasource.getIdleConnNum() >= dsOpt.getMinIdleCount());
            sleepTime -= interval;
        }
        Assert.assertTrue("datasource.getIdleConnNum(): " + datasource.getIdleConnNum() +
                ", dsOpt.getMaxIdleCount(): " + dsOpt.getMaxIdleCount(),datasource.getIdleConnNum() <= dsOpt.getMaxIdleCount());
    }

    /*
     * check:
     * 1. cacheLimit works or not
     * 2. background thread create connection or not(minIdleCount != maxIdleCount)
     * */
    @Test
    @Ignore
    public void jira_6721_features3() {
        DatasourceOptions dsOpt = new DatasourceOptions();
        dsOpt.setCheckInterval(10 * 1000);
        dsOpt.setCacheLimit(10);
        dsOpt.setMaxIdleCount(20);
        dsOpt.setMinIdleCount(10);
        SequoiadbDatasource datasource = new SequoiadbDatasource(coords, "", "", null, dsOpt);
        List<Sequoiadb> sdbList = new ArrayList<Sequoiadb>();
        int sleepTime = 240 * 1000;
        try {
            int i = 0;
            try {
                while(i < dsOpt.getMaxCount()) {
                    sdbList.add(datasource.getConnection());
                    i++;
                }
            } finally {
                while (sdbList.size() > 0) {
                    runQuery(sdbList.get(0));
                    datasource.releaseConnection(sdbList.get(0));
                    sdbList.remove(0);
                }
            }
            Assert.assertTrue("datasource.getIdleConnNum(): " + datasource.getIdleConnNum(),
                    datasource.getIdleConnNum() == 0);
        } catch (InterruptedException e) {
            e.printStackTrace();
            Assert.fail();
        }
        while (sleepTime > 0) {
            int interval = 15 * 1000;
            try {
                Thread.sleep(interval);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
            Assert.assertTrue("datasource.getIdleConnNum(): " + datasource.getIdleConnNum() +
                    ", dsOpt.getMinIdleCount(): " + dsOpt.getMinIdleCount(),datasource.getIdleConnNum() >= dsOpt.getMinIdleCount());
            sleepTime -= interval;
        }
        Assert.assertTrue("datasource.getIdleConnNum(): " + datasource.getIdleConnNum() +
                ", dsOpt.getMaxIdleCount(): " + dsOpt.getMaxIdleCount(),datasource.getIdleConnNum() <= dsOpt.getMaxIdleCount());
    }

    /*
     * check:
     * 1. cacheLimit is set to 0, test it works or not. connection come back to the pool will not destroy.
     * 2. background thread destroy connection or not
     * */
    @Test
    @Ignore
    public void jira_6721_features4() {
        DatasourceOptions dsOpt = new DatasourceOptions();
        dsOpt.setCheckInterval(10 * 1000);
        dsOpt.setCacheLimit(0);
        dsOpt.setMaxIdleCount(20);
        dsOpt.setMinIdleCount(10);
        SequoiadbDatasource datasource = new SequoiadbDatasource(coords, "", "", null, dsOpt);
        List<Sequoiadb> sdbList = new ArrayList<Sequoiadb>();
        try {
            int i = 0;
            try {
                while(i < dsOpt.getMaxCount()) {
                    sdbList.add(datasource.getConnection());
                    i++;
                }
            } finally {
                while (sdbList.size() > 0) {
                    runQuery(sdbList.get(0));
                    datasource.releaseConnection(sdbList.get(0));
                    sdbList.remove(0);
                }
            }
            Assert.assertTrue("datasource.getIdleConnNum(): " + datasource.getIdleConnNum(),
                    datasource.getIdleConnNum() == dsOpt.getMaxCount());
        } catch (InterruptedException e) {
            e.printStackTrace();
            Assert.fail();
        }
        int sleepTime = 240 * 1000;
        while (sleepTime > 0) {
            int interval = 15 * 1000;
            try {
                Thread.sleep(interval);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
            Assert.assertTrue("datasource.getIdleConnNum(): " + datasource.getIdleConnNum() +
                    ", dsOpt.getMinIdleCount(): " + dsOpt.getMinIdleCount(),datasource.getIdleConnNum() >= dsOpt.getMinIdleCount());
            sleepTime -= interval;
        }
        Assert.assertTrue("datasource.getIdleConnNum(): " + datasource.getIdleConnNum() +
                ", dsOpt.getMaxIdleCount(): " + dsOpt.getMaxIdleCount(),datasource.getIdleConnNum() <= dsOpt.getMaxIdleCount());
    }

    /*
    * test:
    * 1. minIdleCount value test
    * */
    @Test
    public void jira_6721_features5() {
        SequoiadbDatasource datasource;
        DatasourceOptions dsOpt = new DatasourceOptions();
        dsOpt.setCheckInterval(10 * 1000);
        dsOpt.setCacheLimit(10);

        // case 1: less than 0
        try {
            dsOpt.setMinIdleCount(-1);
            datasource = new SequoiadbDatasource(coords, "", "", null, dsOpt);
            Assert.fail();
        } catch (BaseException e) {
            Assert.assertEquals(e.getErrorCode(), SDBError.SDB_INVALIDARG.getErrorCode());
        }

        // case 2: max than maxIdleCount
        try {
            dsOpt.setMinIdleCount(11);
            datasource = new SequoiadbDatasource(coords, "", "", null, dsOpt);
            Assert.fail();
        } catch (BaseException e) {
            Assert.assertEquals(e.getErrorCode(), SDBError.SDB_INVALIDARG.getErrorCode());
        }

        // case 3: max than maxCount
        try {
            dsOpt.setMinIdleCount(501);
            dsOpt.setMaxIdleCount(501);
            datasource = new SequoiadbDatasource(coords, "", "", null, dsOpt);
            Assert.fail();
        } catch (BaseException e) {
            Assert.assertEquals(e.getErrorCode(), SDBError.SDB_INVALIDARG.getErrorCode());
        }

        // case 4: equal to 0
        dsOpt.setMinIdleCount(0);
        dsOpt.setMaxIdleCount(10);
        datasource = new SequoiadbDatasource(coords, "", "", null, dsOpt);
        datasource.close();

        // case 5: equal to maxIdleCount
        dsOpt.setMinIdleCount(100);
        dsOpt.setMaxIdleCount(100);
        datasource = new SequoiadbDatasource(coords, "", "", null, dsOpt);
        datasource.close();

        // case 6: equal to maxCount
        dsOpt.setMinIdleCount(500);
        dsOpt.setMaxIdleCount(500);
        datasource = new SequoiadbDatasource(coords, "", "", null, dsOpt);
        datasource.close();
    }

    @Test
    public void preferredInstanceTest(){
        String mode = "random";
        String instance = "A";
        List<String> instanceList = new ArrayList<>();
        instanceList.add( instance );

        DatasourceOptions options = new DatasourceOptions();
        options.setPreferredInstanceMode( mode );
        options.setPreferredInstance( instanceList );

        Assert.assertEquals( mode, options.getPreferredInstanceMode() );
        Assert.assertEquals( instanceList, options.getPreferredInstance() );

        SequoiadbDatasource ds = new SequoiadbDatasource( coords, "", "", null, options );
        Sequoiadb db = null;
        try {
            db = ds.getConnection();
            BSONObject obj = db.getSessionAttr( false );
            Assert.assertEquals( mode, obj.get( "PreferredInstanceMode" ) );
            Assert.assertEquals( instance, obj.get( "PreferredInstance" ) );
        } catch ( Exception e ){
            e.printStackTrace();
        } finally {
            if ( db != null ){
                ds.releaseConnection( db );
            }
            ds.close();
        }
    }

    @Test
    public void builderTest() {
        DatasourceOptions dsOpt = new DatasourceOptions();
        dsOpt.setMinIdleCount( 10 );
        dsOpt.setMaxIdleCount( 20 );

        ConfigOptions netOpt = new ConfigOptions();
        netOpt.setConnectTimeout( 1000 ); // 1s
        netOpt.setMaxAutoConnectRetryTime( 0 );

        // case 1: address and datasourceOptions
        try {
            SequoiadbDatasource ds = SequoiadbDatasource.builder()
                    .serverAddress( ( String ) null )
                    .build();
        } catch ( BaseException e ){
            Assert.assertEquals( SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode() );
        }

        List<String> addressList = new ArrayList<>();
        addressList.add( "127.0.0.1:11810" );
        addressList.add( "127.0.0.1:11820" );
        addressList.add( "" );
        addressList.add( Constants.COOR_NODE_CONN );

        SequoiadbDatasource ds1 = SequoiadbDatasource.builder()
                .serverAddress( addressList )
                .configOptions( netOpt )
                .datasourceOptions( dsOpt )
                .build();
        try {
            checkDataSource( ds1 );
            Assert.assertEquals( 3, ds1.getNormalAddrNum() + ds1.getAbnormalAddrNum() );
            DatasourceOptions resultOpt = ds1.getDatasourceOptions();
            Assert.assertEquals( resultOpt.getMinIdleCount(), dsOpt.getMinIdleCount());
            Assert.assertEquals( resultOpt.getMaxIdleCount(), dsOpt.getMaxIdleCount());
        } finally {
            ds1.close();
        }

        // case 2: user
        try {
            Sequoiadb db = ds.getConnection();
            try {
                db.createUser( Constants.TEST_USER_NAME, Constants.TEST_USER_PASSWORD );

                // user name and password
                SequoiadbDatasource ds2 = SequoiadbDatasource.builder()
                        .userConfig( new UserConfig( Constants.TEST_USER_NAME, Constants.TEST_USER_PASSWORD ) )
                        .serverAddress( Constants.COOR_NODE_CONN )
                        .configOptions( netOpt )
                        .datasourceOptions( dsOpt )
                        .build();
                try {
                    checkDataSource( ds2 );
                }finally {
                    ds2.close();
                }
            } finally {
                db.removeUser( Constants.TEST_USER_NAME, Constants.TEST_USER_PASSWORD );
            }
            ds.releaseConnection( db );
        } catch ( InterruptedException e ) {
            e.printStackTrace();
        }
    }

    private void checkDataSource( SequoiadbDatasource ds ) {
        try {
            Sequoiadb db = ds.getConnection();
            Assert.assertTrue( db.isValid() );
            ds.releaseConnection( db );
        } catch ( InterruptedException e ) {
            e.printStackTrace();
        }
    }
}
