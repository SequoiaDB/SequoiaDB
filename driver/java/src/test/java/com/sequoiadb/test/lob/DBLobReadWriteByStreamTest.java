package com.sequoiadb.test.lob;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.test.common.Constants;
import com.sequoiadb.test.common.LobHelper;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;
import org.junit.*;

import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.ArrayList;

public class DBLobReadWriteByStreamTest {
    private static Sequoiadb sdb;
    private static CollectionSpace cs;
    private static DBCollection cl;

    @BeforeClass
    public static void setConnBeforeClass() throws Exception {
    }

    @AfterClass
    public static void DropConnAfterClass() throws Exception {
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
        conf.put("ReplSize", 0);
        cl = cs.createCollection(Constants.TEST_CL_NAME_1, conf);
    }

    @After
    public void tearDown() throws Exception {
        try {
            if (sdb != null) {
                sdb.disconnect();
            }
        } catch (BaseException e) {
            e.printStackTrace();
        }
    }

    /*
     * 测试java驱动使用lob流式接口读写,功能的正确性。
     * */
    @Test
//    @Ignore
    public void testLobReadWriteByStreamAPI() throws BaseException {
        // prepare file
        String inputFileName = null;
        String outputFileName = null;
        if (System.getProperty("os.name").startsWith("Windows")) {
//        	inputFileName = "E:\\tmp\\sequoiadb-2.6-linux_x86_64-enterprise-installer.run";
//        	outputFileName = "E:\\tmp\\output\\sequoiadb-2.6-linux_x86_64-enterprise-installer.run";
            inputFileName = "input.txt";
            outputFileName = "output.txt";
        } else {
//        	inputFileName = "/opt/sequoiadb/packet/sequoiadb-2.8-linux_x86_64-enterprise-installer.run";
//        	outputFileName = "/opt/sequoiadb/packet/sequoiadb-2.8-linux_x86_64-enterprise-installer.run_out";
            inputFileName = "input.txt";
            outputFileName = "output.txt";
        }

        LobHelper.genFile(inputFileName, 1024 * 1024 * 10, null);
        LobHelper.deleteOnExist(inputFileName);
        LobHelper.deleteOnExist(outputFileName);

        // write lob
        String inputFileMd5 = null;
        String outputFileMd5 = null;
        FileInputStream fileInputStream = null;
        FileOutputStream fileOutputStream = null;
        long startTime = 0;
        long endTime = 0;
        long interval = 0;

        DBLob lob = null;
        ObjectId id = null;
        int retryTimes = 1;
        ArrayList<Long> writeTimeList = new ArrayList<Long>();
        ArrayList<Long> readTimeList = new ArrayList<Long>();
        int i = 0;
        while (i++ < retryTimes) {
            if (id != null) {
                try {
                    cl.removeLob(id);
                } catch (BaseException e) {
                }
            }
            System.out.println(String.format("---------------- No.%d time ----------------", i));
            long totalStartTime = System.currentTimeMillis();
            try {
                lob = cl.createLob();
                id = lob.getID();
                try {
                    fileInputStream = new FileInputStream(inputFileName);
                } catch (FileNotFoundException e) {
                    e.printStackTrace();
                    return;
                }
                startTime = System.currentTimeMillis();
                lob.write(fileInputStream);
                endTime = System.currentTimeMillis();
                interval = endTime - startTime;
                if (retryTimes == 1 || i != 1) {
                    writeTimeList.add(interval);
                }
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

            // read lob
            try {
                lob = cl.openLob(id);
                try {
                    fileOutputStream = new FileOutputStream(outputFileName);
                } catch (FileNotFoundException e) {
                    e.printStackTrace();
                    return;
                }
                startTime = System.currentTimeMillis();
                lob.read(fileOutputStream);
                endTime = System.currentTimeMillis();
                interval = endTime - startTime;
                if (retryTimes == 1 || i != 1) {
                    readTimeList.add(interval);
                }
            } finally {
                if (lob != null) {
                    lob.close();
                }
                if (fileOutputStream != null) {
                    try {
                        fileOutputStream.close();
                    } catch (IOException e) {
                        e.printStackTrace();
                        return;
                    }
                }
            }

            // check
            try {
                startTime = System.currentTimeMillis();
                inputFileMd5 = LobHelper.getMD5(inputFileName);
                endTime = System.currentTimeMillis();
                interval = endTime - startTime;

                startTime = System.currentTimeMillis();
                outputFileMd5 = LobHelper.getMD5(outputFileName);
                endTime = System.currentTimeMillis();
                interval = endTime - startTime;
            } catch (IOException e) {
                e.printStackTrace();
            }
            long totalEndTime = System.currentTimeMillis();
            interval = totalEndTime - totalStartTime;
            Assert.assertEquals(inputFileMd5, outputFileMd5);
        }
        long sum = 0;
        for (long elem : writeTimeList) {
            sum += elem;
        }
        long avg = sum / writeTimeList.size();
        System.out.println(String.format("##### Write lob average takes: %dms", avg));
        sum = 0;
        for (long elem : readTimeList) {
            sum += elem;
        }
        avg = sum / readTimeList.size();
        System.out.println(String.format("##### Read lob average takes: %dms", avg));
    }


}
