package com.sequoiadb.test.cursor;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.test.common.Constants;
import com.sequoiadb.test.common.ConstantsInsert;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.junit.*;

import java.util.List;

import static org.junit.Assert.*;


public class CursorTest {
    private static Sequoiadb sdb;
    private static CollectionSpace cs;
    private static DBCollection cl;
    private static DBCursor cursor;
    private static DBCursor cursor1;
    private static DBCursor cursor2;
    private static DBCursor cursor3;
    private static DBCursor cursor4;
    private static final int totalNum = 99;
    private static BSONObject record;

    @BeforeClass
    public static void setConnBeforeClass() throws Exception {
        sdb = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");
        System.out.println("connect ok!");
        if (sdb.isCollectionSpaceExist(Constants.TEST_CS_NAME_1)) {
            sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
            cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
        } else
            cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
        BSONObject conf = new BasicBSONObject();
        conf.put("ReplSize", 0);
        cl = cs.createCollection(Constants.TEST_CL_NAME_1, conf);
        List<BSONObject> list = ConstantsInsert.createRecordList(totalNum);
        cl.bulkInsert(list, DBCollection.FLG_INSERT_CONTONDUP);
        record = new BasicBSONObject();
        record.put("hello", "world");
        cl.insert(record);

        System.out.println("pre data ok!");
    }

    @AfterClass
    public static void DropConnAfterClass() throws Exception {
        sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
        sdb.disconnect();
        System.out.println("disconnect ok!");
    }

    @Before
    public void setUp() throws Exception {
    }

    @After
    public void tearDown() throws Exception {
    }

    @Test
    public void testHasNext() {
        System.out.println("begin to test testHasNext ...");
        int i = 0;
        cursor = cl.query();
        while (cursor.hasNext()) {
            i++;
            if (i > (totalNum + 1))
                break;
            cursor.getNext();
        }
        cursor.close();
        assertEquals((totalNum + 1), i);
    }

    @Test
    public void testGetCurrent() {
        System.out.println("begin to test testGetCurrent ...");
        cursor = cl.query();
        BSONObject obj = null;
        obj = cursor.getCurrent();
        cursor.close();
        assertTrue(null != obj);
    }


    @Test
    public void testGetNext() {
        System.out.println("begin to test testGetNext ...");
        cursor = cl.query();
        int i = 0;
        while (cursor.getNext() != null) {
            i++;
            if (i > (totalNum + 1))
                break;
        }
        cursor.close();
        assertEquals((totalNum + 1), i);
    }

    @Test
    public void testGetNextRaw() {
        System.out.println("begin to test testGetNextRaw ...");
        BSONObject selector1 = new BasicBSONObject();
        selector1.put("hello", "");
        cursor = cl.query(record, null, null, null);
        System.out.println(cursor.getCurrent());
		cursor.close();
        BSONObject selector = new BasicBSONObject();
        selector.put("hello", "");
        cursor = cl.query(record, null, null, null);
        byte[] arr = null;
        int i = 0;
        while (cursor.hasNext()) {
            arr = cursor.getNextRaw();
            i++;
            if (i > (totalNum + 1))
                break;
        }
        cursor.close();
        assertTrue(null != arr);
    }

    @Test
    public void testRunOutThenCloseCursor() {
        System.out.println("begin to test testRunOutThenCloseCursor ...");
        BSONObject obj = null;
        cursor4 = cl.query("", "", "", "", 0, 1);
        try {
            obj = cursor4.getNext();
            assertTrue(null != obj);
        } catch (BaseException e) {
            assertTrue(false);
        }
        try {
            obj = cursor4.getNext();
            assertTrue(null == obj);
        } catch (BaseException e) {
            assertTrue(false);
        }
        try {
            obj = cursor4.getNext();
            assertTrue(null == obj);
        } catch (BaseException e) {
            assertTrue(false);
        }
        try {
            cursor4.close();
        } catch (BaseException e) {
            assertTrue(false);
        }
    }

    @Test
    public void testCloseCursor() {
        System.out.println("begin to test testCloseCursor ...");
        cursor = cl.query();
        BSONObject obj = null;
        byte[] arr = null;
        cursor.close();
        try {
            obj = cursor.getCurrent();
        } catch (BaseException e) {
            assertTrue(e.getErrorType().equals("SDB_RTN_CONTEXT_NOTEXIST"));
        }
        try {
            obj = cursor.getNext();
        } catch (BaseException e) {
            assertTrue(e.getErrorType().equals("SDB_RTN_CONTEXT_NOTEXIST"));
        }
        try {
            arr = cursor.getNextRaw();
        } catch (BaseException e) {
            assertTrue(e.getErrorType().equals("SDB_RTN_CONTEXT_NOTEXIST"));
        }
    }


    @Test
    public void testCloseAllCursor() {
        System.out.println("begin to test testCloseAllCursor ...");
        cursor = cl.query();
        cursor1 = cl.query();
        cursor2 = cl.query();
        cursor3 = cl.query();
        BSONObject obj = null;
        byte[] arr = null;
        cursor.close();
        cursor1.getCurrent();
        cursor2.getNext();
        cursor3.getNextRaw();
        sdb.closeAllCursors();
        try {
            obj = cursor.getCurrent();
            assertTrue(obj == null);
        } catch (BaseException e) {
            System.out.println("1:" + e.getErrorCode());
            assertTrue(e.getErrorType().equals("SDB_RTN_CONTEXT_NOTEXIST"));
        }
        try {
            obj = cursor1.getNext();
        } catch (BaseException e) {
            System.out.println("2:" + e.getErrorCode());
            assertTrue(e.getErrorType().equals("SDB_RTN_CONTEXT_NOTEXIST"));
        }
        try {
            arr = cursor2.getNextRaw();
            assertNotNull(arr);
        } catch (BaseException e) {
            System.out.println("3:" + e.getErrorCode());
            assertTrue(e.getErrorType().equals("SDB_RTN_CONTEXT_NOTEXIST"));
        }
    }
}
