package com.sequoiadb.test.cursor;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.test.common.Constants;
import com.sequoiadb.test.common.ConstantsInsert;
import org.bson.BSON;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.junit.*;

import java.util.List;

import static org.junit.Assert.*;


public class CursorTest {
    private static Sequoiadb sdb;
    private static DBCollection cl;
    private static BSONObject record;

    @BeforeClass
    public static void setConnBeforeClass() throws Exception {
        // sdb
        sdb = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");

        // cs
        if (sdb.isCollectionSpaceExist(Constants.TEST_CS_NAME_1)) {
            sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
        }
        CollectionSpace cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);

        // cl
        cl = cs.createCollection(Constants.TEST_CL_NAME_1);

        //insert 100 records
        List<BSONObject> list = ConstantsInsert.createRecordList(100);
        cl.bulkInsert(list);

        record = new BasicBSONObject();
        record.put("hello", "world");
        cl.insertRecord(record);
    }

    @AfterClass
    public static void DropConnAfterClass() throws Exception {
        try {
            sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
        } finally {
            sdb.close();
        }
    }

    @Before
    public void setUp() throws Exception {
    }

    @After
    public void tearDown() throws Exception {
    }

    @Test
    public void testHasNext() {
        long totalNum = cl.getCount();
        int i = 0;
        DBCursor cursor = cl.query();
        while (cursor.hasNext()) {
            i++;
            if (i > totalNum)
                break;
            cursor.getNext();
        }
        cursor.close();
        assertEquals(totalNum, i);
    }

    @Test
    public void testGetCurrent() {
        // case 1: getCurrent() before close()
        DBCursor cursor = cl.query();
        BSONObject obj = cursor.getCurrent();
        cursor.close();
        assertNotNull(obj);

        // case 2: getCurrent() after close()
        cursor = cl.query();cursor.getNext();
        cursor.close();
        try {
            cursor.getCurrent();
            Assert.fail("We can't get data from a closed cursor");
        } catch (BaseException e) {
            Assert.assertEquals(SDBError.SDB_DMS_CONTEXT_IS_CLOSE.getErrorCode(), e.getErrorCode());
        }
    }


    @Test
    public void testGetNext() {
        long totalNum = cl.getCount();
        DBCursor cursor = cl.query();
        int i = 0;
        while (cursor.getNext() != null) {
            i++;
            if (i > totalNum)
                break;
        }
        cursor.close();
        assertEquals(totalNum, i);
    }

    @Test
    public void testGetNextRaw() {
        BSONObject data1;
        BSONObject data2;

        try (DBCursor cursor = cl.query(record, null, null, null)) {
            data1 = cursor.getNext();
        }

        try (DBCursor cursor = cl.query(record, null, null, null)) {
            byte[] arr = cursor.getNextRaw();
            data2 = BSON.decode(arr);
        }

        Assert.assertEquals(data1, data2);
    }

    @Test
    public void testRunOutThenCloseCursor() {
        BSONObject obj;
        // only one record
        DBCursor cursor = cl.query(record, null, null, null, 0, 1);

        obj = cursor.getNext();
        assertNotNull(obj);

        obj = cursor.getNext();
        assertNull(obj);

        cursor.close();
    }

    @Test
    public void testCloseCursor() {
        DBCursor cursor = cl.query();
        cursor.close();

        // get record again
        try {
            cursor.getCurrent();
            Assert.fail("We can't get data from a closed cursor");
        } catch (BaseException e) {
            Assert.assertEquals(SDBError.SDB_DMS_CONTEXT_IS_CLOSE.getErrorCode(), e.getErrorCode());
        }

        try {
            cursor.getNext();
            Assert.fail("We can't get data from a closed cursor");
        } catch (BaseException e) {
            Assert.assertEquals(SDBError.SDB_DMS_CONTEXT_IS_CLOSE.getErrorCode(), e.getErrorCode());
        }

        try {
            cursor.getNextRaw();
            Assert.fail("We can't get data from a closed cursor");
        } catch (BaseException e) {
            Assert.assertEquals(SDBError.SDB_DMS_CONTEXT_IS_CLOSE.getErrorCode(), e.getErrorCode());
        }
    }


    @Test
    public void testCloseAllCursor() {
        BSONObject obj;
        byte[] arr;

        DBCursor cursor1 = cl.query();
        DBCursor cursor2 = cl.query();

        cursor1.close();

        obj = cursor2.getCurrent();
        assertNotNull(obj);

        obj = cursor2.getNext();
        assertNotNull(obj);

        arr = cursor2.getNextRaw();
        assertNotNull(arr);

        sdb.closeAllCursors();

        // get record again
        try {
            cursor1.getCurrent();
            Assert.fail("We can't get data from a closed cursor");
        } catch (BaseException e) {
            Assert.assertEquals(SDBError.SDB_DMS_CONTEXT_IS_CLOSE.getErrorCode(), e.getErrorCode());
        }

        try {
            obj = cursor2.getNext();
            assertNotNull(obj);
        } catch (BaseException e) {
            Assert.assertEquals(SDBError.SDB_DMS_CONTEXT_IS_CLOSE.getErrorCode(), e.getErrorCode());
        }

        try {
            arr = cursor2.getNextRaw();
            assertNotNull(arr);
        } catch (BaseException e) {
            Assert.assertEquals(SDBError.SDB_DMS_CONTEXT_IS_CLOSE.getErrorCode(), e.getErrorCode());
        }

        DBCursor cursor3 = cl.query();

        obj = cursor3.getCurrent();
        assertNotNull(obj);

        obj = cursor3.getNext();
        assertNotNull(obj);

        arr = cursor3.getNextRaw();
        assertNotNull(arr);
    }
}
