package com.sequoiadb.test.lob;

import com.sequoiadb.base.*;
import com.sequoiadb.datasource.SequoiadbDatasource;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.test.common.Constants;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.bson.types.ObjectId;
import org.junit.*;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.concurrent.atomic.AtomicInteger;

public class DBLobSnapshotTest {
    private static Sequoiadb sdb;
    private static CollectionSpace cs;
    private static DBCollection cl;
    private static SequoiadbDatasource ds;
    private static boolean isCluster = true;
    private static byte[] inBytes = Constants.data2mb.getBytes();
    private static String writeData = "abcdefghijklmnopqrstuvwxyz0123456789";

    @BeforeClass
    public static void setConnBeforeClass() throws Exception {
        isCluster = Constants.isCluster();
        byte[] inBytes = Constants.data2mb.getBytes();
    }

    @AfterClass
    public static void DropConnAfterClass() throws Exception {
    }

    @Before
    public void setUp() throws Exception {
        // datasource
        ds = new SequoiadbDatasource(Arrays.asList(Constants.COOR_NODE_CONN),
                "admin", "admin", new ConfigOptions(), null);
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
//        conf.put("Group", Constants.GROUPNAME);
        cl = cs.createCollection(Constants.TEST_CL_NAME_1, conf);
    }

    @After
    public void tearDown() throws Exception {
        try {
//            sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
            sdb.close();
        } catch (BaseException e) {
            e.printStackTrace();
        }
        if (ds != null) {
            ds.close();
        }
    }

    @Test
    public void testLobMode() {
        byte readData[] ;
        ObjectId oid;
        int seq = 1;

        // case 1: create empty lob
        {
            sdb.msg("case begin: " + seq);
            DBLob baseLob = cl.createLob();
            baseLob.close();
            oid = baseLob.getID();
            sdb.msg("case end: " + seq++);
        }

        // case 2, write to an exist lob.
        {
            sdb.msg("case begin: " + seq);
            DBLob lob1 = cl.openLob(oid, DBLob.SDB_LOB_WRITE);
            try {
                lob1.write(writeData.getBytes());
            } catch (BaseException e) {
                Assert.assertEquals(0, e.getErrorCode());
            } finally {
                lob1.close();
            }
            sdb.msg("case end: " + seq++);
        }

        // case 3, write to an newly created lob.
        {
            sdb.msg("case begin: " + seq);
            DBLob lob2 = cl.createLob();
            try {
                lob2.write(writeData.getBytes());
            } catch (BaseException e) {
                Assert.assertEquals(0, e.getErrorCode());
            } finally {
                lob2.close();
            }
            sdb.msg("case end: " + seq++);
        }

        // case 4, read from an exist lob.
        {
            sdb.msg("case begin: " + seq);
            DBLob lob3 = cl.openLob(oid, DBLob.SDB_LOB_READ);
            try {
                readData = new byte[(int) lob3.getSize()];
                lob3.read(readData);
            } catch (BaseException e) {
                Assert.assertEquals(0, e.getErrorCode());
            } finally {
                lob3.close();
            }
            sdb.msg("case end: " + seq++);
        }

        // case 5, read from an exist lob.
        {
            sdb.msg("case begin: " + seq);
            DBLob lob4 = cl.openLob(oid, DBLob.SDB_LOB_SHAREREAD);
            try {
                readData = new byte[(int) lob4.getSize()];
                lob4.read(readData);
            } catch (BaseException e) {
                Assert.assertEquals(0, e.getErrorCode());
            } finally {
                lob4.close();
            }
            sdb.msg("case end: " + seq++);
        }

        // case 6, open lob with write mode, read data from lob after write.
        {
            sdb.msg("case begin: " + seq);
            DBLob lob5 = cl.openLob(oid, DBLob.SDB_LOB_SHAREREAD | DBLob.SDB_LOB_WRITE);
            try {
                lob5.write(writeData.getBytes());
                readData = new byte[(int) lob5.getSize()];
                lob5.read(readData);
            } catch (BaseException e) {
                Assert.assertEquals(0, e.getErrorCode());
            } finally {
                lob5.close();
            }
            sdb.msg("case end: " + seq++);
        }

        // case 7, truncate lob.
        {
            sdb.msg("case begin: " + seq);
            cl.truncateLob(oid, 10);
            sdb.msg("case end: " + seq++);
        }

        // case 8, remove lob.
        {
            sdb.msg("case begin: " + seq);
            cl.removeLob(oid);
            sdb.msg("case end: " + seq++);
        }
    }

    @Test
    public void testLobAlternate() {
        sdb.resetSnapshot();
        DBLob lob1 = cl.createLob();
        DBLob lob2 = cl.createLob();
        byte[] bytes = inBytes;
        System.out.println("bytes is: " + bytes.length);
        lob1.write(bytes);
        lob1.close();
        lob2.write(bytes);
        lob2.close();
//        // check result
//        DBCursor cursor = getSnapshot();
//        BSONObject result = cursor.getNext();
//        result.get()
    }

    @Test
    public void testOpenReadWrite() {
        byte[] outBytes = new byte[2000];
        System.out.println("bytes is: " + inBytes.length);
        DBLob lob = cl.createLob();
        lob.write(inBytes);
        lob.close();

        DBLob lob1;
        DBLob lob2;
        lob1 = cl.openLob(lob.getID(),
                DBLob.SDB_LOB_SHAREREAD|DBLob.SDB_LOB_WRITE);
        lob2 = cl.openLob(lob.getID(),
                DBLob.SDB_LOB_SHAREREAD|DBLob.SDB_LOB_WRITE);
//        lob1.lockAndSeek(0, outBytes.length);
//        lob1.read(outBytes);  // totalReadSize = 1000
//        lob1.seek(1000, DBLob.SDB_LOB_SEEK_CUR);
//        lob1.lockAndSeek(outBytes.length, outBytes.length);
//        lob1.read(outBytes);  // totalReadSize = 2000
        lob2.lockAndSeek(inBytes.length, inBytes.length);
        lob2.write(inBytes);    // totalReadSize = 1000
        lob1.close();
        lob2.close();

        lob1 = cl.openLob(lob.getID(),
                DBLob.SDB_LOB_SHAREREAD);
        lob2 = cl.openLob(lob.getID(),
                    DBLob.SDB_LOB_WRITE);
        System.out.println("lob1 size: " + lob1.getSize());
        System.out.println("lob2 size: " + lob2.getSize());
    }

    @Test
    public void testUseLobWithQuery() {
        DBCursor cursor;
        byte[] outBytes = new byte[2000];
        System.out.println("bytes is: " + inBytes.length);
        DBLob lob = cl.createLob();

        System.out.println("snapshot 6: 1");
        cursor = sdb.getSnapshot(6, new BasicBSONObject("RawData", true), null, null);
        while(cursor.hasNext()) {
            System.out.println(cursor.getNext());
        }

        lob.write(inBytes);
        System.out.println("snapshot 6: 2");
        cursor = sdb.getSnapshot(6, new BasicBSONObject("RawData", true), null, null);
        while(cursor.hasNext()) {
            System.out.println(cursor.getNext());
        }

        cl.insert(new BasicBSONObject("a", 1));
        System.out.println("snapshot 6: 3");
        cursor = sdb.getSnapshot(6, new BasicBSONObject("RawData", true), null, null);
        while(cursor.hasNext()) {
            System.out.println(cursor.getNext());
        }

        lob.close();
    }

    DBCursor getSnapshot() {
        BSONObject obj = new BasicBSONObject();
        BSONObject arr = new BasicBSONList();
        arr.put("0", new BasicBSONObject("TotalLobWriteSize", new BasicBSONObject("$gt", 0)));
        arr.put("1", new BasicBSONObject("TotalLobAddressing", new BasicBSONObject("$gt", 0)));
        obj.put("$or", arr);
//        System.out.println("obj is: " + obj);
        return getSnapshot(obj);
    }

    DBCursor getSnapshot(BSONObject matcher) {
        DBCursor cursor = sdb.getSnapshot(6,
                matcher,
                new BasicBSONObject()
                        .append("TotalLobGet","")
                        .append("TotalLobPut","")
                        .append("TotalLobDelete","")
                        .append("TotalLobList","")
                        .append("TotalLobReadSize","")
                        .append("TotalLobWriteSize","")
                        .append("TotalLobRead","")
                        .append("TotalLobWrite", "")
                        .append("TotalLobTruncate", "")
                        .append("TotalLobAddressing", ""),
                null);
        return cursor;
    }

    void lobRoutine(DBCollection coll) {
        long truncateLen = 1000 * 1000;
        byte[] outBytes = new byte[2000];

        /**
         * after lob write:
         * in cluster:
         * TotalLobGet: 0, TotalLobPut: 2, TotalLobDelete: 0, TotalLobList: 0,
         * TotalLobReadSize: 0, TotalLobWriteSize: 4000000,
         * TotalLobRead: 0, TotalLobWrite: 8, TotalLobTruncate: 0, TotalLobAddressing: 8
         * in standalone:
         * TotalLobGet: 0, TotalLobPut: 1, TotalLobDelete: 0, TotalLobList: 0,
         * TotalLobReadSize: 0, TotalLobWriteSize: 2000000,
         * TotalLobRead: 0, TotalLobWrite: 8, TotalLobTruncate: 0, TotalLobAddressing: 8
         */
        DBLob lob = coll.createLob();
        lob.write(inBytes);
        lob.close();
//        System.out.println("after write, snapshot: " + getSnapshot().getNext());

        /**
         * after lob read:
         * in cluster:
         * TotalLobGet: 2, TotalLobPut: 2, TotalLobDelete: 0, TotalLobList: 0,
         * TotalLobReadSize: 4000000, TotalLobWriteSize: 4000000,
         * TotalLobRead: 8, TotalLobWrite: 8, TotalLobTruncate: 0, TotalLobAddressing: 16
         * in standalone:
         * TotalLobGet: 1, TotalLobPut: 1, TotalLobDelete: 0, TotalLobList: 0,
         * TotalLobReadSize: 2000000, TotalLobWriteSize: 2000000,
         * TotalLobRead: 8, TotalLobWrite: 8, TotalLobTruncate: 0, TotalLobAddressing: 16
         */
        lob = coll.openLob(lob.getID());
        int readSize = 0;
        while(readSize < lob.getSize()) {
            int readLen = lob.read(outBytes);
            readSize += readLen;
        }
        lob.close();
//        System.out.println("after read, snapshot: " + getSnapshot().getNext());
        Assert.assertEquals(lob.getSize(), readSize);

        /**
         * after lob truncate:
         * in cluster:
         * TotalLobGet: 2, TotalLobPut: 2, TotalLobDelete: 2, TotalLobList: 0,
         * TotalLobReadSize: 4000000, TotalLobWriteSize: 4000000,
         * TotalLobRead: 14, TotalLobWrite: 9, TotalLobTruncate: 4, TotalLobAddressing: 22
         * in standalone:
         * TotalLobGet: 1, TotalLobPut: 1, TotalLobDelete: 1, TotalLobList: 0,
         * TotalLobReadSize: 2000000, TotalLobWriteSize: 2000000,
         * TotalLobRead: 14, TotalLobWrite: 9, TotalLobTruncate: 4, TotalLobAddressing: 22
         */
        coll.truncateLob(lob.getID(), truncateLen);
//        System.out.println("after truncate, snapshot: " + getSnapshot().getNext());

        /**
         * after lob delete:
         * in cluster:
         * TotalLobGet: 2, TotalLobPut: 2, TotalLobDelete: 4, TotalLobList: 0,
         * TotalLobReadSize: 4000000, TotalLobWriteSize: 4000000,
         * TotalLobRead: 19, TotalLobWrite: 9, TotalLobTruncate: 8, TotalLobAddressing: 27
         * in standalone:
         * TotalLobGet: 1, TotalLobPut: 1, TotalLobDelete: 2, TotalLobList: 0,
         * TotalLobReadSize: 2000000, TotalLobWriteSize: 2000000,
         * TotalLobRead: 19, TotalLobWrite: 9, TotalLobTruncate: 8, TotalLobAddressing: 27
         */
        coll.removeLob(lob.getID());
//        System.out.println("after delete, snapshot: " + getSnapshot().getNext());
    }

    void lobListRoutine(DBCollection coll, int lobCount) {
        ArrayList<ObjectId> arrayList = new ArrayList();
        for (int i = 0; i < lobCount; i++) {
            DBLob lob = coll.createLob();
            lob.write(inBytes);
            lob.close();
            arrayList.add(lob.getID());
        }
        System.out.println("after write, snapshot: " + getSnapshot().getNext());
        sdb.resetSnapshot();
        DBCursor cursor = coll.listLobs();
        while (cursor.hasNext()) {
            cursor.getNext();
        }
        System.out.println("after list, snapshot: " + getSnapshot().getNext());
    }

    long getLong(Object obj) {
        if (obj instanceof Double) {
            return Double.valueOf((Double) obj).longValue();
        } else {
            return (long)obj;
        }
    }

    void checkLobCRUDResult(BSONObject result, int runTimes) {
        long totalLobGet = getLong(result.get("TotalLobGet"));
        long totalLobPut = getLong(result.get("TotalLobPut"));
        long totalLobDelete = getLong(result.get("TotalLobDelete"));
        long totalLobList = getLong(result.get("TotalLobList"));
        long totalLobReadSize = getLong(result.get("TotalLobReadSize"));
        long totalLobWriteSize = getLong(result.get("TotalLobWriteSize"));
        long totalLobRead = getLong(result.get("TotalLobRead"));
        long totalLobWrite = getLong(result.get("TotalLobWrite"));
        long totalLobTruncate = getLong(result.get("TotalLobTruncate"));
        long totalLobAddressing = getLong(result.get("TotalLobAddressing"));
        if ( isCluster ) {
            Assert.assertEquals(2 * runTimes, totalLobGet);
            Assert.assertEquals(2 * runTimes, totalLobPut);
            Assert.assertEquals(4 * runTimes, totalLobDelete);
            Assert.assertEquals(0 * runTimes, totalLobList);
            Assert.assertEquals(4000000 * runTimes, totalLobReadSize);
            Assert.assertEquals(4000000 * runTimes, totalLobWriteSize);
            Assert.assertEquals(19 * runTimes, totalLobRead);
            Assert.assertEquals(9 * runTimes, totalLobWrite);
            Assert.assertEquals(8 * runTimes, totalLobTruncate);
            Assert.assertEquals(27 * runTimes, totalLobAddressing);
        } else {
            Assert.assertEquals(1 * runTimes, totalLobGet);
            Assert.assertEquals(1 * runTimes, totalLobPut);
            Assert.assertEquals(2 * runTimes, totalLobDelete);
            Assert.assertEquals(0 * runTimes, totalLobList);
            Assert.assertEquals(2000000 * runTimes, totalLobReadSize);
            Assert.assertEquals(2000000 * runTimes, totalLobWriteSize);
            Assert.assertEquals(19 * runTimes, totalLobRead);
            Assert.assertEquals(9 * runTimes, totalLobWrite);
            Assert.assertEquals(8 * runTimes, totalLobTruncate);
            Assert.assertEquals(27 * runTimes, totalLobAddressing);
        }
    }

    void checkLobListResult(BSONObject result, int lobCount) {
        long totalLobGet = getLong(result.get("TotalLobGet"));
        long totalLobPut = getLong(result.get("TotalLobPut"));
        long totalLobDelete = getLong(result.get("TotalLobDelete"));
        long totalLobList = getLong(result.get("TotalLobList"));
        long totalLobReadSize = getLong(result.get("TotalLobReadSize"));
        long totalLobWriteSize = getLong(result.get("TotalLobWriteSize"));
        long totalLobRead = getLong(result.get("TotalLobRead"));
        long totalLobWrite = getLong(result.get("TotalLobWrite"));
        long totalLobTruncate = getLong(result.get("TotalLobTruncate"));
        long totalLobAddressing = getLong(result.get("TotalLobAddressing"));

        Assert.assertEquals(0, totalLobGet);
        Assert.assertEquals(0, totalLobPut);
        Assert.assertEquals(0, totalLobDelete);
        if (isCluster) {
            Assert.assertEquals(2, totalLobList);
            Assert.assertEquals(lobCount, totalLobRead);
            Assert.assertEquals(lobCount, totalLobAddressing);
        } else {
            Assert.assertEquals(1, totalLobList);
            Assert.assertEquals(lobCount, totalLobRead);
            Assert.assertEquals(lobCount, totalLobAddressing);
        }
        Assert.assertEquals(0, totalLobReadSize);
        Assert.assertEquals(0, totalLobWriteSize);
        Assert.assertEquals(0, totalLobWrite);
        Assert.assertEquals(0, totalLobTruncate);
    }

    @Test
    public void testLobListBySerial() {
        // list lobs
        int lobCount = 100;
        sdb.resetSnapshot();
        lobListRoutine(cl, lobCount);
        DBCursor cursor = getSnapshot();
        checkLobListResult(cursor.getNext(), lobCount);
    }

    @Test
    public void testLobCRUDBySerial() {
        sdb.resetSnapshot();
        int runTimes = 1;
        for (int i = 0; i < runTimes; i++) {
            lobRoutine(cl);
        }
        // check result
        DBCursor cursor = getSnapshot();
        checkLobCRUDResult(cursor.getNext(), runTimes);
    }

    class LobRoutine implements Runnable {
        private SequoiadbDatasource ds;
        private AtomicInteger atomicRunTimes;

        public LobRoutine(SequoiadbDatasource datasource,
                          AtomicInteger runTimes) {
            ds = datasource;
            atomicRunTimes = runTimes;
        }

        @Override
        public void run() {
            Sequoiadb db = null;
            try {
                db = ds.getConnection();
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
            try {
                DBCollection coll = db.getCollectionSpace(Constants.TEST_CS_NAME_1).
                        getCollection(Constants.TEST_CL_NAME_1);
                while (atomicRunTimes.getAndDecrement() > 0) {
                    lobRoutine(coll);
                }
            } finally {
                ds.releaseConnection(db);
            }
        }
    }

    @Test
    public void testLobCRUDByParallel() {
        sdb.resetSnapshot();
        int totalRunTimes = 500;
        int threadNum = 100;
        AtomicInteger atomicRunTimes = new AtomicInteger(totalRunTimes);
        Thread[] threads = new Thread[threadNum];

        // run test
        for ( int i = 0 ; i < threadNum ; i++ ) {
            threads[i] = new Thread(new LobRoutine(ds, atomicRunTimes));
            threads[i].start();
		}
		for (int i = 0; i < threadNum; i++) {
			try {
                threads[i].join();
			} catch (InterruptedException e) {
				e.printStackTrace();
			}
		}

        // checkout result
        DBCursor cursor = getSnapshot();
        checkLobCRUDResult(cursor.getNext(), totalRunTimes);
    }

}
