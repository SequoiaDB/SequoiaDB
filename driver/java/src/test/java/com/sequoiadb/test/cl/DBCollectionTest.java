package com.sequoiadb.test.cl;

import java.util.LinkedList;
import com.sequoiadb.base.*;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.test.common.Constants;
import com.sequoiadb.test.common.ConstantsInsert;
import com.sequoiadb.testdata.SDBTestHelper;
import com.sequoiadb.testdata.TotalReadValue;
import org.bson.BSON;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BSONTimestamp;
import org.bson.types.BasicBSONList;
import org.bson.types.Binary;
import org.bson.util.DateInterceptUtil;
import org.bson.util.JSON;
import org.junit.*;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import static org.junit.Assert.*;

public class DBCollectionTest {
    private static Sequoiadb sdb = null;
    private static CollectionSpace cs = null;
    private static DBCollection cl = null;
    private static DBCollection mainCL = null;
    private static DBCollection subCL1 = null;
    private static DBCollection subCL2 = null;
    private static DBCursor cursor = null;
    private static int NUM = 10;

    @BeforeClass
    public static void beforeClass() throws Exception {
        sdb = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");
        assertNotNull(sdb);
    }

    @AfterClass
    public static void afterClass() throws Exception {
        if (sdb != null) {
            sdb.disconnect();
        }
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
        assertNotNull(cl);
        if (cursor != null) {
            cursor.close();
            cursor = null;
        }

        // create main cl and sub cl
        List<String> subCLName = new ArrayList<>();
        subCLName.add(Constants.TEST_CL_SUB_NAME_1);
        subCLName.add(Constants.TEST_CL_SUB_NAME_2);
        createMainSubCollections(Constants.TEST_CL_MAIN_NAME, subCLName, Constants.SHARDING_KEY);
        mainCL = cs.getCollection(Constants.TEST_CL_MAIN_NAME);
        subCL1 = cs.getCollection(Constants.TEST_CL_SUB_NAME_1);
        subCL2 = cs.getCollection(Constants.TEST_CL_SUB_NAME_2);
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

    public DBCollection createCL(String clName, BSONObject options){
        DBCollection collection;
        if (cs.isCollectionExist(clName)){
            cs.dropCollection(clName);
        }
        collection = cs.createCollection(clName, options);
        return collection;
    }

    public void createMainSubCollections(String mainName, List<String> subNames, String shardingKey){
        DBCollection mainCL;
        BSONObject options = new BasicBSONObject();
        BasicBSONObject attachOptions = new BasicBSONObject();

        // create main collections
        options.put("ShardingKey",new BasicBSONObject(shardingKey, 1));
        options.put("ShardingType","range");
        options.put("IsMainCL",true);
        mainCL = createCL(mainName,options);

        // create sub collection
        int i = 0;
        int scope = 10;
        DBCollection subCL ;
        for (String name :subNames) {
            subCL = createCL(name, null);
            attachOptions.clear();
            attachOptions.put("LowBound",new BasicBSONObject(shardingKey,i++ * scope));
            attachOptions.put("UpBound", new BasicBSONObject(shardingKey,i * scope ));
            mainCL.attachCollection(subCL.getFullName(), attachOptions);
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
        // create index
        BSONObject index = new BasicBSONObject();
        index.put("Id", 1);
        cl.createIndex(Constants.TEST_INDEX_NAME, index, false, false);
        // insert some record
        ConstantsInsert.insertRecords(cl, NUM);
        // test
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
        // insert some record
        ConstantsInsert.insertRecords(cl, NUM);
        // test
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
        // insert some record
        ConstantsInsert.insertRecords(cl, NUM);
        // test
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
        // create index
        String indexName = "indexfortestgetcount";
        BSONObject index = new BasicBSONObject("Id", 1);
        cl.createIndex(indexName, index, false, false);
        // insert some record
        ConstantsInsert.insertRecords(cl, NUM);
        // test
        BSONObject condition = new BasicBSONObject();
        BSONObject hint = new BasicBSONObject();
        BSONObject m = new BasicBSONObject();
        m.put("$gte", 0);
        m.put("$lte", 9);
        condition.put("Id", m);
        hint.put("", "Id");

        BSONObject empty = new BasicBSONObject();

        // test
        DBCursor cur1 = sdb.getSnapshot(3, empty, empty, empty);
        long count = cl.getCount(condition, hint);
        DBCursor cur2 = sdb.getSnapshot(3, empty, empty, empty);

        // check
        assertTrue(count == 10);
        // check totalIndexRead and totalDataRead
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
        // insert some record
        ConstantsInsert.insertRecords(cl, NUM);
        // test
        BSONObject matcher = new BasicBSONObject();
        matcher.put("Id", 0);
        cl.delete(matcher);
        cursor = cl.query(matcher, null, null, null);
        assertNull(cursor.getNext());
    }

    @Test
    public void testDeleteOne() {
        BSONObject matcher ;
        BSONObject hint = new BasicBSONObject();
        int flag = DBCollection.FLG_DELETE_ONE;

        int count = 0;
        int insertTimes = 5;
        BSONObject record = new BasicBSONObject("deleteRecord",1);
        matcher = record;
        //case 1: delete one record on ordinary collection
        ConstantsInsert.insertSameRecords(cl,record,insertTimes);
        cl.delete(matcher,hint,flag);
        DBCursor result = cl.query(matcher,null,null,null,0);
        while ( result.hasNext() ){
            result.getNext();
            count++;
        }
        assertEquals(count,insertTimes - 1 );

        //case 2: delete one on main and sub collection
        ConstantsInsert.insertSameRecords(subCL1,record,insertTimes);
        ConstantsInsert.insertSameRecords(subCL2,record,insertTimes);
        BaseException expectE = new BaseException(0);
        try {
            mainCL.delete(matcher,hint,flag);
        }catch (BaseException e){
            expectE = e;
        }
        assertEquals(expectE.getErrorCode(),SDBError.SDB_COORD_DELETE_MULTI_NODES.getErrorCode() );
    }

    @Test
    public void testDeleteByHint() {
        // create index
        BSONObject index = new BasicBSONObject();
        index.put("Id", 1);
        cl.createIndex(Constants.TEST_INDEX_NAME, index, false, false);
        // insert some record
        ConstantsInsert.insertRecords(cl, NUM);
        // test
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
        // create index
        BSONObject index = new BasicBSONObject();
        index.put("Id", 1);
        cl.createIndex(Constants.TEST_INDEX_NAME, index, false, false);
        // insert some record
        ConstantsInsert.insertRecords(cl, NUM);
        // test
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
        // insert some record
        ConstantsInsert.insertRecords(cl, NUM);
        // test
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
        // insert some record
        ConstantsInsert.insertRecords(cl, NUM);
        // test
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
    public void testUpdateOne() {
        BSONObject matcher ;
        BSONObject hint = new BasicBSONObject();
        BSONObject modifier = new BasicBSONObject();
        BSONObject updateValue = new BasicBSONObject();
        int flag = DBCollection.FLG_UPDATE_ONE;

        String updateKey = "updateRecord";
        String oldValue = "old";
        String newValue = "new";
        int count = 0;
        int insertTimes = 5;
        BSONObject record = new BasicBSONObject(updateKey, oldValue);
        matcher = record;

        //case 1: update one record on ordinary collection
        ConstantsInsert.insertSameRecords(cl, record, insertTimes);
        updateValue .put(updateKey, newValue);
        modifier.put("$set", updateValue);
        cl.update(matcher, modifier, hint, flag);
        DBCursor result = cl.query(updateValue,null,null,null,0);
        while ( result.hasNext() ){
            result.getNext();
            count++;
        }
        assertEquals(count,1);

        //case 2: update one on main and sub collection
        ConstantsInsert.insertSameRecords(subCL1, record, insertTimes);
        ConstantsInsert.insertSameRecords(subCL2, record, insertTimes);
        BaseException expectE = new BaseException(0);
        try {
            mainCL.update(matcher, modifier, hint, flag);
        }catch (BaseException e){
            expectE = e;
        }
        assertEquals(expectE.getErrorCode(),SDBError.SDB_COORD_UPDATE_MULTI_NODES.getErrorCode() );
    }

    @Test
    public void testUpsert() {
        // insert some record
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
    public void testUpsertOne() {
        BSONObject matcher ;
        BSONObject hint = new BasicBSONObject();
        BSONObject modifier = new BasicBSONObject();
        BSONObject upsertValue = new BasicBSONObject();
        int flag = DBCollection.FLG_UPDATE_ONE;

        String upsertKey = "upsertRecord";
        String oldValue = "old";
        String newValue = "new";
        int count = 0;
        int insertTimes = 5;
        BSONObject record = new BasicBSONObject(upsertKey,oldValue);
        matcher = record;
        //insert two record, then upsert one

        ConstantsInsert.insertSameRecords(cl,record,insertTimes);
        upsertValue .put(upsertKey,newValue);
        modifier.put("$set",upsertValue);
        cl.upsert(matcher,modifier,hint, null, flag);
        DBCursor result = cl.query(upsertValue,null,null,null,0);
        while ( result.hasNext() ){
            result.getNext();
            count++;
        }
        assertEquals(count,1);

        //case 2: upsert one on main and sub collection
        ConstantsInsert.insertSameRecords(subCL1, record, insertTimes);
        ConstantsInsert.insertSameRecords(subCL2, record, insertTimes);
        BaseException expectE = new BaseException(0);
        try {
            mainCL.upsert(matcher, modifier, hint, null, flag);
        }catch (BaseException e){
            expectE = e;
        }
        assertEquals(expectE.getErrorCode(),SDBError.SDB_COORD_UPDATE_MULTI_NODES.getErrorCode() );
    }

    @Test
    public void testDropIndex() {
        // create index
        BSONObject index = new BasicBSONObject();
        index.put("Id", 1);
        cl.createIndex(Constants.TEST_INDEX_NAME, index, false, false);
        //test
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
//  @Test
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
//      cl1.split(srcGroup, destGroup, condition, new BasicBSONObject());
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
                // applicable to engines lower than 5.0
                object.put("a", "testaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaatesttesttetestaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaatesttesttetestaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaatesttesttetestaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaatesttesttetestaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaatesttesttetestaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaatesttesttetestaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaatesttesttetestaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaatesttesttetestaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaatesttesttetestaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
                // applicable to engines of 5.0 and above
                //object.put("a", "testaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaatesttesttetestaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaatesttesttetestaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaatesttesttetestaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaatesttesttetestaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaatesttesttetestaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaatesttesttetestaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaatesttesttetestaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaatesttesttetestaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaatesttesttetestaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
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
                // ignore
            }
        }
    }

    @Test
    public void testInterrupt() {
        BSONObject obj = new BasicBSONObject();
        obj.put("date", new Date());
        cl.delete("");
        cl.insert(obj);
        DBCursor cursor = cl.query("", "", "", "", 0, 0);
        sdb.closeAllCursors();
        try {
            while (cursor.hasNext()) {
                System.out.println("record is: " + cursor.getNext());
            }
        } catch (BaseException e) {
            Assert.assertEquals(SDBError.SDB_DMS_CONTEXT_IS_CLOSE.getErrorCode(), e.getErrorCode());
        }
    }

    @Test
    public void testInterruptOperation() {
        BSONObject obj = new BasicBSONObject();
        obj.put("date", new Date());
        cl.delete("");
        sdb.interruptOperation();
        cl.insert(obj);
    }

    // create chinese record
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

    // create name list
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

    @Test
    @Ignore // jira_3357
    public void jira_3357_insertBigRecord() {
        final int recordSize = 15 * 1024 * 1024;
        byte[] binData = new byte[recordSize];

        for(int i = 0; i < recordSize; ++i) {
            binData[i] = 'a';
        }

        Binary binary = new Binary(binData);
        BSONObject object = new BasicBSONObject();
        object.put("bindata", binary);
        cl.insert(object);
    }

    @Test
	@Ignore
    public void jira_() {
        String csName = "testfoo_cs";
        String clName = "testbar_cs";
        DBCollection coll = sdb.getCollectionSpace(csName).getCollection(clName);
        DBCursor cursor = coll.query();
        while(cursor.hasNext()) {
            BSONObject object = cursor.getNext();
            System.out.println("record is: " + object);
        }
    }

    @Test
    @Ignore
    public void jira_bb() {
        Sequoiadb db =  new Sequoiadb("192.168.20.165", 11810, "", "");
        String coordIP = sdb.getReplicaGroup("SYSCatalogGroup").getSlave().getHostName();
        System.out.println("coordIP is: " + coordIP);
        ReplicaGroup rg = db.getReplicaGroup("group1");
        Node data = rg.createNode(coordIP, 21000, "/opt/sequoiadb/database/data/20000");
        rg.start();
        String hostName = data.getHostName();
        System.out.println("hostName is: " + hostName);
    }

    @Test
    @Ignore
    public void jira_cc() {
        DBCursor cursor = sdb.getList(Sequoiadb.SDB_LIST_COLLECTIONS, null, null, null);
        while(cursor.hasNext()) {
            BSONObject obj = cursor.getNext();
            System.out.println(obj);
        }
    }

    @Test
    @Ignore
    public void jira_3269() throws InterruptedException {
        String csName = "jira_3269";
        String clName = "jira_3269";
        try {
            ClientOptions clientOptions = new ClientOptions();
            clientOptions.setEnableCache(true);
            clientOptions.setCacheInterval(2000);
            Sequoiadb.initClient(clientOptions);
            Sequoiadb db1 = new Sequoiadb("192.168.20.165", 11810, "", "");
            Sequoiadb db2 = new Sequoiadb("192.168.20.165", 11810, "", "");
            db1.createCollectionSpace(csName).createCollection(clName);
            CollectionSpace cs2 = db2.getCollectionSpace(csName);
            DBCollection cl2 = cs2.getCollection(clName);
            Thread.sleep(2000);
            db1.getCollectionSpace(csName).dropCollection(clName);
            cs2.getCollection(clName);
        } finally {
            sdb.dropCollectionSpace(csName);
        }
    }

    @Test
    public void invalidateCacheTest() {
        BSONObject options = new BasicBSONObject();
        options.put("Role", "coord");
        sdb.invalidateCache(null);
        sdb.invalidateCache(options);
    }

    @Test
    public void testExactlyDate() {
        String utilStr = "utilDate";
        String sqlStr = "utilDate";
        BSONObject obj = new BasicBSONObject();
        Date date = new Date();
        java.sql.Date sqlDate = new java.sql.Date( date.getTime() );
        ClientOptions options = new ClientOptions();

        // case 1: exactlyDate is true
        options.setExactlyDate( true );
        Sequoiadb.initClient( options );
        obj.put( utilStr, date );
        obj.put( sqlStr, sqlDate );
        cl.truncate();
        cl.insertRecord( obj );

        BSONObject result1 = cl.queryOne();
        Date actualUtilDate1 = (Date) result1.get( utilStr );
        Date actualSqlDate1 = (Date) result1.get( sqlStr );
        Assert.assertEquals( DateInterceptUtil.getYMDTime( date ), (Long)actualUtilDate1.getTime() );
        Assert.assertEquals( DateInterceptUtil.getYMDTime( sqlDate ), (Long)actualSqlDate1.getTime() );

        // case 2: exactlyDate is false
        options.setExactlyDate( false );
        Sequoiadb.initClient( options );
        obj.put( utilStr, date );
        obj.put( sqlStr, sqlDate );
        cl.truncate();
        cl.insertRecord( obj );

        BSONObject result2 = cl.queryOne();
        Date actualUtilDate2 = (Date) result2.get( utilStr );
        Date actualSqlDate2 = (Date) result2.get( sqlStr );
        Assert.assertEquals( date.getTime(), actualUtilDate2.getTime() );
        Assert.assertEquals( sqlDate.getTime(), actualSqlDate2.getTime() );
    }

    @Test
    public void testBSONTimestamp() {
        BSONTimestamp ts1 = new BSONTimestamp(10000, 1000000);
        Assert.assertEquals(10001, ts1.getTime());
        Assert.assertEquals(0, ts1.getInc());

        BSONTimestamp ts2 = new BSONTimestamp(10000, -1);
        Assert.assertEquals(9999, ts2.getTime());
        Assert.assertEquals(999999, ts2.getInc());

        BSONTimestamp ts3 = new BSONTimestamp(10000, 0);
        Assert.assertEquals(10000, ts3.getTime());
        Assert.assertEquals(0, ts3.getInc());

        BSONTimestamp ts4 = new BSONTimestamp(10000, 999999);
        Assert.assertEquals(10000, ts4.getTime());
        Assert.assertEquals(999999, ts4.getInc());

        int time = 1534942305;
        int inc = 123456789;
        int incSec = 123456789 / 1000000;
        int incMSec = 123456789 % 1000000;
        int incMSec2 = 1000000 - incMSec;
        BSONTimestamp ts5 = new BSONTimestamp(time, inc);
        Assert.assertEquals(time + incSec, ts5.getTime());
        Assert.assertEquals(incMSec, ts5.getInc());

        BSONTimestamp ts6 = new BSONTimestamp(time, -inc);
        Assert.assertEquals(time - incSec - 1, ts6.getTime());
        Assert.assertEquals(incMSec2, ts6.getInc());

        BSONTimestamp ts7 = new BSONTimestamp(-time, inc);
        Assert.assertEquals(-time + incSec, ts7.getTime());
        Assert.assertEquals(incMSec, ts7.getInc());

        BSONTimestamp ts8 = new BSONTimestamp(-time, -inc);
        Assert.assertEquals(-time - incSec - 1, ts8.getTime());
        Assert.assertEquals(incMSec2, ts8.getInc());

        BasicBSONObject object = new BasicBSONObject();
        object.append("ts1", ts1).
                append("ts2", ts2).
                append("ts3", ts3).
                append("ts4", ts4).
                append("ts5", ts5).
                append("ts6", ts6).
                append("ts7", ts7).
                append("ts8", ts8);
        cl.insert(object);
        BSONObject result = cl.queryOne();
        System.out.println("result is: " + result);
    }
    
    @Test
    public void testCreateDropAutoIncrement() {
        if (!Constants.isCluster()) {
            return;
        }
        
        final String autoIncName1 = "ID1";
        final String autoIncName2 = "ID2";
        final String autoIncName3 = "ID3";

        cl.createAutoIncrement(new BasicBSONObject("Field", autoIncName1));

        List<BSONObject> options = new ArrayList<BSONObject>();
        BSONObject autoInc1 = new BasicBSONObject();
        autoInc1.put("Field", autoIncName2);
        autoInc1.put("StartValue", 100);
        options.add(autoInc1);

        BSONObject autoInc2 = new BasicBSONObject();
        autoInc2.put("Field", autoIncName3);
        autoInc2.put("StartValue", 200);
        options.add(autoInc2);

        cl.createAutoIncrement(options);

        String clFullName = Constants.TEST_CS_NAME_1 + "." + Constants.TEST_CL_NAME_1;
        BSONObject matcher = new BasicBSONObject("Name", clFullName);
        DBCursor cursor = sdb.getSnapshot(Sequoiadb.SDB_SNAP_CATALOG, matcher, null, null);
        while (cursor.hasNext()) {
            BSONObject obj = cursor.getNext();
            BasicBSONList autoIncList = (BasicBSONList)obj.get("AutoIncrement");
            assertEquals(3, autoIncList.size());
            // System.out.println(autoIncList);
        }
        cursor.close();

        cl.dropAutoIncrement(autoIncName1);
        List<String> names = new ArrayList<String>();
        names.add(autoIncName2);
        names.add(autoIncName3);
        cl.dropAutoIncrement(names);

        cursor = sdb.getSnapshot(Sequoiadb.SDB_SNAP_CATALOG, matcher, null, null);
        while (cursor.hasNext()) {
            BSONObject obj = cursor.getNext();
            BasicBSONList autoIncList = (BasicBSONList)obj.get("AutoIncrement");
            assertEquals(0, autoIncList.size());
        }
        cursor.close();
    }

    @Test
    public void testExplain() {
        BSONObject options1 = new BasicBSONObject();
        options1.put("Expand", true);
        BSONObject options2 = new BasicBSONObject();
//        options2.put("Expand", false);
        DBCursor cursor1 = cl.explain(null, null, null, null, 0, -1, 0, options1);
        DBCursor cursor2 = cl.explain(null, null, null, null, 0, -1, 0, options2);
        while (cursor1.hasNext()) {
            BSONObject obj = cursor1.getNext();
            System.out.println("obj1 is: " + obj.toString());
        }
        while (cursor2.hasNext()) {
            BSONObject obj = cursor2.getNext();
            System.out.println("obj2 is: " + obj.toString());
        }

    }

}
