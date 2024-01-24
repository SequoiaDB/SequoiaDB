package com.sequoiadb.test.cl;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.test.common.Constants;
import com.sequoiadb.test.common.ConstantsInsert;
import com.sequoiadb.testdata.SDBTestHelper;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.junit.*;

import java.util.List;

import static org.junit.Assert.*;


public class CreateGetDropIndex {
    private static Sequoiadb sdb;
    private static CollectionSpace cs;
    private static DBCollection cl;
    private static DBCursor cursor;

    @BeforeClass
    public static void setConnBeforeClass() throws Exception {
        sdb = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");

    }

    @AfterClass
    public static void DropConnAfterClass() throws Exception {
        sdb.disconnect();
    }

    @Before
    public void setUp() throws Exception {
        // cs
        if (sdb.isCollectionSpaceExist(Constants.TEST_CS_NAME_1)) {
            sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
            cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
        } else
            cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
        // cl
        BSONObject conf = new BasicBSONObject();
        conf.put("ReplSize", 0);
        cl = cs.createCollection(Constants.TEST_CL_NAME_1, conf);
    }

    @After
    public void tearDown() throws Exception {
        sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
    }

    @Test
    public void testCreateIndex() {
        BSONObject index = new BasicBSONObject();
        index.put("Id", 1);
        cl.createIndex(Constants.TEST_INDEX_NAME, index, false, false);
        SDBTestHelper.waitIndexCreateFinish(cl, Constants.TEST_INDEX_NAME, 100);
        DBCursor cursor = cl.getIndex(Constants.TEST_INDEX_NAME);
        if (cursor == null || !cursor.hasNext())
            assertTrue(false);
        else {
            BSONObject obj = cursor.getNext();
            if (obj != null) {
                assertEquals(
                    ((BSONObject) obj.get(Constants.IXM_INDEXDEF))
                        .get(Constants.IXM_NAME),
                    Constants.TEST_INDEX_NAME);
            } else
                assertTrue(false);
        }
    }

    @Test(expected = BaseException.class)
    public void testCreateSameIndex() {
        // ceate index
        BSONObject index = new BasicBSONObject();
        index.put("Id", 1);
        cl.createIndex(Constants.TEST_INDEX_NAME, index, false, false);
        // TODO:
        BSONObject sameindex = new BasicBSONObject();
        sameindex.put("Id", 1);
        cl.createIndex(Constants.TEST_INDEX_NAME, sameindex, false, false);
    }

    @Test
    public void testGetIndex() {
        // ceate index
        BSONObject index = new BasicBSONObject();
        index.put("Id", 1);
        cl.createIndex(Constants.TEST_INDEX_NAME, index, false, false);
        SDBTestHelper.waitIndexCreateFinish(cl, Constants.TEST_INDEX_NAME, 100);
        BSONObject ret = cl.getIndex(Constants.TEST_INDEX_NAME).getNext();
        BSONObject def = (BSONObject) ret.get(Constants.IXM_INDEXDEF);
        assertEquals(def.get(Constants.IXM_NAME), Constants.TEST_INDEX_NAME);
    }

    @Test
    public void testGetIndexes() {
        // ceate index
        BSONObject index = new BasicBSONObject();
        index.put("Id", 1);
        cl.createIndex(Constants.TEST_INDEX_NAME, index, false, false);
        SDBTestHelper.waitIndexCreateFinish(cl, Constants.TEST_INDEX_NAME, 100);
        // TODO:
        DBCursor cursor = cl.getIndexes();
        int count = 0;
        while (cursor.hasNext()) {
            assertNotNull(cursor.getNext());
            count++;
        }
        assertEquals(2, count);
    }

    @Test
    public void testDropIndex() {
        // ceate index
        BSONObject index = new BasicBSONObject();
        index.put("Id", 1);
        cl.createIndex(Constants.TEST_INDEX_NAME, index, false, false);
        // TODO:
        cl.dropIndex(Constants.TEST_INDEX_NAME);
        SDBTestHelper.waitIndexDropFinish(cl, Constants.TEST_INDEX_NAME, 100);
        DBCursor cursor = cl.getIndex(Constants.TEST_INDEX_NAME);
        assertFalse(cursor.hasNext());
    }

    @Test(expected = BaseException.class)
    public void testIllegalIndex() {
        BSONObject key = new BasicBSONObject();
        BSONObject key1 = new BasicBSONObject();
        BSONObject key2 = new BasicBSONObject();
        BSONObject key3 = new BasicBSONObject();
        key.put("Id", 1);
        key1.put("<Id>", 1);
        key2.put("Id", "");
        key3.put("$Id", "");
        cl.createIndex("test_index", key1, false, false);
        cl.createIndex("test_index", key2, false, false);
        cl.createIndex("test_index", key3, false, false);
        cl.createIndex("", key, false, false);
        cl.createIndex("$test_index", key, false, false);
    }

    @Test
    public void testCreateUniqueIndex() {
        BSONObject key_field = new BasicBSONObject();
        key_field.put("Id", 1);
        cl.createIndex(Constants.TEST_INDEX_NAME, key_field, true, false);
        BSONObject obj = new BasicBSONObject();
        obj.put("Id", 1);
        obj.put("name", "hu");
        cl.insert(obj);
    }

    //after create a unique index for the filed name Id ,then can not insert two same records
    @Test(expected = BaseException.class)
    public void testInsertSameRecord() {
        BSONObject key_field = new BasicBSONObject();
        key_field.put("Id", 1);
        cl.createIndex(Constants.TEST_INDEX_NAME, key_field, true, false);
        BSONObject obj = new BasicBSONObject();
        obj.put("Id", 1);
        obj.put("name", "hu");
        cl.insert(obj);
        // TODO:
        BSONObject obj1 = new BasicBSONObject();
        obj1.put("Id", 1);
        obj1.put("name", "shan");
        cl.insert(obj1);
    }

    @Test
    public void testInsertRecord() {
        BSONObject key_field = new BasicBSONObject();
        key_field.put("Id", 1);
        cl.createIndex(Constants.TEST_INDEX_NAME, key_field, true, false);
        BSONObject obj = new BasicBSONObject();
        obj.put("Id", 1);
        obj.put("name", "hu");
        cl.insert(obj);
        // TODO:
        BSONObject obj2 = new BasicBSONObject();
        obj2.put("Id", 2);
        obj2.put("name", "hu");
        cl.insert(obj2);
    }

    @Test
    public void testDeleteRecords() {
        BSONObject key_field = new BasicBSONObject();
        key_field.put("Id", 1);
        cl.createIndex(Constants.TEST_INDEX_NAME, key_field, true, false);
        BSONObject obj = new BasicBSONObject();
        obj.put("Id", 1);
        obj.put("name", "hu");
        cl.insert(obj);
        BSONObject obj2 = new BasicBSONObject();
        obj2.put("Id", 2);
        obj2.put("name", "hu");
        cl.insert(obj2);
        // TODO:
        BSONObject matcher = new BasicBSONObject();
        BSONObject matcher2 = new BasicBSONObject();
        matcher.put("Id", 1);
        matcher2.put("Id", 2);
        cl.delete(matcher);
        cl.delete(matcher2);
        cursor = cl.query();
        assertFalse(cursor.hasNext());
    }

    /*
     * insert 100 records ,{Id:i,str:"foo"+i},and create unique index on the field name Id
     * insert success and create index sucess
     */
    @Test
    public void testInsertAndCreateUniqueIndex() {
        List<BSONObject> list = ConstantsInsert.createRecordList(100);
        cl.bulkInsert(list, DBCollection.FLG_INSERT_CONTONDUP);
        BSONObject Id_field = new BasicBSONObject();
        Id_field.put("Id", 1);
        cl.createIndex("unique_index_id", Id_field, true, false);
        DBCursor cursor = cl.query();
        int size = 0;
        while (cursor.hasNext()) {
            cursor.getNext();
            size++;
        }
        assertEquals(100, size);

        BSONObject index_name = cl.getIndex("unique_index_id").getNext();
        BSONObject def = (BSONObject) index_name.get(Constants.IXM_INDEXDEF);
        assertEquals(def.get(Constants.IXM_NAME), "unique_index_id");

        BSONObject matcher3 = new BasicBSONObject();
        BSONObject con = new BasicBSONObject();
        matcher3.put("Id", con);
        con.put("$gte", 0);
        con.put("$lte", 99);
        cl.delete(matcher3);

        cl.dropIndex("unique_index_id");
    }

    /*
     * insert 100 records ,{Id:10,..},and create unique index on the field name Id
     * insert success but create index failed
     */
    @Test(expected = BaseException.class)
    public void testInsertSameAndCreateUniqueIndex() {
        for (int i = 0; i < 100; i++) {
            BSONObject objOne = ConstantsInsert.createOneRecord();
            cl.insert(objOne);
        }
        DBCursor cursor = cl.query();
        int size = 0;
        while (cursor.hasNext()) {
            cursor.getNext();
            size++;
        }
        assertEquals(100, size);
        BSONObject Id_key = new BasicBSONObject();
        Id_key.put("Id", 1);
        cl.createIndex("uniqueindexid", Id_key, true, false);

        DBCursor cursor1 = cl.getIndex("uniqueindexid");
        assertFalse(cursor1.hasNext());
    }
}
