package com.sequoiadb.test.cl;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.base.options.InsertOption;
import com.sequoiadb.base.result.InsertResult;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.test.common.Constants;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.junit.*;

import java.util.ArrayList;
import java.util.List;

/**
 * Test SEQUOIADBMAINSTREAM-8889
 */
public class CLInsert8889 {

    private static Sequoiadb sdb;
    private static CollectionSpace cs;
    private static DBCollection cl;
    private static DBCollection hashCL;
    private static final String indexName = "a";
    private static List<String> groupList;

    @BeforeClass
    public static void setConnBeforeClass() throws Exception {
        sdb = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");

        // 1. get group info
        groupList = new ArrayList<>();
        try (DBCursor cursor = sdb.listReplicaGroups()){
            while (cursor.hasNext()) {
                String groupName = (String)cursor.getNext().get("GroupName");
                if (groupName.equals("SYSCatalogGroup") || groupName.equals("SYSCoord")) {
                    continue;
                }
                groupList.add(groupName);
            }
        }
        if (groupList.size() < 2) {
            throw new BaseException(SDBError.SDB_SYS, "At least two data groups are required!");
        }

        // 2. create domain
        BSONObject domainOpt = new BasicBSONObject();
        domainOpt.put("AutoSplit", true);
        domainOpt.put("Groups", groupList);
        sdb.createDomain(Constants.TEST_DOMAIN_NAME, domainOpt);

        // 3. create cs
        if (sdb.isCollectionSpaceExist(Constants.TEST_CS_NAME_1)) {
            sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
        }
        BSONObject csOpt = new BasicBSONObject("Domain", Constants.TEST_DOMAIN_NAME);
        cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1, csOpt);

        // 4. create cl
        cl = cs.createCollection(Constants.TEST_CL_NAME_1);
        BSONObject indexDef = new BasicBSONObject(indexName, 1);
        BSONObject indexOpt = new BasicBSONObject();
        indexOpt.put("Unique", true);
        cl.createIndex(indexName, indexDef, indexOpt);

        // 5. create Sharding cl
        BSONObject hashCLOpt = new BasicBSONObject();
        hashCLOpt.put("ShardingKey", new BasicBSONObject(Constants.OID, 1));
        hashCLOpt.put("ShardingType", "hash");
        hashCLOpt.put("EnsureShardingIndex", true);
        hashCLOpt.put("AutoSplit", true);
        hashCL = cs.createCollection(Constants.TEST_CL_NAME_2, hashCLOpt);
    }

    @AfterClass
    public static void DropConnAfterClass() throws Exception {
        try {
            sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
            sdb.dropDomain(Constants.TEST_DOMAIN_NAME);
        } finally {
            sdb.disconnect();
        }
    }

    @Before
    public void setUp() throws Exception {
        cl.truncate();
        hashCL.truncate();
    }

    @After
    public void tearDown() throws Exception {
    }

    @Test
    public void testHashCL() {
        int totalNumber = 100;
        for(int i = 0; i < totalNumber; i++) {
            BSONObject obj = new BasicBSONObject();
            obj.put("a", i);
            hashCL.insertRecord(obj);
        }

        int nodeMaxNumber = ( totalNumber / groupList.size() ) * 2;
        for (String groupName: groupList) {
            String address = sdb.getReplicaGroup(groupName).getMaster().getNodeName();
            try (Sequoiadb db = new Sequoiadb(address, "", "")) {
                DBCollection cl = db.getCollectionSpace(Constants.TEST_CS_NAME_1).getCollection(Constants.TEST_CL_NAME_2);
                long count = cl.getCount();
                if (count < 0) {
                    Assert.fail("The number of records on the node[" + address + "] must be greater than 0");
                }
                if (count > nodeMaxNumber) {
                    Assert.fail("The number of records on the node[" + address + "] must be less than " + nodeMaxNumber);
                }
            }
        }

        DBCursor cursor = hashCL.query();
        List<Object> errorData = new ArrayList<>();
        while (cursor.hasNext()){
            BSONObject record = cursor.getNext();
            Object oid = record.get(Constants.OID);
            BSONObject result = hashCL.queryOne(new BasicBSONObject(Constants.OID, oid), null, null, null, 0);
            if ( result == null ){
                errorData.add(oid);
            } else {
                Assert.assertEquals(oid, result.get(Constants.OID));
            }
        }
        Assert.assertEquals(errorData.toString(), 0, errorData.size());
    }

    @Test
    public void testInsert() {
        InsertOption option = new InsertOption();
        option.setFlag(InsertOption.FLG_INSERT_RETURN_OID);

        BSONObject record = new BasicBSONObject();
        record.put(indexName, 1);
        InsertResult result1 = cl.insertRecord(record, option);
        Object oid = result1.getOid();

        option.appendFlag(InsertOption.FLG_INSERT_REPLACEONDUP);
        record.put(Constants.OID, 1);
        InsertResult result2 = cl.insertRecord(record, option);

        Assert.assertEquals(1, result2.getDuplicatedNum());
        Assert.assertEquals(1, result2.getOid());

        Assert.assertEquals(oid, cl.queryOne().get(Constants.OID));
    }

    @Test
    public void testBulkInsert() {
        InsertOption option = new InsertOption();
        option.setFlag(InsertOption.FLG_INSERT_RETURN_OID);

        BSONObject record = new BasicBSONObject();
        record.put(indexName, 1);
        List<BSONObject> recordList = new ArrayList<>();
        recordList.add(record);

        InsertResult result1 = cl.bulkInsert(recordList, option);
        Object oid = result1.getOidList().get(0);

        option.appendFlag(InsertOption.FLG_INSERT_REPLACEONDUP);
        record.put(Constants.OID, 1);
        InsertResult result2 = cl.bulkInsert(recordList, option);

        Assert.assertEquals(1, result2.getDuplicatedNum());
        Assert.assertEquals(1, result2.getOidList().get(0));

        Assert.assertEquals(oid, cl.queryOne().get(Constants.OID));
    }

    @Test
    public void testCompatibility() {
        cl.ensureOID(false);

        InsertOption option = new InsertOption();
        option.setFlag(InsertOption.FLG_INSERT_RETURN_OID);
        BSONObject obj = new BasicBSONObject();
        obj.put("b", 123);
        List<BSONObject> objList = new ArrayList<>();
        objList.add(obj);

        Object result1 = cl.insert(obj);
        Assert.assertNotNull(result1);
        Assert.assertNotNull(obj.get(Constants.OID));

        obj.removeField(Constants.OID);
        InsertResult result2 = cl.insertRecord(obj, option);
        Assert.assertNotNull(result2.getOid());
        Assert.assertNotNull(obj.get(Constants.OID));

        obj.removeField(Constants.OID);
        InsertResult result3 = cl.bulkInsert(objList, option);
        Assert.assertNotNull(result3.getOidList().get(0));
        Assert.assertNotNull(obj.get(Constants.OID));
    }
}
