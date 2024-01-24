package com.sequoiadb.test.cl;

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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

public class TestIndex {
    private static Sequoiadb sdb;
    private static CollectionSpace cs;
    private static DBCollection cl;
    private static DBCollection mainCL;
    private static DBCollection subCL1;
    private static DBCollection subCL2;

    @BeforeClass
    public static void setConnBeforeClass() throws Exception {

    }

    @AfterClass
    public static void DropConnAfterClass() throws Exception {

    }

    @Before
    public void setUp() throws Exception {
        // sdb
        sdb = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");
        // cs
        if (sdb.isCollectionSpaceExist(Constants.TEST_CS_NAME_1)) {
            sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
        }

        cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
        BSONObject conf = new BasicBSONObject();
        conf.put("ReplSize", 0);
        cl = cs.createCollection(Constants.TEST_CL_NAME_1, conf);
        List<BSONObject> list = ConstantsInsert.createRecordList(100);
        cl.bulkInsert(list, DBCollection.FLG_INSERT_CONTONDUP);

        BSONObject option = new BasicBSONObject();
        option.put("IsMainCL",true);
        option.put("ShardingKey", new BasicBSONObject(Constants.SHARDING_KEY,1));
        option.put("ShardingType","range");
        mainCL = cs.createCollection(Constants.TEST_CL_MAIN_NAME, option);
        subCL1 = cs.createCollection(Constants.TEST_CL_SUB_NAME_1);
        subCL2 = cs.createCollection(Constants.TEST_CL_SUB_NAME_2);
    }

    @After
    public void tearDown() throws Exception {
        sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
        sdb.disconnect();
    }

    @Test
    public void testCreateIndex() {
        String indexName = "testCreateIndex";
        BasicBSONObject key = new BasicBSONObject("a", 1);

        // case 1: createIndex(String indexName, BSONObject indexKeys, boolean isUnique, boolean enforced)
        cl.createIndex(indexName, key, false, false);
        Assert.assertNotNull(cl.getIndexInfo(indexName));
        cl.dropIndex(indexName);

        // case 2: createIndex(String indexName, BSONObject indexKeys, BSONObject indexAttr, BSONObject option)
        cl.createIndex(indexName, key, null, null);
        Assert.assertNotNull(cl.getIndexInfo(indexName));
        cl.dropIndex(indexName);

        BSONObject indexAttr = new BasicBSONObject();
        indexAttr.put("Unique",false);
        indexAttr.put("Enforced",false);
        indexAttr.put("NotNull",false);
        BSONObject option = new BasicBSONObject();
        option.put("SortBufferSize", 100);
        cl.createIndex(indexName, key, indexAttr, option);
        Assert.assertNotNull(cl.getIndexInfo(indexName));
        cl.dropIndex(indexName);

        try {
            indexAttr.put("SortBufferSize", 100);
            cl.createIndex(indexName, key, indexAttr, null);
        }catch (BaseException e){
            assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode());
        }

        // case 3: createIndexAsync(String indexName, BSONObject indexKeys, BSONObject indexAttr, BSONObject option
        long taskId = cl.createIndexAsync(indexName, key, null, null);
        checkTask(taskId, "TaskTypeDesc", "Create index");
    }

    @Test
    public void testGetEmptyIndex() {

        String emptyIndexName = "aaaaaaaaa";
        // case 1:
        DBCursor cursor;
        cursor = cl.getIndex(emptyIndexName);
        while(cursor.hasNext()) {
            System.out.println("index is: " + cursor.getNext());
        }

        // case 2:
        Assert.assertFalse(cl.isIndexExist(emptyIndexName));
        try {
            cl.getIndexInfo(emptyIndexName);
            Assert.fail();
        } catch (BaseException e) {
            Assert.assertEquals(SDBError.SDB_IXM_NOTEXIST.getErrorCode(),
                    e.getErrorCode());
        }

        // case 3:
        String idIdxName = "$id";
        Assert.assertTrue(cl.isIndexExist(idIdxName));
        BSONObject indexObj = cl.getIndexInfo(idIdxName);
        Assert.assertNotNull(indexObj);
        System.out.println("id index is: " + indexObj.toString());

    }

    @Test
    public void testCreateIndexWithOptions(){
        BasicBSONObject key = new BasicBSONObject();
        String name = "name";
        String name2 = "name2";
        String name3 = "name3";
        String name4 = "name4";
        key.put(name,1);
        BSONObject indexObj;

        BasicBSONObject optionsCase1 = new BasicBSONObject();
        cl.createIndex(name,key,optionsCase1);
        indexObj = cl.getIndexInfo(name);
        Assert.assertNotNull(indexObj);
        System.out.println("Case1 index is: " + indexObj.toString());
        cl.dropIndex(name);

        BasicBSONObject optionsCase2 = null;
        cl.createIndex(name2,key,optionsCase2);
        indexObj = cl.getIndexInfo(name2);
        Assert.assertNotNull(indexObj);
        System.out.println("Case2 index is: " + indexObj.toString());
        cl.dropIndex(name2);

        BasicBSONObject optionsCase3 = new BasicBSONObject();
        optionsCase3.put("Unique",true);
        optionsCase3.put("Enforced",true);
        optionsCase3.put("NotNull",false);
        optionsCase3.put("SortBufferSize", 100);
        cl.createIndex(name3, new BasicBSONObject().append("Id", 1),optionsCase3);
        indexObj = cl.getIndexInfo(name3);
        Assert.assertNotNull(indexObj);
        System.out.println("Case3 index is: " + indexObj.toString());
        cl.dropIndex(name3);

        BasicBSONObject optionsCase4 = new BasicBSONObject();
        optionsCase4.put("Unique",true);
        optionsCase4.put("Enforced",false);
        optionsCase4.put("NotNull",false);
        optionsCase4.put("SortBufferSize", 64);
        cl.createIndex(name4,key,optionsCase4);
        indexObj = cl.getIndexInfo(name4);
        Assert.assertNotNull(indexObj);
        System.out.println("Case4 index is: " + indexObj.toString());
        cl.dropIndex(name4);
    }

    @Test
    public void testSnapshotIndexes(){
        String indexName = "testSnapshotIndex";
        cl.createIndex(indexName, new BasicBSONObject(indexName, 1), null);

        BSONObject condition = new BasicBSONObject();
        condition.put("IndexDef", new BasicBSONObject("name", indexName));
        DBCursor cursor = cl.snapshotIndexes(null, null, null, null, 0, -1);
        assertTrue(cursor.hasNext());
        System.out.println(cursor.getNext());
    }

    @Test
    public void testCopyIndex(){
        String indexName1 = "testCopyIndexA";
        String indexName2 = "testCopyIndexB";
        String errorCLName = "errorCL";
        String errorIndexName = "errorIndex";

        // create index in main-cl
        mainCL.createIndex(indexName1, new BasicBSONObject(indexName1, 1), null);
        mainCL.createIndex(indexName2, new BasicBSONObject(indexName2, 1), null);

        // attach sub-cl
        BSONObject obj = new BasicBSONObject();
        obj.put("LowBound", new BasicBSONObject(Constants.SHARDING_KEY,0));
        obj.put("UpBound", new BasicBSONObject(Constants.SHARDING_KEY,1000));
        mainCL.attachCollection(subCL1.getFullName(), obj);
        obj.put("LowBound", new BasicBSONObject(Constants.SHARDING_KEY,1000));
        obj.put("UpBound", new BasicBSONObject(Constants.SHARDING_KEY,2000));
        mainCL.attachCollection(subCL2.getFullName(), obj);

        // case 1: copyIndex(null, "")
        mainCL.copyIndex(null, "");
        subCL1.dropIndex(indexName1);
        subCL1.dropIndex(indexName2);
        subCL2.dropIndex(indexName1);
        subCL2.dropIndex(indexName2);

        // case 2: copyIndex("", null)
        mainCL.copyIndex("", null);
        subCL1.dropIndex(indexName1);
        subCL1.dropIndex(indexName2);
        subCL2.dropIndex(indexName1);
        subCL2.dropIndex(indexName2);

        // case 3: copyIndex(sub_cl, "")
        mainCL.copyIndex(subCL1.getFullName(), null);
        subCL1.dropIndex(indexName1);
        subCL1.dropIndex(indexName2);
        try {
            subCL2.dropIndex(indexName1);
        }catch (BaseException e){
            assertEquals(SDBError.SDB_IXM_NOTEXIST.getErrorCode(), e.getErrorCode());
        }

        // case 4: copyIndex(null, indexName)
        mainCL.copyIndex( null, indexName1);
        subCL1.dropIndex(indexName1);
        subCL2.dropIndex(indexName1);
        try {
            subCL1.dropIndex(indexName2);
        }catch (BaseException e){
            assertEquals(SDBError.SDB_IXM_NOTEXIST.getErrorCode(), e.getErrorCode());
        }

        // case 5: copIndex(sub-cl, indexName)
        mainCL.copyIndex( subCL1.getFullName(), indexName1);
        subCL1.dropIndex(indexName1);

        // case 6: copyIndex(errorCL, indexName)
        try {
            mainCL.copyIndex( errorCLName, indexName1);
        }catch (BaseException e){
            assertEquals(SDBError.SDB_DMS_NOTEXIST.getErrorCode(), e.getErrorCode());
        }

        // case 7: copyIndex(sub_cl, errorIndex)
        try {
            mainCL.copyIndex( subCL1.getFullName(), errorIndexName);
        }catch (BaseException e){
            assertEquals(SDBError.SDB_IXM_NOTEXIST.getErrorCode(), e.getErrorCode());
        }

        // case 8: copyIndexAsync
        long taskId;
        taskId = mainCL.copyIndexAsync( subCL1.getFullName(), indexName1);
        checkTask(taskId, "TaskTypeDesc", "Copy index");

        // case 9: copyIndexAsync(sub_cl, errorIndex)
        try {
            mainCL.copyIndexAsync( subCL1.getFullName(), errorIndexName);
        }catch (BaseException e){
            assertEquals(SDBError.SDB_IXM_NOTEXIST.getErrorCode(), e.getErrorCode());
        }

        mainCL.detachCollection(subCL1.getFullName());
        mainCL.detachCollection(subCL2.getFullName());
        mainCL.dropIndex(indexName1);
        mainCL.dropIndex(indexName2);
    }

    @Test
    public void testDropIndex(){
        String indexName = "testDropIndex";
        String errorIndex = "testDropIndexError";

        // case 1: dropIndex(null)
        try {
            cl.dropIndex(null);
        }catch (BaseException e){
            assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode());
        }

        // case 2: dropIndex("")
        try {
            cl.dropIndex("");
        }catch (BaseException e){
            assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode());
        }

        // case 3: dropIndex(indexName)
        cl.createIndex(indexName, new BasicBSONObject(indexName, 1), null);
        cl.dropIndex(indexName);

        // case 4: dropIndex(errorIndex)
        try {
            cl.dropIndex(errorIndex);
        }catch (BaseException e){
            assertEquals(SDBError.SDB_IXM_NOTEXIST.getErrorCode(), e.getErrorCode());
        }

        // case 5: dropIndexAsync()
        long taskId;
        cl.createIndex(indexName, new BasicBSONObject(indexName, 1), null);
        taskId = cl.dropIndexAsync(indexName);
        checkTask(taskId, "TaskTypeDesc", "Drop index");

        // case 6: dropIndexAsync(errorIndex)
        try {
            cl.dropIndexAsync(errorIndex);
        }catch (BaseException e){
            assertEquals(SDBError.SDB_IXM_NOTEXIST.getErrorCode(), e.getErrorCode());
        }
    }

    @Test
    public void testGetIndexStat(){
        String indexName = "testGetIndexStat";
        String errorIndex = "testGetIndexStatError";

        // case 1, index exist
        cl.createIndex(indexName, new BasicBSONObject(indexName, 1), null);
        sdb.analyze();
        BSONObject obj = cl.getIndexStat(indexName);
        Assert.assertEquals(indexName, obj.get("Index"));
        Assert.assertEquals(cl.getFullName(), obj.get("Collection"));

        // case 2, index no exist
        try {
            cl.getIndexStat(errorIndex);
        }catch (BaseException e){
            assertEquals(SDBError.SDB_IXM_STAT_NOTEXIST.getErrorCode(), e.getErrorCode());
        }
    }

    private void checkTask(long taskId, String fieldName, String expectedValue){
        DBCursor cursor = sdb.listTasks(new BasicBSONObject("TaskID", taskId), null, null, null);
        if (!cursor.hasNext()){
            throw new BaseException(SDBError.SDB_CAT_TASK_NOTFOUND);
        }
        BSONObject obj = cursor.getNext();
        System.out.println(obj);
        assertEquals(expectedValue, obj.get(fieldName));
    }
}