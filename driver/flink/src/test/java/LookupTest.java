import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.DBQuery;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.flink.common.client.SDBClientProvider;
import com.sequoiadb.flink.common.client.SDBCollectionProvider;
import com.sequoiadb.flink.common.exception.SDBException;
import com.sequoiadb.flink.common.util.LookupUtil;
import org.apache.flink.api.java.tuple.Tuple2;
import org.apache.flink.table.api.DataTypes;
import org.apache.flink.table.api.EnvironmentSettings;
import org.apache.flink.table.api.TableEnvironment;
import org.apache.flink.table.types.DataType;
import org.apache.flink.table.types.logical.RowType;
import org.apache.flink.types.Row;
import org.apache.flink.util.CloseableIterator;
import org.apache.flink.util.Preconditions;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.junit.After;
import org.junit.Assert;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Random;


@RunWith(Parameterized.class)
public class LookupTest {

    private static final TableEnvironment tEnv = TableEnvironment.create(EnvironmentSettings.newInstance().inStreamingMode().build());
    private final String hosts;
    private final String username;
    private final String password;
    private final String lookupCsString;
    private final String lookupClString1;
    private final String lookupClString2;
    private final String lookupClString3;
    private static Sequoiadb sdb;
    private CollectionSpace collectionSpace;
    private DBCollection collection1;
    private DBCollection collection2;
    private DBCollection collection3;
    private SDBCollectionProvider lookup1CollectionProvider;

    public LookupTest(String hosts, String username, String password, String lookupCsString, String lookupClString1, String lookupClString2, String lookupClString3) {
        Preconditions.checkNotNull(hosts);
        Preconditions.checkNotNull(username);
        Preconditions.checkNotNull(password);
        Preconditions.checkNotNull(lookupCsString);
        this.hosts = hosts;
        this.username = username;
        this.password = password;
        this.lookupCsString = lookupCsString;
        this.lookupClString1 = lookupClString1;
        this.lookupClString2 = lookupClString2;
        this.lookupClString3 = lookupClString3;
        this.lookup1CollectionProvider = (SDBCollectionProvider) SDBClientProvider.builder()
                .withHosts(Arrays.asList(hosts.split(",")))
                .withCollectionSpace(lookupCsString)
                .withCollection(lookupClString1)
                .withUsername(username)
                .withPassword(password)
                .build();

        try {
            sdb = new Sequoiadb(hosts, username, password);
        } catch (BaseException e) {
            throw new SDBException("cannot get Squoiadb connection.");
        }
        initLookupTest();
    }


    /**
     * Pass in the parameters corresponding to the constructor
     *
     * @return
     */
    @Parameterized.Parameters
    public static Collection initLookupParameters() {
        return Arrays.asList(new String[][]{
                {"192.168.16.187:11810", "sdbadmin", "sdbadmin", "lookupCS", "lookupCL1", "lookupCL2", "lookupCL3"}
        });
    }

    public void initLookupTest() {
        // get a sdb DBCollection connection
        try {
            collectionSpace = sdb.getCollectionSpace(lookupCsString);
        } catch (BaseException e) {
            if (e.getErrorCode() == SDBError.SDB_DMS_CS_NOTEXIST.getErrorCode()) {
                collectionSpace = sdb.createCollectionSpace(lookupCsString);
            } else {
                sdb.close();
                throw e;
            }
        }

        try {
            collection1 = collectionSpace.getCollection(lookupClString1);
        } catch (BaseException e) {
            if (e.getErrorCode() == SDBError.SDB_DMS_NOTEXIST.getErrorCode()) {
                collection1 = collectionSpace.createCollection(lookupClString1);
                initCLWithIndexAndData(collection1);
            } else {
                sdb.close();
                throw e;
            }
        }

        try {
            collection2 = collectionSpace.getCollection(lookupClString2);
        } catch (BaseException e) {
            if (e.getErrorCode() == SDBError.SDB_DMS_NOTEXIST.getErrorCode()) {
                collection2 = collectionSpace.createCollection(lookupClString2);
                initCLWithIndexAndData(collection2);
            } else {
                sdb.close();
                throw e;
            }
        }

        try {
            collection3 = collectionSpace.getCollection(lookupClString3);
        } catch (BaseException e) {
            if (e.getErrorCode() == SDBError.SDB_DMS_NOTEXIST.getErrorCode()) {
                collection3 = collectionSpace.createCollection(lookupClString3);
                initCLWithIndexAndData(collection3);
            } else {
                sdb.close();
                throw e;
            }
        }

        //create a datagen table to emit data
        String datagenSql = "CREATE TABLE IF NOT EXISTS datagentest (" +
                "ID INT," +
                "name STRING," +
                "age INT," +
                "sex BOOLEAN," +
                "birth TIMESTAMP," +
                "btime TIMESTAMP_LTZ," +
                "atime DATE," +
                "property DECIMAL," +
                "address STRING," +
                "proc_time AS PROCTIME()\n" +
                ") WITH (" +
                "'connector' = 'datagen'," +
                "'number-of-rows' = '110'," +
                "'fields.ID.kind'='sequence'," +
                "'fields.ID.start'='1'," +
                "'fields.ID.end'='500'," +
                "'fields.age.min'='10'," +
                "'fields.age.max'='100'," +
                "'fields.name.length'='6')";


        //create a sdb table to combine other streaming table as a dim table
        String sdbSql1 = "CREATE TABLE IF NOT EXISTS sdbtest1 (" +
                "ID INT," +
                "name STRING," +
                "age BIGINT" +
                ") WITH (" +
                "'connector' = 'sequoiadb'," +
                "'hosts' = '" + hosts + "'," +
                "'collection' = '" + lookupClString1 + "'," +
                "'collectionspace' = '" + lookupCsString + "'," +
                "'username' = '" + username + "'," +
                "'password' = '" + password + "'," +
                "'overwrite' = 'false')";

        String sdbSql2 = "CREATE TABLE IF NOT EXISTS sdbtest2 (" +
                "ID INT," +
                "name STRING," +
                "age INT" +
                ") WITH (" +
                "'connector' = 'sequoiadb'," +
                "'hosts' = '" + hosts + "'," +
                "'collection' = '" + lookupClString2 + "'," +
                "'collectionspace' = '" + lookupCsString + "'," +
                "'username' = '" + username + "'," +
                "'password' = '" + password + "'," +
                "'overwrite' = 'false')";

        String sdbSql3 = "CREATE TABLE IF NOT EXISTS sdbtest3 (" +
                "ID INT," +
                "name STRING," +
                "age INT" +
                ") WITH (" +
                "'connector' = 'sequoiadb'," +
                "'hosts' = '" + hosts + "'," +
                "'collection' = '" + lookupClString3 + "'," +
                "'collectionspace' = '" + lookupCsString + "'," +
                "'username' = '" + username + "'," +
                "'password' = '" + password + "'," +
                "'overwrite' = 'false')";

        tEnv.executeSql(datagenSql);
        tEnv.executeSql(sdbSql1);
        tEnv.executeSql(sdbSql2);
        tEnv.executeSql(sdbSql3);
    }

    /**
     * The two result include a true result and an execution result.
     * true if that two result is equal else false.This function of comparison
     * use a hashmap as a true result,the purpose is order to check the right
     * extent of result.Every one from execution results shall be match with
     * all key from the hashmap.This way can ensure result exactly.
     */
    @Test
    public void testLookupOneFieldJoin() {

        String lookupJoinSql = "SELECT d1.ID,s1.ID FROM datagentest AS d1 " +
                "LEFT JOIN sdbtest1 FOR SYSTEM_TIME AS OF d1.proc_time AS s1 ON d1.ID = s1.ID";

        Assert.assertTrue("This sql isn't a Lookup join.", isLookupJoin(lookupJoinSql));

        // get all data from sdb to query  by using map form
        HashMap<Object, BSONObject> integerBSONObjectHashMap = getHashMapWithObjectKey(collection1, "ID");

        // get a concrete result from lookup join tEnv
        CloseableIterator<Row> collect = tEnv.executeSql(lookupJoinSql).collect();

        // compare two result
        Assert.assertTrue(checkLookupWithOneFieldJoin(collect, integerBSONObjectHashMap, "ID", "ID0"));
    }


    @Test
    public void testLookupTwoFieldJoin() {
        String lookupJoinSql = "SELECT d1.ID,d1.age,s1.ID,s1.age FROM datagentest AS d1 " +
                "LEFT JOIN sdbtest1 FOR SYSTEM_TIME AS OF d1.proc_time AS s1 ON d1.ID = s1.ID AND d1.age = s1.age";

        Assert.assertTrue("This sql isn't a Lookup join.", isLookupJoin(lookupJoinSql));
        // get all data from sdb to query  by using map form
        // hashmap : key => Tuple2(ID,age) value => BsonObject
        HashMap<Tuple2<Object, Object>, BSONObject> tuple2BSONObjectHashMap = getHashMapWithTuple2Key(collection1, "ID", "age");

        // get a concrete result from lookup join
        CloseableIterator<Row> lookupReulstCollect = tEnv.executeSql(lookupJoinSql).collect();

        // compare two result
        Assert.assertTrue(checkLookupWithTwoFieldsJoin(lookupReulstCollect, tuple2BSONObjectHashMap, "ID", "age", "ID0", "age0"));
    }

    @Test
    public void testMultiDimTableLookup() {
        String lookupJoinSql = "SELECT d1.ID,d1.age,s1.ID,s2.age,s2.ID,s3.ID FROM datagentest AS d1 " +
                "LEFT JOIN sdbtest1 FOR SYSTEM_TIME AS OF d1.proc_time AS s1 ON d1.ID = s1.ID \n" +
                "LEFT JOIN sdbtest2 FOR SYSTEM_TIME AS OF d1.proc_time AS s2 ON d1.ID = s2.ID AND d1.age = s2.age\n" +
                "LEFT JOIN sdbtest3 FOR SYSTEM_TIME AS OF d1.proc_time AS s3 ON d1.ID = s3.ID";

        Assert.assertTrue("This sql isn't a Lookup join.", isLookupJoin(lookupJoinSql));
        // get sdbtest1 from Sequpiadb as hashmap
        HashMap<Tuple2<Object, Object>, BSONObject> sdbtest1HashMapWithIDAndAge = getHashMapWithTuple2Key(collection1, "ID", "age");

        // get sdbtest2  from Sequpiadb as hashmap
        HashMap<Object, BSONObject> sdbtest2HashMapWithID = getHashMapWithObjectKey(collection2, "ID");

        // get sdbtest3 from Sequoiadb as hasmap
        HashMap<Object, BSONObject> sdbtest3HashMapWithID = getHashMapWithObjectKey(collection3, "ID");

        // get lookupResult List
        CloseableIterator<Row> multiDimLookupResult = tEnv.executeSql(lookupJoinSql).collect();
        List<Row> multiDimLookupList = lookupIteratorToList(multiDimLookupResult);


        // compare result
        Assert.assertTrue(checkLookupWithTwoFieldsJoin(multiDimLookupList.iterator(), sdbtest1HashMapWithIDAndAge, "ID", "age", "ID0", "age0")
                && checkLookupWithOneFieldJoin(multiDimLookupList.iterator(), sdbtest2HashMapWithID, "ID", "ID1")
                && checkLookupWithOneFieldJoin(multiDimLookupList.iterator(), sdbtest3HashMapWithID, "ID", "ID2"));
    }


    @Test
    public void testGetJoinedRowType() {
        // defined a table schema
        DataType produceType = DataTypes.ROW(
                DataTypes.FIELD("a", DataTypes.INT().notNull(), "must be not null"),
                DataTypes.FIELD("b", DataTypes.STRING().nullable(), "should be null"),
                DataTypes.FIELD("c", DataTypes.TIMESTAMP(3)),
                DataTypes.FIELD("d", DataTypes.TIME(3)),
                DataTypes.FIELD("e", DataTypes.BIGINT().notNull()));

        //defined a joinKey[][]
        int[][] joinKeys = {{0}, {4}};

        //defined a result
        List<RowType.RowField> rowFields = new ArrayList<>();
        rowFields.add(new RowType.RowField("a", DataTypes.INT().notNull().getLogicalType()));
        rowFields.add(new RowType.RowField("e", DataTypes.BIGINT().notNull().getLogicalType()));
        RowType result = new RowType(rowFields);

        RowType joinedRowType = LookupUtil.getJoinedRowType(produceType, joinKeys);

        Assert.assertTrue(assertEqual(result, joinedRowType));
    }

    @Test
    public void testIsNestedType() {
        int[][] nestedKey = {{0}, {1, 2}};
        int[][] noNestedKey = {{0}, {2}};
        boolean nestedExpection = true;
        boolean noNestedExpection = false;

        Assert.assertSame(nestedExpection, LookupUtil.isNestedType(nestedKey));
        Assert.assertSame(noNestedExpection, LookupUtil.isNestedType(noNestedKey));
    }

    @Test
    public void testGetIndexColumnNames() {
        List<String> exceptIndexes = new ArrayList<>();
        exceptIndexes.add("_id");
        exceptIndexes.add("ID");
        exceptIndexes.add("ID");
        List<String> indexColumnNames = lookup1CollectionProvider.getIndexColumnNames();

        Assert.assertEquals(exceptIndexes, indexColumnNames);
    }

    public boolean assertEqual(RowType result, RowType joinedRowType) {

        List<RowType.RowField> fields = result.getFields();
        List<RowType.RowField> joinedRowTypeFields = joinedRowType.getFields();

        for (int i = 0; i < fields.size(); i++) {

            RowType.RowField rowField = fields.get(i);
            RowType.RowField joinedRowField = joinedRowTypeFields.get(i);
            if (!rowField.getType().equals(joinedRowField.getType()) || !rowField.getName().equals(joinedRowField.getName())) {
                return false;
            }
        }

        return true;

    }


    public boolean checkLookupWithOneFieldJoin(Iterator<Row> lookupResultCollect, Map<Object, BSONObject> dimMap, String joinedKey, String targetKey) {
        while (lookupResultCollect.hasNext()) {

            Row next = lookupResultCollect.next();
            Object datagenId = next.getField(joinedKey);
            if (dimMap.containsKey(datagenId)) {
                if (dimMap.get(datagenId).get(joinedKey) == next.getField(targetKey)) {
                    continue;
                } else {
                    return false;
                }
            } else if (next.getField(targetKey) != null) {
                return false;
            }
        }
        return true;
    }

    public boolean checkLookupWithTwoFieldsJoin(Iterator<Row> lookupResultCollect, Map<Tuple2<Object, Object>, BSONObject> dimMapWithKeyTuple2,
                                                String joinedKey1, String joinedKey2, String targetKey1, String targetKey2) {

        while (lookupResultCollect.hasNext()) {
            Row next = lookupResultCollect.next();
            Tuple2<Object, Object> key = Tuple2.of(next.getField(joinedKey1), next.getField(joinedKey2));
            if (dimMapWithKeyTuple2.containsKey(key)) {
                if (next.getField(targetKey1).equals(dimMapWithKeyTuple2.get(key).get(joinedKey1))
                        && next.getField(targetKey2).equals(dimMapWithKeyTuple2.get(key).get(joinedKey2))) {
                    continue;
                } else {
                    return false;
                }
            } else if (next.getField(targetKey1) != null || next.getField(targetKey2) != null) {
                return false;
            }
        }


        return true;
    }


    public List<Row> lookupIteratorToList(CloseableIterator<Row> lookupResultCollect) {
        List<Row> lookupList = new ArrayList<>();
        try {
            while (lookupResultCollect.hasNext()) {
                Row next = lookupResultCollect.next();
                lookupList.add(next);
            }
        } catch (Exception e) {
            throw new SDBException("get Row data from lookup Iterator failed.\n", e);
        } finally {
            try {
                if (lookupResultCollect != null) {
                    lookupResultCollect.close();
                }
            } catch (Exception e) {
                throw new SDBException("flink lookup Iterator close failed.\n", e);
            }
        }
        return lookupList;
    }


    public HashMap<Object, BSONObject> getHashMapWithObjectKey(DBCollection cl, String keyField) {
        HashMap<Object, BSONObject> objectBSONObjectHashMap = new HashMap<>();
        DBQuery dbQuery = new DBQuery();
        try (DBCursor dbCursor = cl.query(dbQuery)) {
            while (dbCursor.hasNext()) {
                dbCursor.getNextRaw();
                BSONObject bsonObj = dbCursor.getNext();
                objectBSONObjectHashMap.put(bsonObj.get(keyField), bsonObj);
            }
        } catch (BaseException e) {
            throw new SDBException("\n" + e);
        }
        return objectBSONObjectHashMap;
    }


    public HashMap<Tuple2<Object, Object>, BSONObject> getHashMapWithTuple2Key(DBCollection cl, String keyField1, String keyField2) {
        HashMap<Tuple2<Object, Object>, BSONObject> tuple2BSONObjectHashMap = new HashMap<>();
        DBQuery dbQuery = new DBQuery();

        try (DBCursor dbCursor = cl.query(dbQuery)) {
            while (dbCursor.hasNext()) {
                BSONObject bsonObj = dbCursor.getNext();
                Tuple2<Object, Object> keyTuple2 = Tuple2.of(bsonObj.get(keyField1), bsonObj.get(keyField2));
                tuple2BSONObjectHashMap.put(keyTuple2, bsonObj);
            }
        } catch (BaseException e) {
            throw new SDBException(e);
        }
        return tuple2BSONObjectHashMap;
    }

    /**
     * return true if the sql is a lookup join
     *
     * @param lookupJoinSql a String sql
     * @return boolean
     */

    public boolean isLookupJoin(String lookupJoinSql) {
        String lookupJoinExplain = tEnv.explainSql(lookupJoinSql);
        StringBuilder stringBuilder = new StringBuilder();
        stringBuilder.append("LookupJoin");
        return lookupJoinExplain.contains(stringBuilder);
    }

    public static void initCLWithIndexAndData(DBCollection cl) {

        BSONObject indexKeyBSON = new BasicBSONObject();
        indexKeyBSON.put("ID", 1);
        BSONObject indexOption = new BasicBSONObject();
        indexOption.put("Unique", true);
        indexOption.put("NotNull", true);
        cl.createIndex("lookupIdIndex", indexKeyBSON, indexOption);

        String str = "abcdefghijklmnopqrstyvwxyz1234567890";
        int length = 6;
        int[] num = {100, 1000, 10000};
        Random random = new Random();
        List<BSONObject> bsonList = new ArrayList<>();
        for (int i = 0; i <= num[random.nextInt(3)]; i++) {
            BasicBSONObject bsonObj = new BasicBSONObject();
            bsonObj.put("ID", i);
            bsonObj.put("name", str.substring(random.nextInt(str.length() - length), str.length() - 1));
            bsonObj.put("age", random.nextInt(100));
        }
        try {
            sdb.beginTransaction();
            cl.bulkInsert(bsonList);
            sdb.commit();
        } catch (BaseException e) {
            sdb.close();
            throw new SDBException("failed to bulkinsert data into Sequoiadb.", e);
        }
    }

    @After
    public void close() {
        if (sdb != null) {
            sdb.close();
        }
    }
}
