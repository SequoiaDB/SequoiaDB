package com.sequoiadb.test.cursor;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.test.common.Constants;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.junit.*;

import java.util.ArrayList;
import java.util.List;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;

public class DBCursorTest {
    private static Sequoiadb sdb;
    private static CollectionSpace cs;
    private static DBCollection cl;
    private static DBCursor cursor;

    @BeforeClass
    public static void setUpBeforeClass() throws Exception {
        sdb = new Sequoiadb(Constants.COOR_NODE_CONN, null, null);

        if (sdb.isCollectionSpaceExist(Constants.TEST_CS_NAME_1)) {
            sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
            cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
        } else
            cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
        BSONObject conf = new BasicBSONObject();
        conf.put("ReplSize", 0);
        cl = cs.createCollection(Constants.TEST_CL_NAME_1, conf);

        assertNotNull(cl);
        List<BSONObject> list = createNameList(10);
        cl.bulkInsert(list, DBCollection.FLG_INSERT_CONTONDUP);
    }

    @AfterClass
    public static void tearDownAfterClass() throws Exception {
        cl.delete("");
        cursor.close();
        cursor = null;
        sdb.disconnect();
    }

    @Before
    public void setUp() throws Exception {
        cursor = cl.query();
    }

    @After
    public void tearDown() throws Exception {
    }

    @Test
    public void testHasNext() {
        int i = 0;
        while (cursor.hasNext()) {
            i++;
            cursor.getNext();
        }
        assertEquals(i, 10);
    }

    @Test
    public void testGetNext() {
        int i = 0;
        while (cursor.getNext() != null)
            i++;
        assertEquals(i, 10);
    }

    /*
        @Test
        public void testUpdateCurrent() {
            cursor.getNext();
            BSONObject current = new BasicBSONObject();
            ObjectId currentId = (ObjectId) cursor.getCurrent().get("_id");
            current.put("_id", currentId);
            BSONObject modifier = new BasicBSONObject();
            BSONObject m = new BasicBSONObject();
            m.put("Age", 20);
            modifier.put("$set", m);
            cursor.updateCurrent(modifier, null);
            cursor = dbc.query(current, null, null, null);
            assertEquals(cursor.getNext().get("Age"), 20);
        }

        @Test
        public void testDelCurrent() {
            cursor.getNext();
            BSONObject current = new BasicBSONObject();
            ObjectId currentId = (ObjectId) cursor.getCurrent().get("_id");
            current.put("_id", currentId);
            cursor.deleteCurrent();
            cursor = null;
            cursor = dbc.query(current, null, null, null);
            assertFalse(cursor.hasNext());
        }
    */
    private static List<BSONObject> createNameList(int listSize) {
        List<BSONObject> list = null;
        if (listSize <= 0) {
            return list;
        }
        try {
            list = new ArrayList<BSONObject>(listSize);
            for (int i = 0; i < listSize; i++) {
                BSONObject obj = new BasicBSONObject();
                BSONObject addressObj = new BasicBSONObject();
                BSONObject phoneObj = new BasicBSONObject();

                addressObj.put("StreetAddress", "21 2nd Street");
                addressObj.put("City", "New York");
                addressObj.put("State", "NY");
                addressObj.put("PostalCode", "10021");

                phoneObj.put("Type", "Home");
                phoneObj.put("Number", "212 555-1234");

                obj.put("FirstName", "John");
                obj.put("LastName", "Smith");
                obj.put("Age", "50");
                obj.put("Id", i);
                obj.put("Address", addressObj);
                obj.put("PhoneNumber", phoneObj);

                list.add(obj);
            }
        } catch (Exception e) {
            System.out.println("Failed to create name list record.");
            e.printStackTrace();
        }
        return list;
    }

}
