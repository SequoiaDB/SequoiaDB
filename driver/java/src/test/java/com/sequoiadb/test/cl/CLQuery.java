package com.sequoiadb.test.cl;

import com.sequoiadb.base.*;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.test.common.Constants;
import com.sequoiadb.test.common.ConstantsInsert;
import com.sequoiadb.testdata.SDBTestHelper;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.junit.*;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

public class CLQuery {
    private static Sequoiadb sdb;
    private static CollectionSpace cs;
    private static DBCollection cl;

    @BeforeClass
    public static void beforeClass() throws Exception {
        // sdb
        sdb = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");
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

    @AfterClass
    public static void afterClass() throws Exception {
        sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
        sdb.disconnect();
    }

    @Before
    public void setUp() throws Exception {
        List<BSONObject> list = ConstantsInsert.createRecordList(100);
        cl.bulkInsert(list, 0);
    }

    @After
    public void tearDown() throws Exception {
        cl.truncate();
    }

    @Test
    public void testQuery() {
        DBCursor cursor = cl.query();
        int i = 0;
        while (cursor.hasNext()) {
            cursor.getNext();
            i++;
        }
        assertEquals(100, i);
    }

    // relational operator test
    @Test
    public void testConditionQuery() {
        // $gt
        BSONObject query = new BasicBSONObject();
        BSONObject con_gt = new BasicBSONObject();
        con_gt.put("$gt", 90);
        query.put("Id", con_gt);

        DBCursor cursor = cl.query(query, null, null, null, 0, -1);
        int gt = 0;
        while (cursor.hasNext()) {
            cursor.getNext();
            gt++;
        }
        assertEquals(9, gt);

        // $gte
        query.removeField("Id");
        BSONObject con_gte = new BasicBSONObject();
        query.put("Id", con_gte);
        con_gte.put("$gte", 90);
        cursor = cl.query(query, null, null, null);
        int gte = 0;
        while (cursor.hasNext()) {
            cursor.getNext();
            gte++;
        }
        assertEquals(10, gte);

        // $lt
        query.removeField("Id");
        BSONObject con_lt = new BasicBSONObject();
        query.put("Id", con_lt);
        con_lt.put("$lt", 90);
        cursor = cl.query(query, null, null, null);
        int lt = 0;
        while (cursor.hasNext()) {
            cursor.getNext();
            lt++;
        }
        assertEquals(90, lt);

        // lte
        query.removeField("Id");
        BSONObject con_lte = new BasicBSONObject();
        query.put("Id", con_lte);
        con_lte.put("$lte", 90);
        cursor = cl.query(query, null, null, null);
        int lte = 0;
        while (cursor.hasNext()) {
            cursor.getNext();
            lte++;
        }
        assertEquals(91, lte);

        // $ne
        query.removeField("Id");
        BSONObject con_ne = new BasicBSONObject();
        query.put("Id", con_ne);
        con_ne.put("$ne", 90);
        cursor = cl.query(query, null, null, null);
        int ne = 0;
        while (cursor.hasNext()) {
            cursor.getNext();
            ne++;
        }
        assertEquals(99, ne);

        // et
        query.removeField("Id");
        BSONObject con_et = new BasicBSONObject();
        query.put("Id", con_et);
        con_et.put("$et", 90);
        cursor = cl.query(query, null, null, null);
        int et = 0;
        while (cursor.hasNext()) {
            cursor.getNext();
            et++;
        }
        assertEquals(1, et);

        // equal
        query.removeField("Id");
        query.put("Id", 90);
        cursor = cl.query(query, null, null, null);
        int equal = 0;
        while (cursor.hasNext()) {
            cursor.getNext();
            equal++;
        }
        assertEquals(1, equal);

        // $gt and $lt
        query.removeField("Id");
        BSONObject con_gt_lt = new BasicBSONObject();

        query.put("Id", con_gt_lt);

        con_gt_lt.put("$gt", 0);
        con_gt_lt.put("$lt", 10);
        con_gt_lt.put("$exists", 1);

        cursor = cl.query(query, null, null, null);
        int gt_lt = 0;
        while (cursor.hasNext()) {
            cursor.getNext();
            gt_lt++;
        }
        assertEquals(9, gt_lt);

        // $gte and $lt
        query.removeField("Id");
        BSONObject con_gte_lt = new BasicBSONObject();

        query.put("Id", con_gte_lt);

        con_gte_lt.put("$gte", 0);
        con_gte_lt.put("$lt", 10);

        cursor = cl.query(query, null, null, null);
        int gte_lt = 0;
        while (cursor.hasNext()) {
            cursor.getNext();
            gte_lt++;
        }
        assertEquals(10, gte_lt);

        // $gte and $lte
        query.removeField("Id");
        BSONObject con_gte_lte = new BasicBSONObject();

        query.put("Id", con_gte_lte);

        con_gte_lte.put("$gte", 0);
        con_gte_lte.put("$lte", 10);

        cursor = cl.query(query, null, null, null);
        int gte_lte = 0;
        while (cursor.hasNext()) {
            cursor.getNext();
            gte_lte++;
        }
        assertEquals(11, gte_lte);

        // $exists
        query.removeField("Id");
        BSONObject con_exists = new BasicBSONObject();

        query.put("Id", con_exists);

        con_exists.put("$exists", 0);

        cursor = cl.query(query, null, null, null);
        int j = 0;
        while (cursor.hasNext()) {
            cursor.getNext();
            j++;
        }
        assertEquals(0, j);

        // $exists
        query.removeField("Id");
        BSONObject con_exist = new BasicBSONObject();

        query.put("Id", con_exist);

        con_exist.put("$exists", 1);

        cursor = cl.query(query, null, null, null);
        int k = 0;
        while (cursor.hasNext()) {
            cursor.getNext();
            k++;
        }
        assertEquals(100, k);

        // $elemMatch
        query.removeField("Id");
        BSONObject con_elemMatch = new BasicBSONObject();
        BSONObject con_elemMatch2 = new BasicBSONObject();

        query.put("phone", con_elemMatch);

        con_elemMatch.put("$elemMatch", con_elemMatch2);
        con_elemMatch2.put("0", 99);
        con_elemMatch2.put("1", 100);

        cursor = cl.query(query, null, null, null);
        int elem = 0;
        while (cursor.hasNext()) {
            cursor.getNext();
            elem++;
        }
        assertEquals(1, elem);

        // logical operators test
        // $in
        int[] arr_in = {10, 20, 30, 100};
        query.removeField("phone");
        BSONObject con_in = new BasicBSONObject();
        query.put("Id", con_in);
        con_in.put("$in", arr_in);
        cursor = cl.query(query, null, null, null);
        int in = 0;
        while (cursor.hasNext()) {
            cursor.getNext();
            in++;
        }
        assertEquals(3, in);

        // $nin
        int[] arr_nin = {10, 20, 30, 100};
        query.removeField("phone");
        BSONObject con_nin = new BasicBSONObject();
        query.put("Id", con_nin);
        con_nin.put("$nin", arr_nin);
        cursor = cl.query(query, null, null, null);
        int nin = 0;
        while (cursor.hasNext()) {
            cursor.getNext();
            nin++;
        }
        assertEquals(97, nin);

        // $and
        query.removeField("Id");
        List<BSONObject> list_and = new ArrayList<BSONObject>();
        BSONObject con_and_id = new BasicBSONObject();
        BSONObject con_and_query = new BasicBSONObject();
        BSONObject con_id = new BasicBSONObject();
        BSONObject con_phone = new BasicBSONObject();
        // {$and:[{Id:{$gte:0,$lte:20}},{"phone.0":{$gte:10,$lte:30}}]}
        con_and_id.put("Id", con_id);
        con_id.put("$gte", 0);
        con_id.put("$lte", 20);
        list_and.add(con_and_id);

        con_and_query.put("phone.0", con_phone);
        con_phone.put("$gte", 10);
        con_phone.put("$lte", 30);
        list_and.add(con_and_query);

        query.put("$and", list_and);
        // System.out.println(query);
        cursor = cl.query(query, null, null, null);
        int and = 0;
        while (cursor.hasNext()) {
            cursor.getNext();
            and++;
        }
        assertEquals(11, and);

        // not
        query.removeField("$and");
        List<BSONObject> list_not = new ArrayList<BSONObject>();
        BSONObject con_id_not = new BasicBSONObject();
        BSONObject con_query_not = new BasicBSONObject();
        BSONObject con_query_not_1 = new BasicBSONObject();
        BSONObject con_not_id = new BasicBSONObject();
        BSONObject con_not_phone = new BasicBSONObject();
        BSONObject con_not_phone_1 = new BasicBSONObject();
        // {$not:[{Id:{$gte:1,$lte:30}},{"phone.0":{$gte:10,$lte:30}},{"phone.1"
        // : { "$gte" : 25 , "$lte" : 45}]}
        con_not_id.put("$gte", 1);
        con_not_id.put("$lte", 30);

        con_not_phone.put("$gte", 10);
        con_not_phone.put("$lte", 30);

        con_not_phone_1.put("$gte", 25);
        con_not_phone_1.put("$lte", 45);

        con_id_not.put("Id", con_not_id);
        list_not.add(con_id_not);
        con_query_not.put("phone.0", con_not_phone);
        list_not.add(con_query_not);
        con_query_not_1.put("phone.1", con_not_phone_1);
        list_not.add(con_query_not_1);

        query.put("$not", list_not);
        // System.out.println("not="+query);
        cursor = cl.query(query, null, null, null);
        int not = 0;
        while (cursor.hasNext()) {
            cursor.getNext();
            not++;
        }
        assertEquals(93, not);

        // or
        query.removeField("$not");
        List<BSONObject> list_or = new ArrayList<BSONObject>();
        BSONObject con_id_or = new BasicBSONObject();
        BSONObject con_query_or = new BasicBSONObject();
        BSONObject con_or_id = new BasicBSONObject();
        BSONObject con_or_phone = new BasicBSONObject();
        // {$or:[{Id:{$gte:0,$lte:20}},{"phone.0":{$gte:10,$lte:30}}]}
        con_or_id.put("$gte", 0);
        con_or_id.put("$lte", 20);

        con_or_phone.put("$gte", 10);
        con_or_phone.put("$lte", 30);

        con_id_or.put("Id", con_or_id);
        list_or.add(con_id_or);
        con_query_or.put("phone.0", con_or_phone);
        list_or.add(con_query_or);

        query.put("$or", list_or);
        // System.out.println("or="+query);

        cursor = cl.query(query, null, null, null);
        int or = 0;
        while (cursor.hasNext()) {
            cursor.getNext();
            or++;
        }
        assertEquals(31, or);

        // $type
        query.removeField("$or");
        BSONObject con_type = new BasicBSONObject();
        // {Id:{$type:16}}
        con_type.put("$type", 1);
        con_type.put("$et", 16);

        query.put("Id", con_type);

        cursor = cl.query(query, null, null, null);
        int type = 0;
        while (cursor.hasNext()) {
            cursor.getNext();
            type++;
        }
        assertEquals(100, type);
    }

    @Test
    public void testQueryWithFlag1() {
        BSONObject empty = new BasicBSONObject();
        BSONObject condition = new BasicBSONObject();
        condition.put("Id", 1);
        BSONObject selector = new BasicBSONObject();
        selector.put("Id", "");
        DBCursor cursor = cl.query(condition, selector, empty, empty, 0, -1, 1);
        String expect = "{ \"\" : \"1\" }";
        int i = 0;
        BSONObject obj = null;
        while (cursor.hasNext()) {
            byte[] bytes = cursor.getNextRaw();
            obj = SDBTestHelper.byteArrayToBSONObject(bytes);
            System.out.println(obj.toString());
            System.out.println(expect);
            assertTrue(obj.toString().compareTo(expect) == 0);
            i++;
        }
        assertEquals(1, i);
    }

    @Test
    public void testQueryWithFlag128() {
        BSONObject empty = new BasicBSONObject();
        BSONObject hint = new BasicBSONObject();
        BSONObject key = new BasicBSONObject();
        key.put("Id", 1);
        key.put("str", -1);
        String indexName = "index128";
        cl.createIndex(indexName, key, false, false);
        hint.put("", indexName);
        DBCursor cur1 = sdb.getSnapshot(3, empty, empty, empty);
        BSONObject obj = cur1.getCurrent();
        long count1 = (Long) obj.get("TotalIndexRead");

        DBCursor cursor = cl.query(empty, empty, empty, hint, 0, -1,
            DBQuery.FLG_QUERY_FORCE_HINT);
        int i = 0;
        while (cursor.hasNext()) {
            BSONObject obj1 = cursor.getNext();
            System.out.println(obj1.toString());
            i++;
        }
        assertEquals(100, i);

        DBCursor cur2 = sdb.getSnapshot(3, empty, empty, empty);
        long count2 = 0;
        while (cur2.hasNext()) {
            obj = cur2.getNext();
            count2 += (Long) obj.get("TotalIndexRead");
        }
        System.out.println("count1 is " + count1 + ", count2 is " + count2);
        assertEquals(100, count2 - count1);
    }

    @Test
    public void testQueryWithFlag128AndHintNotExist() {
        BSONObject empty = new BasicBSONObject();
        BSONObject hint = new BasicBSONObject();
        String indexName = "index128";
        hint.put("", indexName);
        DBCursor cur1 = sdb.getSnapshot(3, empty, empty, empty);
        try {
            DBCursor cursor = cl.query(empty, empty, empty, hint, 0, -1,
                DBQuery.FLG_QUERY_FORCE_HINT);
        } catch (BaseException e) {
            assertTrue(e.getErrorType().equals("SDB_RTN_INVALID_HINT"));
        }
    }

    @Test
    public void testQueryWithFlag256() {
        BSONObject empty = new BasicBSONObject();
        DBCursor cursor = cl.query(empty, empty, empty, empty, 0, -1,
            DBQuery.FLG_QUERY_PARALLED);
        int i = 0;
        BSONObject obj1 = null;
        while (cursor.hasNext()) {
            obj1 = cursor.getNext();
            System.out.println(obj1.toString());
            i++;
        }
        assertEquals(100, i);
    }

    @Test
    public void testQueryWithMultiFlags() {
        BSONObject empty = new BasicBSONObject();
        DBCursor cursor = cl.query(empty, empty, empty, empty, 0, -1,
                DBQuery.FLG_QUERY_STRINGOUT|DBQuery.FLG_QUERY_FORCE_HINT|DBQuery.FLG_QUERY_PARALLED);
        int i = 0;
        BSONObject obj1 = null;
        while (cursor.hasNext()) {
            obj1 = cursor.getNext();
            System.out.println(obj1.toString());
            i++;
        }
        assertEquals(100, i);
    }

    @Test
    public void testQueryRecordHasNoKey() {
        BSONObject record = new BasicBSONObject();
        record.put("", "name|age|Id");
        cl.insert(record);
        BSONObject empty = new BasicBSONObject();
        DBCursor cursor = cl.query(empty, empty, empty, empty, 0, -1, 0);
        int i = 0;
        BSONObject obj1 = null;
        while (cursor.hasNext()) {
            obj1 = cursor.getNext();
            System.out.println(obj1.toString());
            i++;
        }
        assertEquals(101, i);
    }

    @Test
    public void testQueryOne() {
        BSONObject empty = new BasicBSONObject();
        BSONObject matcher = new BasicBSONObject("Id", 0);
        BSONObject selector = new BasicBSONObject();
        selector.put("age", "");
        selector.put("phone", "");
        selector.put("str", "");
        BSONObject orderBy = new BasicBSONObject();
        orderBy.put("Id", 1);
        orderBy.put("age", -1);
        BSONObject hint = new BasicBSONObject();
        hint.put("", "_id");
        BSONObject record = cl.queryOne(matcher, selector, orderBy, hint, 0);
        System.out.println("queryOne(...) return record is: "
            + record.toString());
    }

    @Test
    public void testQueryOneWithNoArg() {
        Random random = new Random();
        int r = random.nextInt(2);
        if (r % 2 == 0) {
            BSONObject matcher = new BasicBSONObject();
            cl.delete(matcher);
            BSONObject record = cl.queryOne();
            assertTrue(record == null);
            // System.out.println("queryOne() no record return");
        } else {
            BSONObject record = cl.queryOne();
            assertTrue(record != null);
            // System.out.println("queryOne() return record is: " +
            // record.toString());
        }
    }

    @Test
    public void testQueryWithFlags() {
        BSONObject dummy = new BasicBSONObject();
        BSONObject hint = new BasicBSONObject("", "$id");
        DBCursor cursor = null;
        // case 1
        cursor = cl.query(dummy, dummy, dummy, dummy,
            DBQuery.FLG_QUERY_STRINGOUT);
        // case 2
        cursor = cl.query(dummy, dummy, dummy, hint,
            DBQuery.FLG_QUERY_STRINGOUT | DBQuery.FLG_QUERY_FORCE_HINT);
        // case 3
        cursor = cl.query(dummy, dummy, dummy, hint,
            DBQuery.FLG_QUERY_STRINGOUT | DBQuery.FLG_QUERY_FORCE_HINT |
                DBQuery.FLG_QUERY_PARALLED | DBQuery.FLG_QUERY_WITH_RETURNDATA);
    }
}
