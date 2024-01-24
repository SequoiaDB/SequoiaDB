package com.sequoiadb.test.cl;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.test.common.Constants;
import com.sequoiadb.test.common.SdbNest;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.junit.*;

import static org.junit.Assert.assertEquals;


public class CLInsertNest {
    private static Sequoiadb sdb;
    private static CollectionSpace cs;
    private static DBCollection cl;
    private static DBCursor cursor;

    @BeforeClass
    public static void setConnBeforeClass() throws Exception {
        // sdb
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
    public void testInsertArray() {
        BSONObject obj = new BasicBSONObject();
        int[] int_arr = {10, 20, 30, 50};
        String[] str_arr = {"field1", "field2", "field3"};

        obj.put("int_arr", int_arr);
        obj.put("str_arr", str_arr);

        cl.insert(obj);
        cursor = cl.query(obj, null, null, null);
        int i = 0;
        while (cursor.hasNext()) {
            cursor.getNext();
            i++;
        }
        assertEquals(1, i);
    }

    @Test
    public void testInsertNestElement() {
        BSONObject obj = new BasicBSONObject();
        String str = SdbNest.createNestElement(100);
        BSONObject str_bson = (BSONObject) JSON.parse(str);
        obj.put("nestElement", str_bson);
        cl.insert(obj);
        cursor = cl.query(obj, null, null, null);
        int i = 0;
        while (cursor.hasNext()) {
            cursor.getNext();
            i++;
        }
        assertEquals(1, i);
    }

    @Test
    public void testInsertNestArray() {
        BSONObject obj = new BasicBSONObject();
        String arr = SdbNest.createNestArray(100);
        BSONObject arr_bson = (BSONObject) JSON.parse(arr);
        obj.put("nestArray", arr_bson);
        cl.insert(obj);
        cursor = cl.query(obj, null, null, null);
        int i = 0;
        while (cursor.hasNext()) {
            cursor.getNext();
            i++;
        }
        assertEquals(1, i);
    }
}
