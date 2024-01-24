package com.sequoiadb.test.lob;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.datasource.SequoiadbDatasource;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.test.common.Constants;
import com.sequoiadb.test.common.LobHelper;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;
import org.junit.*;

import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.util.ArrayList;
import java.util.concurrent.ConcurrentLinkedQueue;

public class DBLobWriteByBufferPerformanceTest {
    private static SequoiadbDatasource ds;
    private static Sequoiadb sdb;
    private static CollectionSpace cs;
    private static DBCollection cl;
    private static String inputFileName = null;
    private static String outputFileName = null;
    private static int threadNum = 16;
    private static int bufferSize = 2048; // KB
    private static int MAX_NUM = 1024 * 1024;

    @BeforeClass
    public static void setConnBeforeClass() throws Exception {
        ds = new SequoiadbDatasource(Constants.COOR_NODE_CONN, "admin", "admin", null);
    }

    @AfterClass
    public static void DropConnAfterClass() throws Exception {
        try {
            if (ds != null) {
                ds.close();
            }
        } catch (BaseException e) {
            e.printStackTrace();
        }
    }

    @Before
    public void setUp() throws Exception {
        // sdb
        sdb = new Sequoiadb(Constants.COOR_NODE_CONN, "admin", "admin");
        // cs
        if (sdb.isCollectionSpaceExist(Constants.TEST_CS_NAME_1)) {
            sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
            cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
        } else {
            cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
        }
        // cl
        BSONObject conf = new BasicBSONObject();
        conf.put("ReplSize", 1);
        cl = cs.createCollection(Constants.TEST_CL_NAME_1, conf);
    }

    @After
    public void tearDown() throws Exception {
        try {
            if (sdb != null) {
                sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
                sdb.disconnect();
            }
        } catch (BaseException e) {
            e.printStackTrace();
        }
    }


    class WriteLobByBufferTask implements Runnable {
        SequoiadbDatasource ds = null;
        String csName = null;
        String clName = null;
        String fileName = null;
        int retryTimes = 0;
        ConcurrentLinkedQueue<Long> list = null;

        public WriteLobByBufferTask(SequoiadbDatasource ds, String clFullName,
                                    String fileName, int retryTimes, ConcurrentLinkedQueue<Long> list) {
            this.ds = ds;
            this.csName = clFullName.split("\\.")[0];
            this.clName = clFullName.split("\\.")[1];
            this.fileName = fileName;
            this.retryTimes = retryTimes;
            this.list = list;
        }

        @Override
        public void run() {
            FileInputStream fileInputStream = null;
            byte[] inBuffer = new byte[bufferSize * 1024];
            int writeLen = 0;
            long startTime = 0;
            long endTime = 0;
            long interval = 0;

            // get db/cl
            Sequoiadb db = null;
            DBCollection coll = null;

            try {
                db = ds.getConnection();
                coll = db.getCollectionSpace(csName).getCollection(clName);
            } catch (BaseException e) {
                e.printStackTrace();
                Assert.fail();
            } catch (InterruptedException e) {
                e.printStackTrace();
                Assert.fail();
            }

            // write lob
            DBLob lob = null;
            ObjectId id = null;
            ArrayList<Long> writeTimeList = new ArrayList<Long>();
            int i = 0;
            while (i++ < retryTimes) {
                if (id != null) {
                    try {
                        coll.removeLob(id);
                    } catch (BaseException e) {
                    }
                }
                try {
                    lob = coll.createLob();
                    id = lob.getID();
                    try {
                        fileInputStream = new FileInputStream(fileName);
                    } catch (FileNotFoundException e) {
                        e.printStackTrace();
                        return;
                    }
                    startTime = System.currentTimeMillis();
                    writeLen = 0;
                    try {
                        while (-1 < (writeLen = fileInputStream.read(inBuffer))) {
                            lob.write(inBuffer, 0, writeLen);
                        }
                    } catch (IOException e) {
                        e.printStackTrace();
                        Assert.fail();
                    }
                    endTime = System.currentTimeMillis();
                    interval = endTime - startTime;
                    if (retryTimes == 1 || i != 1) {
                        writeTimeList.add(interval);
                    }
//			        System.out.println(String.format("Write lob takes: %dms", interval));
                } finally {
                    if (lob != null) {
                        lob.close();
                        lob = null;
                    }
                    if (fileInputStream != null) {
                        try {
                            fileInputStream.close();
                        } catch (IOException e) {
                            e.printStackTrace();
                            return;
                        }
                    }
                }
            }
            long sum = 0;
            for (long elem : writeTimeList) {
                sum += elem;
            }
            long avg = sum / writeTimeList.size();
            System.out.println(String.format("##### Write lob average takes: %dms", avg));
            list.add(avg);
        }
    }


    /*
     * 使用write(buf, off, len)/read(buf, off, len)测试v2.8修改后，
     * java驱动lob写性能与v2.6的差距
     * */
    @Test
//    @Ignore
    @Ignore
    public void testWriteRunPacketToLob() throws BaseException {
        if (System.getProperty("os.name").startsWith("Windows")) {
            inputFileName = "E:\\tmp\\sequoiadb-2.6-linux_x86_64-enterprise-installer.run";
            outputFileName = "E:\\tmp\\output\\sequoiadb-2.6-linux_x86_64-enterprise-installer.run";
        } else {
            //inputFileName = "/opt/driver/java/sequoiadb-2.8.1-linux_x86_64-enterprise-installer.run";
            //outputFileName = "/opt/driver/java/sequoiadb-2.8.1-linux_x86_64-enterprise-installer.run_out";
            inputFileName = "/opt/driver/java/14m.txt";
            outputFileName = "/opt/driver/java/14m.txt_out";
        }
        int retryTimes = 3;
        System.out.println(String.format("%d线程并发测试通过buffer的方式写lob的性能", threadNum));
        System.out.println("write buffer is: " + bufferSize);
        long avg = execute(inputFileName, threadNum, retryTimes);
        System.out.println(
            String.format("Write %dMB's lob, %d threads run takes %dms",
                14, threadNum, avg));
    }

    /*
     * 测试写100k的lob时,v2.6与v2.8性能差别
     * */
    @Test
    @Ignore
    public void testWrite100kLobByBuffer() {
        // prepare file to write
        int retryTimes = 1001;
        String fileName = "100k.txt";
        LobHelper.genFile(fileName, 100 * 1024, null);
        // run test
        long avg = execute(fileName, threadNum, retryTimes);
        System.out.println(
            String.format("Write %d KB's lob, %d threads run takes %dms",
                100, threadNum, avg));
    }

    /*
     * 测试写1024k的lob时,v2.6与v2.8性能差别
     * */
    @Test
    @Ignore
    public void testWrite1024kLobByBuffer() {
        // prepare file to write
        int retryTimes = 101;
        String fileName = "1024k.txt";
        LobHelper.genFile(fileName, 1024 * 1024, null);
        // run test
        long avg = execute(fileName, threadNum, retryTimes);
        System.out.println(
            String.format("Write %d MB's lob, %d threads run takes %dms",
                1, threadNum, avg));
    }

    /************************************* help method *************************************/

    private long execute(String fileName, int threadNum, int retryTime) {
        // prepare threads
        Thread[] threads = new Thread[threadNum];
        ConcurrentLinkedQueue<Long> list = new ConcurrentLinkedQueue<Long>();
        long total = 0;
        long avg = 0;
        for (int i = 0; i < threads.length; i++) {
            threads[i] = new Thread(
                new WriteLobByBufferTask(
                    ds, Constants.TEST_CS_NAME_1 + "." + Constants.TEST_CL_NAME_1,
                    fileName, retryTime, list));
        }
        for (int i = 0; i < threads.length; i++) {
            threads[i].start();
        }
        for (int i = 0; i < threads.length; i++) {
            try {
                threads[i].join();
            } catch (InterruptedException e) {
                e.printStackTrace();
                Assert.fail();
            }
        }
        for (long n : list) {
            total += n;
        }
        if (list.size() != 0) {
            avg = total / list.size();
        } else {
            Assert.fail();
        }
        return avg;
    }


}
