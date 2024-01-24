package com.sequoiadb.test.lob;

import com.sequoiadb.base.*;
import com.sequoiadb.datasource.SequoiadbDatasource;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.test.common.Constants;
import com.sequoiadb.test.common.Helper;
import com.sequoiadb.testdata.SDBTestHelper;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;
import org.junit.*;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.util.Arrays;
import java.util.Random;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import static org.junit.Assert.assertEquals;

public class DBLobTest {
    private static Sequoiadb sdb;
    private static CollectionSpace cs;
    private static DBCollection cl;
    private static SequoiadbDatasource ds;

    private static final String LOB_SIZE = "Size";
    private static final String LOB_AVAILABLE = "Available";
    private static final String LOB_OID = "Oid";

    @BeforeClass
    public static void setConnBeforeClass() throws Exception {

    }

    @AfterClass
    public static void DropConnAfterClass() throws Exception {

    }

    @Before
    public void setUp() throws Exception {
        ds = new SequoiadbDatasource(Arrays.asList(Constants.COOR_NODE_CONN),
                "admin", "admin", null, null);
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
            sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
            sdb.disconnect();
        } catch (BaseException e) {
            e.printStackTrace();
        }
        if (ds != null) {
            ds.close();
        }
    }

    class ReadLobTask implements Runnable {
        ObjectId objectId;
        int serialNum;
        ReadLobTask(ObjectId objectId, int serialNum) {
            this.objectId = objectId;
            this.serialNum = serialNum;
        }
        @Override
        public void run() {
            int bufSize = 1024;
            byte[] buffer = new byte[bufSize];
            DBLob lob = null;
            try {
                lob = ds.getConnection().getCollectionSpace(Constants.TEST_CS_NAME_1).
                        getCollection(Constants.TEST_CL_NAME_1).openLob(objectId);
            } catch (InterruptedException e) {
                e.printStackTrace();
                Assert.fail();
            }
            try {
                while(lob.read(buffer) != -1);
            } finally {
                lob.close();
            }
            Helper.print("finish thread: " + serialNum);
        }
    }

    @Test
    public void testLobConcurrentReading() {
        ObjectId objectId = new ObjectId();
        // write lob
        DBLob lob = cl.createLob(objectId);
        String str = "hello world";
        try {
            for (int i = 0; i < 10000; i++) {
                lob.write(str.getBytes());
            }
        } finally {
            lob.close();
        }
        // concurrent read lob
        int threadCount = 50;
        Thread[] threads = new Thread[threadCount];
        for (int i = 0; i < threadCount; ++i) {
            threads[i] = new Thread(new ReadLobTask(objectId, i));
        }
        for(int i = 0; i < threadCount; ++i) {
            threads[i].start();
        }
        for(int i = 0; i < threadCount; ++i) {
            try {
                threads[i].join();
            } catch (InterruptedException e) {
                e.printStackTrace();
                Assert.fail();
            }
        }
        Helper.print("finish running!");
    }

    /*
     * create an empty lob
     * */
    @Test
    public void testCreateLob() throws BaseException {
        DBLob lob = cl.createLob();
        lob.close();

        long createTime = lob.getCreateTime();
        long modificationTime = lob.getModificationTime();
        long size = lob.getSize();
        ObjectId id = lob.getID();
        SDBTestHelper.println("id:" + id);
        SDBTestHelper.println("createTime:" + SDBTestHelper.millisToDate(createTime));
        SDBTestHelper.println("modificationTime:" + SDBTestHelper.millisToDate(modificationTime));
        SDBTestHelper.println("lobSize:" + size);
        assertEquals(0, size);
    }

    /*
     * test create Lob with ID
     * */
    @Test
    public void testCreateLobWithID() throws BaseException {
        ObjectId id = ObjectId.get();
        DBLob lob = cl.createLob(id);
        String w = "Helloworld";
        lob.write(w.getBytes());
        lob.close();

        lob = cl.openLob(id);
        byte read[] = new byte[100];
        int len = lob.read(read);
        String s = new String(read, 0, len);
        SDBTestHelper.println("read data:" + s);
        assertEquals(true, w.equals(s));
    }

    @Test
    public void testCreateWriteLob() throws BaseException {
        // create a lob and write data into this lob
        DBLob lob = cl.createLob();
        Assert.assertTrue(lob.isEof());
        String data = new String("HelloWorld1234567890");
        lob.write(data.getBytes());
        Assert.assertTrue(lob.isEof());
        lob.close();
        Assert.assertTrue(lob.isEof());

        ObjectId id = lob.getID();

        // read data from the lob just created.
        DBLob rLob = cl.openLob(id);
        Assert.assertFalse(rLob.isEof());

        byte[] b = new byte[5];
        int len = rLob.read(b);
        assertEquals(5, len);
        Assert.assertFalse(rLob.isEof());
        String rData = new String(b, 0, len);
        assertEquals(true, rData.equals(data.substring(0, 5)));
        rLob.close();
        Assert.assertFalse(rLob.isEof());
    }

    @Test
    public void testSeekLob() throws BaseException {
        // create a lob and write data into this lob
        DBLob lob = cl.createLob();
        // data.length = 10(this data affect the the follow code)
        String data = "HelloWorld1234567890";
        lob.write(data.getBytes());
        lob.close();

        ObjectId id = lob.getID();

        // read data from the lob just created.
        DBLob rLob = cl.openLob(id);

        // offset = 5
        rLob.seek(5, DBLob.SDB_LOB_SEEK_SET);
        byte[] b = new byte[5];
        // after read offset = 10
        int len = rLob.read(b);
        assertEquals(5, len);
        String rData = new String(b, 0, len);
        assertEquals(true, rData.equals(data.substring(5, 10)));

        // now we change offset = 0
        rLob.seek(-10, DBLob.SDB_LOB_SEEK_CUR);
        // after read offset = 5
        len = rLob.read(b);
        assertEquals(5, len);
        rData = new String(b, 0, len);
        assertEquals(true, rData.equals(data.substring(0, 5)));

        // now we change offset = 15
        rLob.seek(5, DBLob.SDB_LOB_SEEK_END);
        // after read offset = 5
        len = rLob.read(b);
        assertEquals(5, len);
        rData = new String(b, 0, len);
        System.out.println("rData is: " + rData);
        System.out.println("data.substring( 15, 20 ) is: " + data.substring(15, 20));
        assertEquals(true, rData.equals(data.substring(15, 20)));

        rLob.close();
    }

    @Test
    public void testListLobs() throws BaseException {
        // create two lob
        DBLob lob = cl.createLob();
        lob.close();

        ObjectId id1 = lob.getID();

        lob = cl.createLob();
        lob.write("123".getBytes());
        ObjectId id2 = lob.getID();
        lob.close();

        DBCursor cur = cl.listLobs();
        int count = 0;
        while (cur.hasNext()) {
            BSONObject obj = cur.getNext();
            SDBTestHelper.println(obj.toString());
            ObjectId id = (ObjectId) obj.get(LOB_OID);
            assertEquals(true, id.equals(id1) || id.equals(id2));

            if (id.equals(id1)) {
                long size = (Long) obj.get(LOB_SIZE);
                boolean isAvailable = (Boolean) obj.get(LOB_AVAILABLE);
                assertEquals(true, isAvailable);
                assertEquals(0, size);
            } else {
                long size = (Long) obj.get(LOB_SIZE);
                boolean isAvailable = (Boolean) obj.get(LOB_AVAILABLE);
                assertEquals(true, isAvailable);
                assertEquals(3, size);
            }

            count++;
        }

        assertEquals(2, count);
    }

    @Test
    public void testRemoveLob() throws BaseException {
        // create two lob
        DBLob lob = cl.createLob();
        lob.close();
        Assert.assertTrue(lob.isEof());

        ObjectId id1 = lob.getID();

        lob = cl.createLob();
        lob.write("123".getBytes());
        ObjectId id2 = lob.getID();
        lob.close();

        // remove id1's lob
        cl.removeLob(id1);

        DBCursor cur = cl.listLobs();
        while (cur.hasNext()) {
            BSONObject obj = cur.getNext();
            SDBTestHelper.println(obj.toString());
            ObjectId id = (ObjectId) obj.get(LOB_OID);
            assertEquals(true, id.equals(id2));

            long size = (Long) obj.get(LOB_SIZE);
            boolean isAvailable = (Boolean) obj.get(LOB_AVAILABLE);
            assertEquals(true, isAvailable);
            assertEquals(3, size);
        }
    }

    @Test
    public void testLargeFile() {

        //1024 bits
        String s1 = "1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234";
        //26 bits
        String s2 = "abcdefghijklmnopqrstuvwxyz";
        //times
        int t1 = 1024 * 16;
        int t2 = 1;
        long allSize = s1.getBytes().length * t1 + s2.getBytes().length * t2;
        System.out.println("allSize:" + allSize);

        ObjectId id = ObjectId.get();
        DBLob lob = cl.createLob(id);

        //insert lob
        for (int i = 0; i < t1; i++) {
            lob.write(s1.getBytes());
        }
        for (int i = 0; i < t2; i++) {
            lob.write(s2.getBytes());
        }
        lob.close();

        //list lob
        DBCursor cur = cl.listLobs();
        while (cur.hasNext()) {
            BSONObject obj = cur.getNext();
            ObjectId cid = (ObjectId) obj.get(LOB_OID);
            if (id.equals(cid)) {
                long size = (Long) obj.get(LOB_SIZE);
                boolean isAvailable = (Boolean) obj.get(LOB_AVAILABLE);
                assertEquals(true, isAvailable);
                assertEquals(allSize, size);
                break;
            }
        }
        cur.close();

        //read lob
        DBLob rLob = cl.openLob(id);
        int len = 0;
        long total = 0;
        byte[] tmp = new byte[1000];
        while ((len = rLob.read(tmp)) > 0) {
            total += len;
            if (total >= allSize) {
                break;
            }
            if (total != allSize) {
                Assert.assertFalse(rLob.isEof());
            }
        }
        Assert.assertTrue(rLob.isEof());
        assertEquals(allSize, total);
        rLob.close();
        Assert.assertTrue(rLob.isEof());
        //remove lob
        cl.removeLob(id);
    }

    @Test
    public void testWithoutClose() {
        int times = 10;

        // 1KB
        String s1 = "1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234";
        byte[] tmp = new byte[s1.getBytes().length + 1];

        ObjectId id = ObjectId.get();

        DBLob lob = cl.createLob(id);
        lob.write(s1.getBytes());
        lob.close();

        for (int i = 0; i < times; i++) {
            DBLob rLob = cl.openLob(id);
            int len = rLob.read(tmp);
            if (len != s1.getBytes().length) {
                System.out.println("open times:" + i);
                assertEquals(s1.getBytes().length, len);
                break;
            }
        }

        try {
            cl.removeLob(id);
            Assert.fail("remove unclosed lob should be failed");
        } catch (BaseException e) {
            assertEquals(SDBError.SDB_LOB_IS_IN_USE.getErrorCode(), e.getErrorCode());
        }
    }

    @Test
    public void testLobFalse() {
        //1024 bits
        String s1 = "1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234";
        byte[] tmp = new byte[s1.getBytes().length + 1];

        ObjectId id = ObjectId.get();

        //insert lob
        DBLob lob = cl.createLob(id);
        lob.write(s1.getBytes());
        //without close
        //lob.close();

        try {
            DBLob rLob = cl.openLob(id);
            rLob.read(tmp);
            rLob.close();
            System.err.println("error:writing a lob without calling close method but the lob can be read!");
            assertEquals(true, false);
        } catch (BaseException e) {
            System.out.println("writing a lob without calling close method and the lob can't be read");
        }

        //close lob
        lob.close();

        try {
            DBLob rLob = cl.openLob(id);
            rLob.read(tmp);
            rLob.close();
            System.out.println("writing a lob with calling close method and the lob can be read");
        } catch (BaseException e) {
            System.err.println("error:writing a lob with calling close method but the lob can't be read");
            assertEquals(true, false);
        }

        //remove lob
        cl.removeLob(id);
    }

    @Test
    public void testOpenSameLobID() {
        //1024 bits
        String s1 = "1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234";
        byte[] tmp = new byte[s1.getBytes().length + 1];

        ObjectId id = ObjectId.get();

        //insert lob
        DBLob lob = cl.createLob(id);
        lob.write(s1.getBytes());
        lob.close();

        int nums = 100;
        DBLob[] rlobs = new DBLob[nums];

        //open lob
        for (int i = 0; i < nums; i++) {
            rlobs[i] = cl.openLob(id);
        }

        //read lob
        for (int i = 0; i < nums; i++) {
            int len = rlobs[i].read(tmp);
            if (len != s1.getBytes().length) {
                assertEquals(s1.getBytes().length, len);
                System.out.println("error: read lob length is not expected at " + i + " times!");
                System.out.println("expected:" + s1.getBytes().length + "\n returned:" + len);
                break;
            }
        }

        //close
        for (int i = 0; i < nums; i++) {
            rlobs[i].close();
        }

        //remove lob
        cl.removeLob(id);
    }

    @Test
    public void testRemoveFalseLob() {
        //1024 bits
        String s1 = "1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234";

        ObjectId id = ObjectId.get();

        //insert lob
        DBLob lob = cl.createLob(id);
        lob.write(s1.getBytes());
        //without close
        //lob.close();

        try {
            //remove lob
            cl.removeLob(id);
            assertEquals(true, false);
            System.err.println("error:remove unavailable lob, expected false, but return true!");
        } catch (BaseException e) {
            System.out.println("can't remove unavailable lob");
        }

        //close lob
        lob.close();

        cl.removeLob(id);
    }

    @Test
    public void testWriteAndDisconnect() {
        //1024 bits
        String s1 = "1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234";
        byte[] tmp = new byte[s1.getBytes().length + 1];

        ObjectId id = ObjectId.get();

        //insert lob
        DBLob lob = cl.createLob(id);
        lob.write(s1.getBytes());
        //without close
        //lob.close();

        sdb.disconnect();

        sdb = new Sequoiadb(Constants.COOR_NODE_CONN, "admin", "admin");
        cs = sdb.getCollectionSpace(Constants.TEST_CS_NAME_1);
        cl = cs.getCollection(Constants.TEST_CL_NAME_1);

        try {
            lob.close();
            System.err.println("error:sdb is disconnected but lob.close succ!");
        } catch (BaseException e) {
            System.out.println("");
        }

        //list lob
        DBCursor cur = cl.listLobs();
        while (cur.hasNext()) {
            BSONObject obj = cur.getNext();
            ObjectId cid = (ObjectId) obj.get(LOB_OID);
            if (id.equals(cid)) {
                long size = (Long) obj.get(LOB_SIZE);
                boolean isAvailable = (Boolean) obj.get(LOB_AVAILABLE);
                assertEquals(false, isAvailable);
                assertEquals(0, size);
            }
        }
        cur.close();

        //read lob
        try {
            DBLob rLob = cl.openLob(id);
            rLob.read(tmp);
            rLob.close();
            assertEquals(true, false);
            System.err.println("error:disconnect before calling close method but the lob can be read");
        } catch (BaseException e) {
            System.out.println("disconnect before calling close method and the lob can't be read");
        }

        //remove lob
        try {
            //remove lob
            cl.removeLob(id);
            assertEquals(true, false);
            System.err.println("error:remove unavailable lob, expected false, but return true!");
        } catch (BaseException e) {
            System.out.println("can't remove unavailable lob");
        }
    }

    @Test
    public void testOpenWithReturnData() {
        int off = 50;
        int len1 = 99;
        int len2 = 2 * 1024 * 1024;
        int len3 = 3 * 1024 * 1024;
        int totalLen = len1 + len2 + len3;
        byte[] arr1 = new byte[len1 + 2 * off];
        byte[] arr2 = new byte[len2 + 2 * off];
        byte[] arr3 = new byte[len3 + 2 * off];
        byte[] out1 = new byte[len1 + 2 * off];
        byte[] out2 = new byte[len2 + 2 * off];
        byte[] out3 = new byte[len3 + 2 * off];
        Arrays.fill(arr1, off, len1 + off, (byte) 'a');
        Arrays.fill(arr2, off, len2 + off, (byte) 'a');
        Arrays.fill(arr3, off, len3 + off, (byte) 'a');

        DBLob lob = cl.createLob();
        Assert.assertEquals(0, lob.getSize());
        lob.write(arr1, off, len1);
        Assert.assertEquals(len1, lob.getSize());
        lob.write(arr2, off, len2);
        Assert.assertEquals(len1 + len2, lob.getSize());
        lob.write(arr3, off, len3);
        Assert.assertEquals(len1 + len2 + len3, lob.getSize());
        lob.close();

        // run
        ObjectId id = lob.getID();
        DBLob lob2 = cl.openLob(id);
        long createTime = lob2.getCreateTime();
        System.out.println("lob's create time is: " + createTime);
        long size = lob2.getSize();
        Assert.assertEquals(lob2.getSize(), lob.getSize());
        Assert.assertEquals(totalLen, size);
        lob2.read(out1, off, len1);
        lob2.read(out2, off, len2);
        lob2.read(out3, off, len3);
        // check
        for (int i = 0; i < out1.length; i++) {
            if (i < off)
                Assert.assertEquals(0, out1[i]);
            else if (i < len1 + off)
                Assert.assertEquals('a', out1[i]);
            else
                Assert.assertEquals(0, out1[i]);
        }
        for (int i = 0; i < out2.length; i++) {
            if (i < off)
                Assert.assertEquals(0, out2[i]);
            else if (i < len2 + off)
                Assert.assertEquals("i is: " + i, 'a', out2[i]);
            else
                Assert.assertEquals(0, out2[i]);
        }
        for (int i = 0; i < out3.length; i++) {
            if (i < off)
                Assert.assertEquals(0, out3[i]);
            else if (i < len3 + off)
                Assert.assertEquals('a', out3[i]);
            else
                Assert.assertEquals(0, out3[i]);
        }

        lob.close();
    }

    @Test
    public void testReadAndSeek() {
        int size = 12 * 1024 * 1024;
        byte[] arr = new byte[size];
        for (int i = 0; i < size; i++) {
            arr[i] = (byte) (i % 10);
        }

        DBLob lob = cl.createLob();
        lob.write(arr);
        lob.close();

        // run
        ObjectId id = lob.getID();
        DBLob lob2 = cl.openLob(id);
        // case 1: seek "SDB_LOB_SEEK_SET", len is: 10000
        int len = 10000;
        lob2.seek(len, DBLob.SDB_LOB_SEEK_SET);
        byte[] out = new byte[(int) len];
        lob2.read(out);
        // check
        for (int i = 0; i < len; i++) {
            Assert.assertEquals("i is: " + i + ", out[i] is: " + out[i], i % 10, (int) (out[i]));
        }
        // case 2: seek "SDB_LOB_SEEK_CUR", len is: 1024 * 1024 * 3 - 1000
        len = 1024 * 1024 * 3 - 1000;
        out = new byte[(int) len];
        lob2.seek(len, DBLob.SDB_LOB_SEEK_CUR);
        lob2.read(out);
        // check
        for (int i = 0; i < len; i++) {
            Assert.assertEquals("i is: " + i + ", out[i] is: " + out[i],
                (i % 10 + len % 10) % 10, (int) (out[i]));
        }

        // case 3: seek "SDB_LOB_SEEK_END", len is: 10000
        len = 10000;
        lob2.seek(len, DBLob.SDB_LOB_SEEK_END);
        out = new byte[(int) len];
        lob2.read(out);
        int tmp = size - len;
        // check
        for (int i = 0; i < len; i++) {
            Assert.assertEquals("i is: " + i + ", out[i] is: " + out[i], (i % 10 + tmp % 10) % 10, (int) (out[i]));
        }

        lob2.close();
    }

    @Test
    public void testEOF() {
        byte[] arr = new byte[10];
        byte[] out = new byte[10];
        DBLob lob = cl.createLob();
        //lob.write(arr);
        lob.close();

        ObjectId id = lob.getID();
        DBLob rLob = cl.openLob(id);

        int len = rLob.read(out);
        assertEquals("len is" + len, -1, len);
        lob.close();
    }

    @Test
    public void testReadWriteStream() {
        // prepare data
        int byteSize = 2 * 1024 * 1024 + 100;
        byte[] arr = new byte[byteSize];
        byte[] out = new byte[byteSize];
        for (int i = 0; i < arr.length; ++i) {
            arr[i] = (byte) (i % 10);
        }
        ByteArrayInputStream input = new ByteArrayInputStream(arr);
        ByteArrayOutputStream output = new ByteArrayOutputStream(byteSize);

        // test
        // write lob
        DBLob lob = cl.createLob();
        lob.write(input);
        lob.close();
        try {
            input.close();
        } catch (IOException e1) {
            e1.printStackTrace();
            Assert.fail();
        }
        // read lob
        ObjectId id = lob.getID();
        DBLob rLob = cl.openLob(id);
        rLob.read(output);
        try {
            out = output.toByteArray();
            output.close();
        } catch (IOException e) {
            e.printStackTrace();
            Assert.fail();
        }
        lob.close();

        // check
        for (int i = 0; i < byteSize; i++) {
            if (arr[i] != out[i]) {
                Assert.fail(String.format("arr[%d] is: %d, out[%d] is: %d", i,
                    (int) arr[i], i, (int) out[i]));
            }
        }
    }

    /*
     * validacate LobReadTest
     * */
    @Test
    public void testForValidicateTestCaseInCSharp() {
        DBLob lob = null;
        DBLob lob2 = null;
        boolean flag = false;
        ObjectId oid1 = null;
        int bufSize = 1024 * 1024 * 100;
        int readNum = 0;
        int retNum = 0;
        int i = 0;
        byte[] readBuf = null;
        byte[] buf = new byte[bufSize];
        for (i = 0; i < bufSize; i++) {
            buf[i] = 65;
        }
        long lobSize = 0;

        // CreateLob
        lob = cl.createLob();
        try {
            Assert.assertNotNull(lob);
            // GetID
            oid1 = lob.getID();
            Assert.assertTrue(null != oid1);
            // Write
            lob.write(buf);
            lobSize = lob.getSize();
            Assert.assertEquals(bufSize, lobSize);
        } finally {
            // Close
            lob.close();
        }

        // Open lob
        lob2 = cl.openLob(oid1);
        try {
            lobSize = lob2.getSize();
            Assert.assertEquals(bufSize, lobSize);
            // Read
            int skipNum = 1024 * 1024 * 50;
            lob2.seek(skipNum, DBLob.SDB_LOB_SEEK_SET);  // after this, the offset is 1024*1024*50
            readNum = 1024 * 1024 * 10;
            readBuf = new byte[readNum];
            retNum = lob2.read(readBuf);  // after this, the offset is 1024*1024*60
            Assert.assertEquals(readNum, retNum);
            // check
            for (i = 0; i < readBuf.length; i++) {
                Assert.assertEquals(readBuf[i], 65);
            }
            skipNum = 1024 * 1024 * 10;
            lob2.seek(skipNum, DBLob.SDB_LOB_SEEK_CUR); // after this, the offset is 1024*1024*70
            readBuf = new byte[readNum];
            retNum = lob2.read(readBuf);
            Assert.assertEquals(readNum, retNum); // after this, the offset is 1024*1024*80
            // check
            for (i = 0; i < readBuf.length; i++) {
                Assert.assertEquals(readBuf[i], 65);
            }
            skipNum = 1024 * 1024 * 20;
            lob2.seek(skipNum, DBLob.SDB_LOB_SEEK_END);
            readNum = 1024 * 1024 * 30; // will only read 1024*1024*20
            readBuf = new byte[readNum];
            retNum = lob2.read(readBuf); // after this, the offset is 1024*1024*100
            Assert.assertEquals(readNum, (retNum + 1024 * 1024 * 10));
        } finally {
            // Close
            lob2.close();
        }
    }

    static int[] generateSequenceNumber(int Length) {
        int[] ret = new int[Length];
        for (int i = 0; i < Length; i++) {
            ret[i] = i + 1;
        }
        return ret;
    }

    public static byte[] intToBytes(int value) {
        byte[] byte_src = new byte[4];
        byte_src[3] = (byte) ((value & 0xFF000000) >> 24);
        byte_src[2] = (byte) ((value & 0x00FF0000) >> 16);
        byte_src[1] = (byte) ((value & 0x0000FF00) >> 8);
        byte_src[0] = (byte) ((value & 0x000000FF));
        return byte_src;
    }

    /*
     * validacate LobReadTest
     * */
    @Test
    public void LobReadWriteSequenceNumber() {
        // gen data
        Random random = new Random();
        int size = random.nextInt(10 * 1024 * 1024);
        //int size = random.Next(1000000);
        size = 8219676;
        byte[] content_bytes = new byte[size * 4];
        int[] content = generateSequenceNumber(size);
        for (int i = 0; i < size; i++) {
            byte[] tmp_buf = intToBytes(content[i]);
            System.arraycopy(tmp_buf, 0, content_bytes, i * 4, tmp_buf.length);
        }
        //Console.WriteLine("content_bytes is: {0}", BitConverter.ToString(content_bytes));


        int end = content_bytes.length;
        int beg = 0;
        int len = end - beg;
        byte[] output_bytes = new byte[content_bytes.length];

        DBLob lob = null;
        DBLob lob2 = null;

        // write to lob
        try {
            lob = cl.createLob();
            lob.write(content_bytes, beg, len);
        } finally {
            if (lob != null) lob.close();
        }

        // read from lob
        ObjectId id = lob.getID();
        try {
            lob2 = cl.openLob(id);
            lob2.read(output_bytes, beg, len);
        } finally {
            if (lob2 != null) lob2.close();
        }

        // check
        for (int i = 0; i < beg; i++) {
            Assert.assertEquals(0, output_bytes[i]);
        }

        for (int i = beg; i < end; i++) {
            try {
                Assert.assertEquals(content_bytes[i], output_bytes[i]);
            } catch (AssertionError e) {
                String errmsg = String.format("expect: %d, actual: %d, beg: %d, end: %d, len: %d, i: %d", content_bytes[i], output_bytes[i], beg, end, len, i);
                System.out.println("errmsg is: " + errmsg);
                throw e;
            }
        }

        for (int i = end; i < output_bytes.length; i++) {
            Assert.assertEquals(0, output_bytes[i]);
        }

    }

    @Test
    public void testLobMode() {
        String writeData = "test lob mode";
        byte readData[] ;

        DBLob baseLob  = cl.createLob();
        baseLob.close();
        ObjectId oid = baseLob.getID();

        // case 1, write data to lob after create.
        DBLob lob1  = cl.createLob();
        try {
            lob1.write(writeData.getBytes());
        }catch (BaseException e){
            Assert.assertEquals(0,e.getErrorCode());
        }finally {
            lob1.close();
        }

        // case 2, read data from lob after create.
        DBLob lob2  = cl.createLob();
        try {
            readData = new byte[(int)lob2.getSize()];
            lob2.read(readData);
        }catch (BaseException e){
            Assert.assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(),e.getErrorCode());
        }finally {
            lob2.close();
        }

        // case 3, open lob with read mode, write data to lob after read.
        DBLob lob3  = cl.openLob(oid, DBLob.SDB_LOB_READ );
        try {
            readData = new byte[(int)lob3.getSize()];
            lob3.read(readData);
            lob3.write(writeData.getBytes());
        }catch ( BaseException e ){
            Assert.assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode());
        }finally {
            lob3.close();
        }

        // case 4, open lob with share read mode, write data to lob after read.
        DBLob lob4  = cl.openLob(oid, DBLob.SDB_LOB_READ );
        try {
            readData = new byte[(int)lob4.getSize()];
            lob4.read(readData);
            lob4.write(writeData.getBytes());
        }catch ( BaseException e ){
            Assert.assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode());
        }finally {
            lob4.close();
        }

        // case 5, open lob with write mode, read data from lob after write.
        DBLob lob5  = cl.openLob(oid, DBLob.SDB_LOB_WRITE);
        try {
            lob5.write(writeData.getBytes());
            readData = new byte[(int)lob5.getSize()];
            lob5.read(readData);
        }catch ( BaseException e ){
            Assert.assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode());
        }finally {
            lob5.close();
        }

        // case 6, open lob with readAndWrite mode, then read/write.
        DBLob lob6  = cl.openLob(oid, (DBLob.SDB_LOB_SHAREREAD | DBLob.SDB_LOB_WRITE ));
        try {
            lob6.write(writeData.getBytes());
            readData = new byte[(int)lob6.getSize()];
            lob6.read(readData);
        }catch (BaseException e){
            Assert.assertEquals(0, e.getErrorCode());
        }finally {
            lob6.close();
        }
    }

    @Test
    public void isEofTest() {

    }

}
