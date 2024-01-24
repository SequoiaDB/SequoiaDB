package com.sequoiadb.test.cl;

import com.sequoiadb.base.*;
import com.sequoiadb.base.options.InsertOption;
import com.sequoiadb.base.result.InsertResult;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.test.common.Constants;
import com.sequoiadb.testdata.SDBTestHelper;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;
import org.junit.*;

import java.util.ArrayList;
import java.util.List;

import static com.sequoiadb.base.DBCollection.FLG_INSERT_CONTONDUP;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;


public class CLInsert {
    private static Sequoiadb sdb;
    private static CollectionSpace cs;
    private static DBCollection cl;
    private static DBCollection cl2;
    private static DBCursor cursor;
    private static long i = 0;

    @BeforeClass
    public static void setConnBeforeClass() throws Exception {
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
        conf.put( "AutoIncrement", new BasicBSONObject("Field", "autoIncField"));
        cl2 = cs.createCollection( Constants.TEST_CS_NAME_2, conf );
    }

    @AfterClass
    public static void DropConnAfterClass() throws Exception {
        sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
        sdb.disconnect();
    }

    @Before
    public void setUp() throws Exception {
        cl.truncate();
        cl2.truncate();
    }

    @After
    public void tearDown() throws Exception {

    }

    @Test
    public void insertOneRecord() {
        System.out.println("begin to test insertOneRecord ...");

        // create record
        BSONObject obj = new BasicBSONObject();
        BSONObject obj1 = new BasicBSONObject();
        ObjectId id = new ObjectId();
        ;
        obj.put("_id", id);
        obj.put("Id", 10);
        obj.put("姓名", "汤姆");
        obj.put("年龄", 30);
        obj.put("Age", 30);

        obj1.put("0", "123456");
        obj1.put("1", "654321");

        obj.put("电话", obj1);
        obj.put("boolean1", true);
        obj.put("boolean2", false);
        obj.put("nullobj", null);
        obj.put("intnum", 999999999);
//		obj.put("floatnum",9999999999.9999999999);

        // record current session totalInsert num
        BSONObject temp = new BasicBSONObject();
        DBCursor cur = sdb.getSnapshot(3, temp, temp, temp);
        long time1 = SDBTestHelper.getTotalBySnapShotKey(cur, "TotalInsert");
        System.out.println("before insert, current session total insert num is " + time1);
        long count = 0;
        count = cl.getCount();
        System.out.println("before insert, the count is: " + count);
        // insert
        cl.insert(obj);
        long count1 = 0;
        count1 = cl.getCount();
        System.out.println("after insert, the count is: " + count1);
        BSONObject empty = new BasicBSONObject();
        cur = sdb.getSnapshot(3, empty, empty, empty);
        long time2 = SDBTestHelper.getTotalBySnapShotKey(cur, "TotalInsert");
        System.out.println("after insert, current session total insert num is " + time2);
        assertEquals(1, time2 - time1);

        //query without condition
        BSONObject tmp = new BasicBSONObject();
        DBCursor tmpCursor = cl.query(tmp, null, null, null);
        while (tmpCursor.hasNext()) {
            BSONObject temp1 = tmpCursor.getNext();
            System.out.println(temp1.toString());
        }

        // query
        BSONObject query = new BasicBSONObject();
        query.put("Id", 10);
        query.put("姓名", "汤姆");
        query.put("年龄", 30);
        query.put("Age", 30);
        query.put("电话.0", "123456");
        query.put("电话.1", "654321");
        query.put("boolean1", true);
        query.put("boolean2", false);
        query.put("nullobj", null);
        query.put("intnum", 999999999);
//		query.put("floatnum",9999999999.9999999999);
        cursor = cl.query(query, null, null, null);
        while (cursor.hasNext()) {
            BSONObject temp1 = cursor.getNext();
            System.out.println(temp1.toString());
            i++;
        }
        long count2 = cl.getCount();
        System.out.println("after cl.query(), i is: " + i);
        System.out.println("after cl.query(), the count is: " + count2);
        // check
        assertEquals(1, i);
    }

    @Test
    public void insertNumberLong() {
        System.out.println("begin to test insertNumberLong ...");

        String json = "{a:{$numberLong:\"10000000\"}}";
        String result = "10000000";

        cl.insert(json);
        BSONObject qobj = new BasicBSONObject();
        DBCursor cursor = cl.query(qobj, null, null, null);
        while (cursor.hasNext()) {
            BSONObject record = cursor.getNext();
            // check
            assertTrue(record.toString().indexOf(result) >= 0);
        }
    }

    @Test
    public void insertWithFlag() {
        BSONObject obj1 = new BasicBSONObject().append("_id", 1).append("a",1);
        BSONObject obj2 = new BasicBSONObject().append("_id", 1).append("a",1);
        cl.insert(obj1, FLG_INSERT_CONTONDUP);
        cl.insert(obj2, FLG_INSERT_CONTONDUP);
        try {
            cl.insert(obj2, 0);
            Assert.fail();
        } catch (BaseException e) {
            Assert.assertEquals(SDBError.SDB_IXM_DUP_KEY.getErrorCode(), e.getErrorCode());
        }
    }

    @Test
    public void insertCompatibilityTest(){
        try {
            cl.bulkInsert( null );
        }catch ( BaseException e ){
            Assert.assertEquals( SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode() );
        }

        try {
            cl.bulkInsert( null, 0 );
        }catch ( BaseException e ){
            Assert.assertEquals( SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode() );
        }

        try {
            cl.bulkInsert( null, null );
        }catch ( BaseException e ){
            Assert.assertEquals( SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode() );
        }
    }

    @Test
    public void insertWithResultTest(){
        cl.createIndex( "a", new BasicBSONObject("a", 1), true, false);

        BSONObject doc = new BasicBSONObject();
        try {
            // case 1: empty bson
            cl.insertRecord( doc );
        }catch ( BaseException e ){
            Assert.assertEquals( SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode() );
        }
        // empty result
        InsertResult r1 = new InsertResult( null );
        Assert.assertEquals( -1, r1.getInsertNum() );
        Assert.assertEquals( -1, r1.getDuplicatedNum() );
        Assert.assertNull( r1.getOid() );
        Assert.assertNull( r1.getOidList() );
        Assert.assertEquals( -1, r1.getLastGenerateID() );

        // case 2: no oid returned
        doc.put( "a", 1 );
        InsertResult r2 = cl.insertRecord( doc );
        Assert.assertEquals( 1, r2.getInsertNum() );
        Assert.assertEquals( 0, r2.getDuplicatedNum() );
        Assert.assertNull( r2.getOid() );
        Assert.assertNull( r2.getOidList() );

        // case 3: return oid
        InsertOption option = new InsertOption();
        option.setFlag( InsertOption.FLG_INSERT_CONTONDUP );
        option.appendFlag( InsertOption.FLG_INSERT_RETURN_OID );
        InsertResult r3 = cl.insertRecord( doc, option );
        Assert.assertEquals( 0, r3.getInsertNum() );
        Assert.assertEquals( 1, r3.getDuplicatedNum() );
        Assert.assertNotNull( r3.getOid() );
        Assert.assertNull( r3.getOidList() );
        Assert.assertEquals( -1, r3.getLastGenerateID() );
        Assert.assertEquals( 0, r3.getModifiedNum() );

        // case 4: user-defined oid
        doc.put( "_id", "111" );
        InsertResult r4 = cl.insertRecord( doc, option );
        Assert.assertEquals( "111", r4.getOid() );

        List<BSONObject> docList = new ArrayList<>();
        try {
            // case 5: empty list
            InsertResult r5 = cl2.bulkInsert( docList );
        }catch ( BaseException e ){
            Assert.assertEquals( SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode() );
        }

        // case 6: return oid
        docList.add( doc );
        docList.add( new BasicBSONObject("a", 2) );
        InsertResult r6 = cl2.bulkInsert( docList, option );
        Assert.assertEquals( 2, r6.getInsertNum() );
        Assert.assertEquals( 0, r6.getDuplicatedNum() );
        Assert.assertNull( r6.getOid() );
        Assert.assertEquals( 2, r6.getOidList().size());

        // case 7: no oid returned
        option.eraseFlag( InsertOption.FLG_INSERT_RETURN_OID );
        cl2.truncate();
        InsertResult r7 = cl2.bulkInsert( docList, option );
        Assert.assertNull( r7.getOid() );
        Assert.assertNull( r7.getOidList());
    }

    @Test
    public void insertResultTest() {
        String key = "_id";
        // case 1: same oid object
        ObjectId oid1 = ObjectId.get();
        InsertResult result1_1 = new InsertResult( new BasicBSONObject(key, oid1) );
        InsertResult result1_2 = new InsertResult( new BasicBSONObject(key, oid1) );
        // symmetric
        Assert.assertTrue( result1_1.equals( result1_2 ) );
        Assert.assertTrue( result1_2.equals( result1_1 ) );
        Assert.assertEquals( result1_1.hashCode(), result1_2.hashCode() );
        // reflexivity
        Assert.assertTrue( result1_1.equals( result1_1 ) );
        Assert.assertEquals( result1_1.hashCode(), result1_1.hashCode() );

        // case 2: different oid object
        InsertResult result2_1 = new InsertResult( new BasicBSONObject(key, ObjectId.get()) );
        InsertResult result2_2 = new InsertResult( new BasicBSONObject(key, ObjectId.get()) );
        InsertResult result2_3 = new InsertResult( new BasicBSONObject(key, ObjectId.get()) );
        // transitive
        Assert.assertFalse( result2_1.equals( result2_2 ) );
        Assert.assertNotEquals( result2_1.hashCode(), result2_2.hashCode() );
        Assert.assertFalse( result2_1.equals( result2_3 ) );
        Assert.assertNotEquals( result2_1.hashCode(), result2_3.hashCode() );
        Assert.assertFalse( result2_2.equals( result2_3 ) );
        Assert.assertNotEquals( result2_2.hashCode(), result2_3.hashCode() );

        // case 3: same oid but different oid object
        ObjectId oid3_1 = new ObjectId(oid1.toString());
        ObjectId oid3_2 = new ObjectId(oid1.toString());
        Assert.assertEquals( oid3_1, oid3_2 );

        InsertResult result3_1 = new InsertResult( new BasicBSONObject(key, oid3_1) );
        InsertResult result3_2 = new InsertResult( new BasicBSONObject(key, oid3_2) );
        // consistent
        Assert.assertTrue( result3_1.equals( result3_2 ) );
        Assert.assertTrue( result3_1.equals( result3_2 ) );
        Assert.assertEquals( result3_1.hashCode(), result3_2.hashCode() );
    }
}
