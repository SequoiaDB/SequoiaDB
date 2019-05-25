package com.sequoiadb.test.cl;

import java.util.LinkedList;
import com.sequoiadb.base.*;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.test.common.Constants;
import com.sequoiadb.test.common.ConstantsInsert;
import com.sequoiadb.testdata.SDBTestHelper;
import com.sequoiadb.testdata.TotalReadValue;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.junit.*;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import static org.junit.Assert.*;

public class DBCollectionTest {
    private static Sequoiadb sdb = null;
    private static CollectionSpace cs = null;
    private static DBCollection cl = null;
    private static DBCursor cursor = null;
    private static int NUM = 10;

    @BeforeClass
    public static void beforeClass() throws Exception {
        sdb = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");
        assertNotNull(sdb);
    }

    @AfterClass
    public static void afterClass() throws Exception {
        sdb.disconnect();
    }

    @Before
    public void setUp() throws Exception {
        if (sdb.isCollectionSpaceExist(Constants.TEST_CS_NAME_1)) {
            sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
            cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
        } else
            cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
        BSONObject conf = new BasicBSONObject();
        conf.put("ReplSize", 0);
        cl = cs.createCollection(Constants.TEST_CL_NAME_1, conf);
        assertNotNull(cl);
        if (cursor != null) {
            cursor.close();
            cursor = null;
        }
    }

    @After
    public void tearDown() throws Exception {
        try {
            sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
        } catch (BaseException e) {
            System.out.println(e.getErrorType());
            assertTrue(false);
        }
    }

    @Test
    public void testInsert() {
        BSONObject objCh = createChineseRecord();
        cl.insert(objCh);
        BSONObject query = new BasicBSONObject();
        query.put("Id", 10000);
        cursor = cl.query(query, null, null, null);
        assertEquals(cursor.getNext().get("Id"), 10000);
    }

    @Test
    public void testBulkInsert() {
        int num = 100;
        List<BSONObject> list1 = createNameList(num);
        cl.bulkInsert(list1, DBCollection.FLG_INSERT_CONTONDUP);
        BSONObject query = new BasicBSONObject();
        BSONObject condition = new BasicBSONObject();
        condition.put("$gte", 0);
        condition.put("$lte", 9);
        query.put("Id", condition);
        cursor = cl.query(query, null, null, null);
        int i = 0;
        while (cursor.hasNext()) {
            cursor.getNext();
            i++;
        }
        assertEquals(10, i);

    }

    @Test
    public void testCreateIndex() {
        BSONObject index = new BasicBSONObject();
        index.put("Id", 1);
        cl.createIndex(Constants.TEST_INDEX_NAME, index, false, false);
        SDBTestHelper.waitIndexCreateFinish(cl, Constants.TEST_INDEX_NAME, 50);
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

    @Test
    public void testFind1() {
        try {
            ConstantsInsert.insertRecords(cl, NUM);
        } catch (BaseException e) {
            System.out.println(e.getErrorType());
            assertTrue(false);
        }
        cursor = cl.query();
        assertNotNull(cursor);
        assertTrue(cursor.hasNext());
        while (cursor.hasNext()) {
            cursor.getNext();
        }
    }

    @Test
    public void testFind2() {
        BSONObject index = new BasicBSONObject();
        index.put("Id", 1);
        cl.createIndex(Constants.TEST_INDEX_NAME, index, false, false);
        ConstantsInsert.insertRecords(cl, NUM);
        DBQuery query = new DBQuery();
        BSONObject matcher = new BasicBSONObject();
        BSONObject hint = new BasicBSONObject();
        matcher.put("Id", 0);
        hint.put("", "Id");
        query.setMatcher(matcher);
        cursor = cl.query(query);
        Object o = cursor.getNext().get("Id");
        int v = (Integer) o;
        assertEquals(v, 0);
    }

    @Test
    public void testFind3() {
        ConstantsInsert.insertRecords(cl, NUM);
        BSONObject query = new BasicBSONObject();
        BSONObject condition = new BasicBSONObject();
        BSONObject selector = new BasicBSONObject();
        BSONObject orderBy = new BasicBSONObject();
        BSONObject hint = new BasicBSONObject();

        condition.put("$gte", 0);
        condition.put("$lte", 9);
        query.put("Id", condition);
        selector.put("Id", "");
        selector.put("age", "");
        orderBy.put("Id", -1);
        hint.put("", "Id");

        cursor = cl.query(query, selector, orderBy, hint);
        int i = 9;
        while (cursor.hasNext()) {
            if (!((cursor.getNext().get("Id").toString()).equals(Integer
                .toString(i)))) {
                assertTrue(false);
                break;
            }
            i--;
        }
    }

    @Test
    public void testGetCount() {
        ConstantsInsert.insertRecords(cl, NUM);
        BSONObject condition = new BasicBSONObject();
        BSONObject m = new BasicBSONObject();
        m.put("$gte", 0);
        m.put("$lte", 9);
        condition.put("Id", m);

        long count = cl.getCount(condition);
        assertTrue(count == 10);
    }

    private void getTotalFromSnapShot(DBCursor snapShotCur, String key1,
                                      String key2, TotalReadValue values) {
        while (snapShotCur.hasNext()) {
            BSONObject result = snapShotCur.getNext();
            values.totalIndexRead += (Long) result.get(key1);
            values.totalDataRead += (Long) result.get(key2);
        }
    }

    @Test
    public void GetCount_with_hint() {
        String indexName = "indexfortestgetcount";
        BSONObject index = new BasicBSONObject("Id", 1);
        cl.createIndex(indexName, index, false, false);
        ConstantsInsert.insertRecords(cl, NUM);
        BSONObject condition = new BasicBSONObject();
        BSONObject hint = new BasicBSONObject();
        BSONObject m = new BasicBSONObject();
        m.put("$gte", 0);
        m.put("$lte", 9);
        condition.put("Id", m);
        hint.put("", "Id");

        BSONObject empty = new BasicBSONObject();

        DBCursor cur1 = sdb.getSnapshot(3, empty, empty, empty);
        long count = cl.getCount(condition, hint);
        DBCursor cur2 = sdb.getSnapshot(3, empty, empty, empty);

        assertTrue(count == 10);
        TotalReadValue values1 = new TotalReadValue();
        getTotalFromSnapShot(cur1, "TotalIndexRead", "TotalDataRead", values1);

        TotalReadValue values2 = new TotalReadValue();
        ;
        getTotalFromSnapShot(cur2, "TotalIndexRead", "TotalDataRead", values2);

        System.out.println("insert record num = " + NUM + ", totalIndexRead1 = "
            + values1.totalIndexRead + ", totalIndexRead2 = "
            + values2.totalIndexRead);
        assertTrue(NUM == values2.totalIndexRead - values1.totalIndexRead);

        System.out.println("totalDataRead1 = " + values1.totalDataRead +
            ", totalDataRead2 = " + values2.totalDataRead);
        assertTrue(0 == values2.totalDataRead - values1.totalDataRead);
    }

    @Test
    public void testDelete() {
        ConstantsInsert.insertRecords(cl, NUM);
        BSONObject matcher = new BasicBSONObject();
        matcher.put("Id", 0);
        cl.delete(matcher);
        cursor = cl.query(matcher, null, null, null);
        assertNull(cursor.getNext());
    }

    @Test
    public void testDeleteByHint() {
        BSONObject index = new BasicBSONObject();
        index.put("Id", 1);
        cl.createIndex(Constants.TEST_INDEX_NAME, index, false, false);
        ConstantsInsert.insertRecords(cl, NUM);
        BSONObject matcher = new BasicBSONObject();
        BSONObject hint = new BasicBSONObject();
        matcher.put("Id", 1);
        hint.put("", "Id");
        cl.delete(matcher, hint);
        cursor = cl.query(matcher, null, null, hint);
        assertNull(cursor.getNext());
    }

    @Test
    public void testGetIndex() {
        BSONObject index = new BasicBSONObject();
        index.put("Id", 1);
        cl.createIndex(Constants.TEST_INDEX_NAME, index, false, false);
        ConstantsInsert.insertRecords(cl, NUM);
        BSONObject idx = cl.getIndex(Constants.TEST_INDEX_NAME)
            .getNext();
        BSONObject def = (BSONObject) idx
            .get(Constants.IXM_INDEXDEF);
        assertEquals(def.get(Constants.IXM_NAME),
            Constants.TEST_INDEX_NAME);
    }

    @Test
    public void testGetIndexs() {
        cursor = cl.getIndexes();
        BSONObject idx = cursor.getNext();
        assertNotNull(idx);
        cursor.close();
    }

    @Test
    public void testUpdateByQuery() {
        ConstantsInsert.insertRecords(cl, NUM);
        DBQuery query = new DBQuery();
        BSONObject matcher = new BasicBSONObject();
        BSONObject modifier = new BasicBSONObject();
        BSONObject m = new BasicBSONObject();
        matcher.put("Id", 2);
        m.put("age", 200);
        modifier.put("$set", m);
        query.setMatcher(matcher);
        query.setModifier(modifier);
        cl.update(query);
        cursor = cl.query(matcher, null, null, null);
        assertEquals(cursor.getNext().get("age"), 200);
    }

    @Test
    public void testUpdate() {
        ConstantsInsert.insertRecords(cl, NUM);
        BSONObject matcher = new BasicBSONObject();
        BSONObject modifier = new BasicBSONObject();
        BSONObject m = new BasicBSONObject();
        BSONObject hint = new BasicBSONObject();

        matcher.put("Id", 3);
        m.put("age", 30);
        modifier.put("$set", m);

        cl.update(matcher, modifier, hint);
        cursor = cl.query(matcher, null, null, null);
        BSONObject obj = cursor.getNext();
        assertEquals(obj.get("age"), 30);
    }

    @Test
    public void testUpsert() {
        ConstantsInsert.insertRecords(cl, NUM);
        BSONObject matcher = new BasicBSONObject();
        BSONObject modifier = new BasicBSONObject();
        BSONObject m = new BasicBSONObject();
        BSONObject hint = new BasicBSONObject();

        matcher.put("Id", 5);
        m.put("age", 80);
        modifier.put("$set", m);

        cl.upsert(matcher, modifier, hint);
        cursor = cl.query(matcher, null, null, null);
        BSONObject obj = cursor.getNext();
        assertEquals(obj.get("age"), 80);
    }

    @Test
    public void testDropIndex() {
        BSONObject index = new BasicBSONObject();
        index.put("Id", 1);
        cl.createIndex(Constants.TEST_INDEX_NAME, index, false, false);
        cl.dropIndex(Constants.TEST_INDEX_NAME);
        SDBTestHelper.waitIndexDropFinish(cl, Constants.TEST_INDEX_NAME, 100);
        BSONObject idx = cl.getIndex(Constants.TEST_INDEX_NAME)
            .getNext();
        assertNull(idx);
    }

    /*
        @Ignore
        @Test
        public void testRename() {
            cl.rename(Constants.TEST_CL_NEW_NAME);
            cl = null;
            cl = sdb.getCollectionSpace(Constants.TEST_CS_NAME_1)
                    .getCollection(Constants.TEST_CL_NEW_NAME);
            cl.rename(Constants.TEST_CL_NAME_1);
            assertNotNull(cl);
        }
    */
    @Ignore
    public void testSplit() {
        String srcGroup = Constants.TEST_RG_NAME_SRC;
        String destGroup = Constants.TEST_RG_NAME_DEST;
        String node2 = Constants.TEST_RN_HOSTNAME_SPLIT + ":"
            + Constants.TEST_RN_PORT_SPLIT;
        String csName = "SplitCS";
        String clName = "SplitCL";
        CollectionSpace cs1;
        if (!sdb.isCollectionSpaceExist(csName))
            cs1 = sdb.createCollectionSpace(csName);
        else
            cs1 = sdb.getCollectionSpace(csName);
        BSONObject shardingKey = new BasicBSONObject();
        shardingKey.put("operation", 1);
        DBCollection cl1;
        if (cs1.isCollectionExist(clName))
            cs1.dropCollection(clName);
        cl1 = cs1.createCollection(clName, shardingKey);
        for (int i = 0; i < 10; i++) {
            String date = new Date().toString();
            BSONObject obj = new BasicBSONObject();
            obj.put("operation", "Split1");
            obj.put("date", date);
            cl1.insert(obj);
        }
        for (int i = 0; i < 10; i++) {
            String date = new Date().toString();
            BSONObject obj = new BasicBSONObject();
            obj.put("operation", "Split2");
            obj.put("date", date);
            cl1.insert(obj);
        }
        BSONObject condition = new BasicBSONObject();
        condition.put("operation", "Split2");
        cl1.split(srcGroup, destGroup, 50.0);

        Sequoiadb sdb2 = sdb.getReplicaGroup(destGroup).getNode(node2)
            .connect();
        CollectionSpace cs2 = sdb2.getCollectionSpace(csName);
        DBCollection cl2 = cs2.getCollection(clName);
        assertEquals(cl2.getCount(condition), 10);

        sdb.dropCollectionSpace(csName);
        sdb2.dropCollectionSpace(csName);
        sdb2.disconnect();
    }

    @Test
    public void testPop() {
        String csName = "cappedCS";
        String clName = "cappedCL";
        CollectionSpace csCapped;
        if (sdb.isCollectionSpaceExist(csName))
        {
            sdb.dropCollectionSpace(csName);
        }

        try {
            BSONObject csOptions = new BasicBSONObject();
            csOptions.put("Capped", true);
            csCapped = sdb.createCollectionSpace(csName, csOptions);
            BSONObject clOptions = new BasicBSONObject();
            clOptions.put("Capped", true);
            clOptions.put("Size", 1024);
            clOptions.put("AutoIndexId", false);
            DBCollection clCapped;
            clCapped = csCapped.createCollection(clName, clOptions);

            LinkedList<BSONObject> insertObjs = new LinkedList<BSONObject>();
            for (int i = 0; i < 32767; i++) {
                BSONObject object = new BasicBSONObject();
                object.put("a", "testaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaatesttesttetestaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaatesttesttetestaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaatesttesttetestaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaatesttesttetestaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaatesttesttetestaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaatesttesttetestaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaatesttesttetestaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaatesttesttetestaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaatesttesttetestaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
                insertObjs.add(object);
            }
            clCapped.bulkInsert(insertObjs, 0);

            BSONObject popOption = new BasicBSONObject();
            popOption.put("LogicalID", 1024);
            clCapped.pop(popOption);
            BSONObject record = clCapped.queryOne();
            assertEquals(2048, ((Long) (record.get("_id"))).longValue());

            BSONObject popOption2 = new BasicBSONObject();
            popOption2.put("LogicalID", 2048);
            popOption2.put("Direction", -1);
            clCapped.pop(popOption2);
            BSONObject record2 = clCapped.queryOne();
            assertNull(record2);
            assertEquals(0, clCapped.getCount());
            clCapped.bulkInsert(insertObjs, 0);
            assertEquals(32767, clCapped.getCount());
        } finally {
            try {
                sdb.dropCollectionSpace(csName);
            } catch (Exception e) {
            }
        }
    }

    private static BSONObject createChineseRecord() {

        BSONObject obj = null;
        try {
            obj = new BasicBSONObject();
            BSONObject obj1 = new BasicBSONObject();
            obj.put("Id", 10000);
            obj.put("姓名", "汤姆");
            obj.put("年龄", 30);
            obj.put("Age", 30);

            obj1.put("0", "123456");
            obj1.put("1", "654321");

            obj.put("电话", obj1);
        } catch (Exception e) {
            System.out.println("Failed to create chinese record.");
            e.printStackTrace();
        }
        return obj;
    }

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
