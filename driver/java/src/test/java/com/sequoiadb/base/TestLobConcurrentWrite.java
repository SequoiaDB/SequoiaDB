package com.sequoiadb.base;

import com.sequoiadb.test.SingleCSCLTestCase;
import com.sequoiadb.test.TestConfig;
import org.bson.BSONObject;
import org.bson.types.ObjectId;
import org.junit.Before;
import org.junit.Test;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Random;
import java.util.concurrent.ConcurrentLinkedQueue;

import static org.junit.Assert.*;

public class TestLobConcurrentWrite extends SingleCSCLTestCase {
    private static final String FIELD_HAS_PIECES_INFO = "HasPiecesInfo";

    @Before
    public void setUp() {
        cl.truncate();
    }

    static class LobSeekWriter {
        private int index;
        private DBLob lob;
        private byte[] data;
        private int offset;
        private int length;

        LobSeekWriter(int index, DBLob lob, byte[] data, int offset, int length) {
            this.index = index;
            this.lob = lob;
            this.data = data;
            this.offset = offset;
            this.length = length;
        }

        void write() {
            System.out.println(
                String.format("Index[%d]: offset=%d, length=%d",
                    index, offset, length));
            lob.seek(offset, DBLob.SDB_LOB_SEEK_SET);
            lob.write(data, offset, length);
        }
    }

    static class LobWriter {
        private int index;
        private String csName;
        private String clName;
        private ObjectId id;
        private byte[] data;
        private int offset;
        private int length;

        LobWriter(int index, String csName, String clName, ObjectId id, byte[] data, int offset, int length) {
            this.index = index;
            this.csName = csName;
            this.clName = clName;
            this.id = id;
            this.data = data;
            this.offset = offset;
            this.length = length;
        }

        void write() {
            System.out.println(
                String.format("Index[%d]: offset=%d, length=%d",
                    index, offset, length));
            Sequoiadb sdb = new Sequoiadb(TestConfig.getSingleHost(),
                Integer.valueOf(TestConfig.getSinglePort()),
                TestConfig.getSingleUsername(),
                TestConfig.getSinglePassword());
            try {
                DBCollection cl = sdb.getCollectionSpace(csName)
                    .getCollection(clName);
                DBLob lob = cl.openLob(id, DBLob.SDB_LOB_WRITE);
                try {
                    lob.lockAndSeek(offset, length);
                    lob.write(data, offset, length);
                } finally {
                    lob.close();
                }
            } catch (Exception e) {
                throw new RuntimeException(
                    String.format("Index[%d]: offset=%d, length=%d", index, offset, length), e);
            } finally {
                sdb.close();
            }
        }
    }

    @Test
    public void testCreateSeekWrite() {
        int bytesNum = 1024 * 1024 * 5;
        byte[] bytes = new byte[bytesNum];
        Random rand = new Random();
        rand.nextBytes(bytes);

        ObjectId id = ObjectId.get();
        DBLob lob = cl.createLob(id);

        ArrayList<LobSeekWriter> list = new ArrayList<LobSeekWriter>();
        int step = 1024 * 211 - 10 + new Random().nextInt(20);
        int offset = 0;
        int taskId = 0;
        while (offset < bytesNum) {
            int length = step;
            if (length > bytesNum - offset) {
                length = bytesNum - offset;
            }
            list.add(
                new LobSeekWriter(taskId++, lob, bytes, offset, length));
            offset += length;
        }

        Collections.shuffle(list);

        PerfTimer timer = new PerfTimer();
        timer.start();

        for (LobSeekWriter writer : list) {
            writer.write();
        }
        lob.close();

        timer.stop();
        System.out.println(
            String.format("Seek writing lob costs %dms in %d task(s)",
                timer.duration(), taskId));

        DBCursor cursor = cl.listLobs();
        assertTrue(cursor.hasNext());
        BSONObject obj = cursor.getNext();
        ObjectId oid = (ObjectId) obj.get("Oid");
        assertEquals(id, oid);
        if (obj.containsField(FIELD_HAS_PIECES_INFO)) {
            Boolean hasPiecesInfo = (Boolean) obj.get(FIELD_HAS_PIECES_INFO);
            assertFalse(hasPiecesInfo);
        }
        assertFalse(cursor.hasNext());

        lob = cl.openLob(id);
        assertEquals(bytesNum, lob.getSize());
        byte[] bytes2 = new byte[(int) lob.getSize()];
        lob.read(bytes2);
        lob.close();

        assertArrayEquals(bytes, bytes2);

        cl.removeLob(id);
        cursor = cl.listLobs();
        assertFalse(cursor.hasNext());
    }

    @Test
    public void testOpenSeekWrite() {
        int bytesNum = 1024 * 1024 * 5;
        byte[] bytes = new byte[bytesNum];
        Random rand = new Random();
        rand.nextBytes(bytes);

        ObjectId id = ObjectId.get();
        DBLob lob = cl.createLob(id);
        lob.close();

        ArrayList<LobWriter> list = new ArrayList<LobWriter>();
        int step = 1024 * 211 - 10 + new Random().nextInt(20);
        int offset = 0;
        int taskId = 0;
        while (offset < bytesNum) {
            int length = step;
            if (length > bytesNum - offset) {
                length = bytesNum - offset;
            }
            list.add(
                new LobWriter(taskId++, cl.getCSName(), cl.getName(), id, bytes, offset, length));
            offset += length;
        }
        Collections.shuffle(list);
        ConcurrentLinkedQueue<LobWriter> queue = new ConcurrentLinkedQueue<LobWriter>(list);

        PerfTimer timer = new PerfTimer();
        timer.start();

        while (true) {
            LobWriter writer = queue.poll();
            if (writer == null) {
                break;
            }

            writer.write();
        }

        timer.stop();
        System.out.println(
            String.format("open seek writing lob costs %dms in %d task(s)",
                timer.duration(), taskId));

        DBCursor cursor = cl.listLobs();
        assertTrue(cursor.hasNext());
        BSONObject obj = cursor.getNext();
        ObjectId oid = (ObjectId) obj.get("Oid");
        assertEquals(id, oid);
        if (obj.containsField(FIELD_HAS_PIECES_INFO)) {
            Boolean hasPiecesInfo = (Boolean) obj.get(FIELD_HAS_PIECES_INFO);
            assertFalse(hasPiecesInfo);
        }
        assertFalse(cursor.hasNext());

        lob = cl.openLob(id);
        assertEquals(bytesNum, lob.getSize());
        byte[] bytes2 = new byte[(int) lob.getSize()];
        lob.read(bytes2);
        lob.close();

        assertArrayEquals(bytes, bytes2);

        cl.removeLob(id);
        cursor = cl.listLobs();
        assertFalse(cursor.hasNext());
    }

    @Test
    public void testConcurrentSeekWrite() {
        int bytesNum = 1024 * 1024 * 5;
        byte[] bytes = new byte[bytesNum];
        Random rand = new Random();
        rand.nextBytes(bytes);

        ObjectId id = ObjectId.get();
        DBLob lob = cl.createLob(id);
        lob.close();

        ArrayList<LobWriter> list = new ArrayList<LobWriter>();
        int step = 1024 * 211 - 10 + new Random().nextInt(20);
        int offset = 0;
        int taskId = 0;
        while (offset < bytesNum) {
            int length = step;
            if (length > bytesNum - offset) {
                length = bytesNum - offset;
            }
            list.add(
                new LobWriter(taskId++, cl.getCSName(), cl.getName(), id, bytes, offset, length));
            offset += length;
        }
        Collections.shuffle(list);
        final ConcurrentLinkedQueue<LobWriter> queue = new ConcurrentLinkedQueue<LobWriter>(list);

        final int threadNum = Math.min(7, list.size());
        Thread[] threads = new Thread[threadNum];
        for (int i = 0; i < threadNum; i++) {
            threads[i] = new Thread(
                new Runnable() {
                    @Override
                    public void run() {
                        while (true) {
                            LobWriter writer = queue.poll();
                            if (writer == null) {
                                break;
                            }

                            writer.write();
                        }
                    }
                }
            );
        }

        PerfTimer timer = new PerfTimer();
        timer.start();

        for (Thread thread : threads) {
            thread.start();
        }

        for (Thread thread : threads) {
            try {
                thread.join();
            } catch (InterruptedException e) {
                throw new RuntimeException(e);
            }
        }

        timer.stop();
        System.out.println(
            String.format("Concurrent writing lob costs %dms in %d task(s) with %d threads",
                timer.duration(), taskId, threadNum));

        DBCursor cursor = cl.listLobs();
        assertTrue(cursor.hasNext());
        BSONObject obj = cursor.getNext();
        ObjectId oid = (ObjectId) obj.get("Oid");
        assertEquals(id, oid);
        if (obj.containsField(FIELD_HAS_PIECES_INFO)) {
            Boolean hasPiecesInfo = (Boolean) obj.get(FIELD_HAS_PIECES_INFO);
            assertFalse(hasPiecesInfo);
        }
        assertFalse(cursor.hasNext());

        lob = cl.openLob(id);
        assertEquals(bytesNum, lob.getSize());
        byte[] bytes2 = new byte[(int) lob.getSize()];
        lob.read(bytes2);
        lob.close();

        assertArrayEquals(bytes, bytes2);

        cl.removeLob(id);
        cursor = cl.listLobs();
        assertFalse(cursor.hasNext());
    }
}
