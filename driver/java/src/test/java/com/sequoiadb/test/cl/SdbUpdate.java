package com.sequoiadb.test.cl;

import com.sequoiadb.base.*;
import com.sequoiadb.base.options.UpdateOption;
import com.sequoiadb.base.options.UpsertOption;
import com.sequoiadb.base.result.InsertResult;
import com.sequoiadb.base.result.UpdateResult;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.message.request.UpdateRequest;
import com.sequoiadb.test.common.Constants;
import com.sequoiadb.test.common.ConstantsInsert;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.junit.*;

import java.util.ArrayList;
import java.util.List;

import static org.junit.Assert.assertEquals;


public class SdbUpdate {
    private static Sequoiadb sdb;
    private static CollectionSpace cs;
    private static DBCollection cl;
    private static DBCursor cursor;

    @BeforeClass
    public static void beforeClass() throws Exception {
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
        cl.bulkInsert(list, DBCollection.FLG_INSERT_CONTONDUP);
    }

    @After
    public void tearDown() throws Exception {
        cl.truncate();
    }

    @Test
    public void testUpdateInc() {
        BSONObject modifier = new BasicBSONObject();
        BSONObject m = new BasicBSONObject();
        BSONObject condition = new BasicBSONObject();
        //$inc modifier:{$inc:{Id:10}}
        modifier.put("$inc", m);
        m.put("Id", 10);
        cl.update(null, modifier, null);
        condition.put("Id", new BasicBSONObject("$gte", 10));
        cursor = cl.query(condition, null, null, null);
        int inc_num = 0;
        while (cursor.hasNext()) {
            cursor.getNext();
            inc_num++;
        }
        assertEquals(100, inc_num);
    }

    @Test
    public void testUpdateSet() {
        //$set  match:{Id:{$gte:0,$lte:9}} modifier:{$set:{age:12}} hint:null
        BSONObject modifier = new BasicBSONObject();
        BSONObject m = new BasicBSONObject();

        BSONObject match = new BasicBSONObject();
        BSONObject con = new BasicBSONObject();
        modifier.put("$set", m);
        m.put("Id", 12);

        con.put("$gte", 0);
        con.put("$lte", 99);
        match.put("age", con);
        cl.update(match, modifier, null);
        cursor = cl.query(m, null, null, null);
        int set_num = 0;
        while (cursor.hasNext()) {
            cursor.getNext();
            set_num++;
        }
        assertEquals(100, set_num);
    }

    @Test
    public void testUpdateUnset() {
        //$unset match:{Id:{$gte:0,$lte:9}} modifier:{$unset:{age:""}}
        BSONObject modifier = new BasicBSONObject();
        BSONObject m = new BasicBSONObject();

        BSONObject match = new BasicBSONObject();
        BSONObject con = new BasicBSONObject();

        modifier.put("$unset", m);
        m.put("Id", "");

        con.put("$gte", 0);
        con.put("$lte", 99);
        match.put("age", con);

        cl.update(match, modifier, null);

        BSONObject query = new BasicBSONObject();
        query.put("Id", "");
        cursor = cl.query(query, null, null, null);

        int unset_num = 0;
        while (cursor.hasNext()) {
            cursor.getNext();
            unset_num++;
        }
        assertEquals(0, unset_num);
    }

    @Test
    public void testUpdateAddtoset() {
        BSONObject modifier = new BasicBSONObject();
        BSONObject m = new BasicBSONObject();

        BSONObject match = new BasicBSONObject();
        BSONObject con = new BasicBSONObject();
        //$addtoset match:{Id:{$gte:0,$lte:9}}  modifier:{$addtoset:{arr:[10,20,30]}}
        int[] arr = {10, 20, 30};
        modifier.put("$addtoset", m);
        m.put("arr", arr);

        con.put("$gte", 0);
        con.put("$lte", 9);
        match.put("Id", con);

        cl.update(match, modifier, null);

        BSONObject query = new BasicBSONObject();
        BSONObject q = new BasicBSONObject();

        query.put("arr", q);
        q.put("$exists", 1);

        cursor = cl.query(query, null, null, null);
        int addtoset_num = 0;
        while (cursor.hasNext()) {
            cursor.getNext();
            addtoset_num++;
        }
        assertEquals(10, addtoset_num);
    }

    @Test
    public void testUpdatePop() {
        //$pop match:{Id:0} modifier:{$pop:{arr:1}}
        BSONObject modifier = new BasicBSONObject();
        BSONObject m = new BasicBSONObject();

        BSONObject match = new BasicBSONObject();
        BSONObject con = new BasicBSONObject();

        modifier.put("$pop", m);
        m.put("arr", 1);

        match.put("Id", 0);

        cl.update(match, modifier, null);

        BSONObject query = new BasicBSONObject();
        int[] arr = {10, 20};
        query.put("Id", 0);
        query.put("arr", arr);

        cursor = cl.query(match, null, null, null);
        int pop_num = 0;
        while (cursor.hasNext()) {
            cursor.getNext();
            pop_num++;
        }
        assertEquals(1, pop_num);

    }

    @Test
    public void testUpdatePull() {
        // insert array before execute pull
        BSONObject modifier1 = new BasicBSONObject();
        BSONObject m1 = new BasicBSONObject();
        modifier1.put("$push_all", m1);
        int[] arr = {40, 50, 20, 60};
        m1.put("arr", arr);
        BSONObject match1 = new BasicBSONObject();
        match1.put("Id", 1);
        cl.update(match1, modifier1, null);
        // TODO:
        //$pull match:{Id:0}  modifier:{$pull:{arr:10}}
        BSONObject modifier = new BasicBSONObject();
        BSONObject m = new BasicBSONObject();

        BSONObject match = new BasicBSONObject();

        modifier.put("$pull", m);
        m.put("arr", 20);

        match.put("Id", 1);

        cl.update(match, modifier, null);
        // check
        int[] arr1 = {40, 50, 60};
        BSONObject matcher = new BasicBSONObject();
        matcher.put("arr", arr1);
        cursor = cl.query(matcher, null, null, null);
        int count = 0;
        while (cursor.hasNext()) {
            cursor.getNext();
            count++;
        }
//	    BSONObject obj = cursor.getNext();
//	    System.out.println("obj is: "+obj.toString());
        assertEquals(count, 1);
        //assertEquals(Integer.parseInt(obj.get("arr").toString().substring(1, 3)),20);
    }

    @Test
    public void testUpdatePull_all() {
        // insert array before execute pull
        BSONObject modifier1 = new BasicBSONObject();
        BSONObject m1 = new BasicBSONObject();
        modifier1.put("$push_all", m1);
        int[] arr1 = {40, 50, 20, 60, 70, 80};
        m1.put("arr", arr1);
        BSONObject match1 = new BasicBSONObject();
        match1.put("Id", 1);
        cl.update(match1, modifier1, null);
        // TODO:
        //$pull_all match:{Id:1} modifier:{$pull_all:{arr:[10,20,40]}}
        BSONObject modifier = new BasicBSONObject();
        BSONObject m = new BasicBSONObject();

        int[] arr = {20, 40};
        modifier.put("$pull_all", m);
        m.put("arr", arr);

        BSONObject match = new BasicBSONObject();

        match.put("Id", 1);

        cl.update(match, modifier, null);

        cursor = cl.query(match, null, null, null);
        // check
        int[] arr2 = {50, 60, 70, 80};
        BSONObject matcher = new BasicBSONObject();
        matcher.put("arr", arr2);
        cursor = cl.query(matcher, null, null, null);
        int count = 0;
        while (cursor.hasNext()) {
            cursor.getNext();
            count++;
        }
        assertEquals(count, 1);
    }

    @Test
    public void testUpdatePush() {
        //$push match:{Id:1} modifier:{$push:{arr:30,arr2:20}}
        BSONObject modifier = new BasicBSONObject();
        BSONObject m = new BasicBSONObject();

        modifier.put("$push", m);
        m.put("arr", 30);
        m.put("arr2", 20);

        BSONObject match = new BasicBSONObject();

        match.put("Id", 1);

        cl.update(match, modifier, null);

        BSONObject query = new BasicBSONObject();
        int[] arr = {30};
        int[] arr2 = {20};

        query.put("Id", 1);
        query.put("arr", arr);
        query.put("arr2", arr2);

        cursor = cl.query(query, null, null, null);
        int push_num = 0;
        while (cursor.hasNext()) {
            cursor.getNext();
            push_num++;
        }
        assertEquals(1, push_num);
    }

    @Test
    public void testUpdatePush_all() {
        //$push match:{Id:1} modifier:{$push_all:{arr:[40,50,20],arr2:[20,50,30]}}
        BSONObject modifier = new BasicBSONObject();
        BSONObject m = new BasicBSONObject();
        int[] arr1 = {40, 50, 20};
        int[] arr2 = {20, 50, 30};
        modifier.put("$push_all", m);
        m.put("arr", arr1);
        m.put("arr2", arr2);

        BSONObject match = new BasicBSONObject();

        match.put("Id", 1);

        cl.update(match, modifier, null);

        BSONObject query = new BasicBSONObject();
//		int[] arr1_1 = {30,30,40,50,20};
//		int[] arr2_2 = {20,20,50,30};

        query.put("Id", 1);
        query.put("arr", arr1);
        query.put("arr2", arr2);

        cursor = cl.query(query, null, null, null);
        int push_all_num = 0;
        while (cursor.hasNext()) {
            cursor.getNext();
            push_all_num++;
        }
        assertEquals(1, push_all_num);
    }

    @Test
    public void testSetMatcherAndSetModifier() {
        DBQuery query = new DBQuery();
        BSONObject matcher = new BasicBSONObject();
        BSONObject modifier = new BasicBSONObject();
        BSONObject m = new BasicBSONObject();
        matcher.put("Id", 2);
        m.put("Age", 20);
        modifier.put("$set", m);
        query.setMatcher(matcher);
        query.setModifier(modifier);
        cl.update(query);
        cursor = cl.query(matcher, null, null, null);
        assertEquals(cursor.getNext().get("Age"), 20);
    }

    @Test
    public void testUpsert() {
        BSONObject matcher = new BasicBSONObject();
        BSONObject modifier = new BasicBSONObject();
        BSONObject m = new BasicBSONObject();
        BSONObject hint = new BasicBSONObject();

        matcher.put("Id", 20);
        m.put("Age", 80);
        modifier.put("$set", m);

        cl.upsert(matcher, modifier, hint);
        cursor = cl.query(matcher, null, null, null);
        BSONObject obj = cursor.getNext();
        assertEquals(obj.get("Age"), 80);
    }

    @Test
    public void testUpdateWithResult(){
        BSONObject matcher = new BasicBSONObject();
        BSONObject modifier = new BasicBSONObject();
        BSONObject m = new BasicBSONObject();

        m.put("b", 1);
        modifier.put("$set", m);

        cl.truncate();
        List<BSONObject> docList = new ArrayList<>();
        docList.add( new BasicBSONObject("a", 1) );
        docList.add( new BasicBSONObject("a", 2) );
        docList.add( new BasicBSONObject("a", 3) );
        cl.bulkInsert( docList );

        // case 1: empty bson or null
        try {
            cl.updateRecords( null, null );
        }catch ( BaseException e ){
            Assert.assertEquals( SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode() );
        }
        try {
            cl.updateRecords( new BasicBSONObject(), new BasicBSONObject() );
        }catch ( BaseException e ){
            Assert.assertEquals( SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode() );
        }
        try {
            cl.upsertRecords( null, null );
        }catch ( BaseException e ){
            Assert.assertEquals( SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode() );
        }
        try {
            cl.upsertRecords( new BasicBSONObject(), new BasicBSONObject() );
        }catch ( BaseException e ){
            Assert.assertEquals( SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode() );
        }
        // empty result
        UpdateResult r1 = new UpdateResult( null );
        Assert.assertEquals( -1, r1.getInsertNum() );
        Assert.assertEquals( -1, r1.getModifiedNum() );
        Assert.assertEquals( -1, r1.getUpdatedNum() );

        // case 2: normal update
        UpdateResult r2 = cl.updateRecords( matcher, modifier );
        Assert.assertEquals( 0, r2.getInsertNum() );
        Assert.assertEquals( 3, r2.getModifiedNum() );
        Assert.assertEquals( 3, r2.getUpdatedNum() );

        // case 3: update one
        UpdateOption option = new UpdateOption();
        option.setFlag( UpdateOption.FLG_UPDATE_ONE );
        UpdateResult r3 = cl.updateRecords( matcher, modifier, option );
        Assert.assertEquals( 1, r3.getUpdatedNum() );

        // case 4: normal upsert
        UpsertOption upsertOption = new UpsertOption();
        upsertOption.setFlag( UpdateOption.FLG_UPDATE_ONE );
        UpdateResult r4 = cl.upsertRecords( matcher, modifier, upsertOption );
        Assert.assertEquals( 0, r4.getInsertNum() );
        Assert.assertEquals( 0, r4.getModifiedNum() );
        Assert.assertEquals( 1, r4.getUpdatedNum() );

        // case 5: insert
        matcher.put( "a", 5 );
        UpdateResult r5 = cl.upsertRecords( matcher, modifier, upsertOption );
        Assert.assertEquals( 1, r5.getInsertNum() );
        Assert.assertEquals( 0, r5.getModifiedNum() );
        Assert.assertEquals( 0, r5.getUpdatedNum() );
    }
}
