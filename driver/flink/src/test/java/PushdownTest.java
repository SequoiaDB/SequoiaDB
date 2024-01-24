import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import org.apache.flink.configuration.Configuration;
import org.apache.flink.table.api.Table;
import org.apache.flink.table.api.TableEnvironment;
import org.apache.flink.table.api.TableResult;
import org.apache.flink.types.Row;
import org.apache.flink.util.CloseableIterator;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BSONDate;
import org.bson.types.BSONTimestamp;
import org.bson.types.BasicBSONList;
import org.junit.Assert;
import org.junit.Test;

import java.math.BigDecimal;
import java.sql.Timestamp;
import java.time.Instant;
import java.time.LocalDate;

/**
 * test push down function are all normal use,mainly:
 * base type,boolean condition,date condition,null value,and,or,in optimize,
 * error input,reverse,flink expression optimize,complicated condition
 */
public class PushdownTest {
    //Build a simple tableEnvironment
    private static final TableEnvironment tableEnvironment = TableEnvironment.create(new Configuration());

    private static final String con = "192.168.16.83:11810";

    private static final String username = "sdbadmin";

    private static final String password = "sdbadmin";

    private static final String cs = "utCS";

    private static final String cl = "utCL";

    private static final Sequoiadb sequoiadb = new Sequoiadb(con, username, password);

    private static final CollectionSpace collectionSpace;

    private static final DBCollection dbCollection;

    static {
        //create a data source by datagen of Flink Connector
        String datagenSql = "CREATE TABLE IF NOT EXISTS sourceTable (" +
                "id INT," +
                "name STRING," +
                "age DOUBLE," +
                "sex BOOLEAN," +
                "birth TIMESTAMP," +
                "btime TIMESTAMP_LTZ," +
                "atime DATE," +
                "property DECIMAL," +
                "address STRING" +
                ") WITH (" +
                "'connector' = 'datagen'," +
                "'number-of-rows' = '100')";
        tableEnvironment.executeSql(datagenSql);

        String sdbSql = "CREATE TABLE IF NOT EXISTS SDBTable (" +
                "id INT," +
                "name STRING," +
                "age DOUBLE," +
                "sex BOOLEAN," +
                "birth TIMESTAMP," +
                "btime TIMESTAMP_LTZ," +
                "atime DATE," +
                "property DECIMAL," +
                "address STRING," +
                "PRIMARY KEY (id) NOT ENFORCED" +
                ") WITH (" +
                "'connector' = 'sequoiadb'," +
                "'hosts' = '" + con + "'," +
                "'collection' = '" + cl + "'," +
                "'collectionspace' = '" + cs + "'," +
                "'username' = '" + username + "'," +
                "'password' = '" + password + "'," +
                "'overwrite' = 'false')";
        tableEnvironment.executeSql(sdbSql);

        Table sourceTable = tableEnvironment.from("sourceTable");

        //insert data
        sourceTable.executeInsert("SDBTable");

        tableEnvironment.executeSql("select * from sourceTable");
        tableEnvironment.executeSql("select * from SDBTable");

        collectionSpace = sequoiadb.getCollectionSpace(cs);
        dbCollection = collectionSpace.getCollection(cl);
    }

    @Test
    public void instant() {
        TableResult tableResult = tableEnvironment.executeSql("select id from SDBTable where " +
                "btime > '2021-07-22 03:04:46.209000'");

        BSONObject conditions = new BasicBSONObject();
        BSONObject value = new BasicBSONObject();
        value.put("$gt", Timestamp.from(Instant.parse("2021-07-22T03:04:46.2090Z")));
        conditions.put("btime", value);

        CloseableIterator<Row> iterator = tableResult.collect();

        BSONObject selector = new BasicBSONObject();
        selector.put("id", null);

        DBCursor dbCursor = dbCollection.query(conditions, selector, null, null);

        Assert.assertTrue(check(dbCursor, iterator));
    }

    @Test
    public void localDate() {
        TableResult tableResult = tableEnvironment.executeSql("select * from SDBTable where atime > '2021-07-22'");

        BSONObject condition = new BasicBSONObject();
        BSONObject value = new BasicBSONObject();
        value.put("$gt", BSONDate.valueOf(LocalDate.parse("2021-07-22")));
        condition.put("atime", value);

        CloseableIterator<Row> iterator = tableResult.collect();

        BSONObject selector = new BasicBSONObject();
        selector.put("id", null);

        DBCursor dbCursor = dbCollection.query(condition, selector, null, null);

        Assert.assertTrue(check(dbCursor, iterator));
    }

    @Test
    public void localDateTimePrecision() {
        TableResult tableResult = tableEnvironment.executeSql("select * from SDBTable where " +
                "birth < '2022-08-23 22:20:40.778001'");

        BSONObject condition = new BasicBSONObject();
        BSONObject value = new BasicBSONObject();
        value.put("$lt", new BSONTimestamp(Timestamp.valueOf("2022-08-23 00:00:00")));
        condition.put("birth", value);

        CloseableIterator<Row> iterator = tableResult.collect();

        BSONObject selector = new BasicBSONObject();
        selector.put("id", null);

        DBCursor dbCursor = dbCollection.query(condition, selector, null, null);

        Assert.assertFalse(check(dbCursor, iterator));
    }

    @Test
    public void localDateTime() {
        TableResult tableResult = tableEnvironment.executeSql("select * from SDBTable where " +
                "birth > '2021-07-22 10:15:54'");

        BSONObject condition = new BasicBSONObject();
        BSONObject value = new BasicBSONObject();
        value.put("$gt", new BSONTimestamp(Timestamp.valueOf("2021-07-22 10:15:54")));
        condition.put("birth" , value);

        CloseableIterator<Row> iterator = tableResult.collect();

        BSONObject selector = new BasicBSONObject();
        selector.put("id", null);

        DBCursor dbCursor = dbCollection.query(condition, selector, null, null);

        Assert.assertTrue(check(dbCursor, iterator));
    }

    @Test
    public void and() {
        TableResult tableResult = tableEnvironment.executeSql("select * from SDBTable where id > 1 and age > 10");

        BSONObject conditions = new BasicBSONObject();
        BasicBSONList andList = new BasicBSONList();

        BSONObject condition1 = new BasicBSONObject();
        BSONObject value1 = new BasicBSONObject();
        value1.put("$gt", 1);
        condition1.put("id", value1);
        BSONObject condition2 = new BasicBSONObject();
        BSONObject value2 = new BasicBSONObject();
        value2.put("$gt", 10.0);
        condition2.put("age", value2);

        andList.add(condition1);
        andList.add(condition2);

        conditions.put("$and", andList);

        CloseableIterator<Row> iterator = tableResult.collect();

        BSONObject selector = new BasicBSONObject();
        selector.put("id", null);

        DBCursor dbCursor = dbCollection.query(conditions, selector, null, null);

        Assert.assertTrue(check(dbCursor, iterator));
    }

    @Test
    public void or() {
        TableResult tableResult = tableEnvironment.executeSql("select * from SDBTable where id > 1 or " +
                "age > 10.0 or property > 1081 or sex is true");
        CloseableIterator<Row> iterator = tableResult.collect();

        BSONObject conditions = new BasicBSONObject();
        BasicBSONList orList = new BasicBSONList();

        BSONObject condition1 = new BasicBSONObject();
        BSONObject value1 = new BasicBSONObject();
        value1.put("$gt", 1);
        condition1.put("id", value1);
        orList.add(condition1);

        BSONObject condition2 = new BasicBSONObject();
        BSONObject value2 = new BasicBSONObject();
        value2.put("$gt", 10.0);
        condition2.put("age", value2);
        orList.add(condition2);

        BSONObject condition3 = new BasicBSONObject();
        BSONObject value3 = new BasicBSONObject();
        value3.put("$gt", new BigDecimal("1081"));
        condition3.put("property", value3);
        orList.add(condition3);

        BSONObject condition4 = new BasicBSONObject();
        BSONObject value4 = new BasicBSONObject();
        value4.put("$et", true);
        condition4.put("sex", value4);
        orList.add(condition4);

        conditions.put("$or", orList);

        BSONObject selector = new BasicBSONObject();
        selector.put("id", null);

        DBCursor dbCursor = dbCollection.query(conditions, selector, null, null);

        Assert.assertTrue(check(dbCursor, iterator));
    }

    @Test
    public void orInOr() {
        TableResult tableResult = tableEnvironment.executeSql("select * from SDBTable where id > 1 or (age > 10.0" +
                " or (name = 'asdasd' or property > 15.5))");
        CloseableIterator<Row> iterator = tableResult.collect();

        BSONObject conditions = new BasicBSONObject();
        BasicBSONList orList = new BasicBSONList();

        BSONObject condition1 = new BasicBSONObject();
        BSONObject value1 = new BasicBSONObject();
        value1.put("$gt", 1);
        condition1.put("id", value1);
        orList.add(condition1);

        BSONObject condition2 = new BasicBSONObject();
        BSONObject value2 = new BasicBSONObject();
        value2.put("$gt", 10.0);
        condition2.put("age", value2);
        orList.add(condition2);

        BSONObject condition3 = new BasicBSONObject();
        BSONObject value3 = new BasicBSONObject();
        value3.put("$et", "asdasd");
        condition3.put("name", value3);
        orList.add(condition3);

        BSONObject condition4 = new BasicBSONObject();
        BSONObject value4 = new BasicBSONObject();
        value4.put("$gt", new BigDecimal("15.5"));
        condition4.put("property", value4);
        orList.add(condition4);

        conditions.put("$or", orList);

        BSONObject selector = new BasicBSONObject();
        selector.put("id", null);

        DBCursor dbCursor = dbCollection.query(conditions, selector, null, null);

        Assert.assertTrue(check(dbCursor, iterator));
    }

    @Test
    public void multiplex() {
        TableResult tableResult = tableEnvironment.executeSql("select * from SDBTable where id > 1 and age > 10 " +
                "or property < 545665.135 and sex is not false");
        CloseableIterator<Row> iterator = tableResult.collect();

        BSONObject conditions = new BasicBSONObject();
        BasicBSONList andList = new BasicBSONList();

        BSONObject condition1 = new BasicBSONObject();
        BasicBSONList orList1 = new BasicBSONList();

        BSONObject condition11 = new BasicBSONObject();
        BSONObject value11 = new BasicBSONObject();
        value11.put("$gt", 1);
        condition11.put("id", value11);
        orList1.add(condition11);

        BSONObject condition12 = new BasicBSONObject();
        BSONObject value12 = new BasicBSONObject();
        value12.put("$lt", new BigDecimal("545665.135"));
        condition12.put("property", value12);
        orList1.add(condition12);
        condition1.put("$or", orList1);
        andList.add(condition1);


        BSONObject condition2 = new BasicBSONObject();
        BasicBSONList orList2 = new BasicBSONList();

        BSONObject condition21 = new BasicBSONObject();
        BSONObject value21 = new BasicBSONObject();
        value21.put("$gt", 1);
        condition21.put("id", value21);
        orList2.add(condition21);

        BSONObject condition22 = new BasicBSONObject();
        BSONObject value22 = new BasicBSONObject();
        value22.put("$ne", false);
        condition22.put("sex", value22);
        orList2.add(condition22);
        condition2.put("$or", orList2);
        andList.add(condition2);

        BSONObject condition3 = new BasicBSONObject();
        BasicBSONList orList3 = new BasicBSONList();

        BSONObject condition31 = new BasicBSONObject();
        BSONObject value31 = new BasicBSONObject();
        value31.put("$gt", 10);
        condition31.put("age", value31);
        orList3.add(condition31);

        BSONObject condition32 = new BasicBSONObject();
        BSONObject value32 = new BasicBSONObject();
        value32.put("$lt", new BigDecimal("545665.135"));
        condition32.put("property", value32);
        orList3.add(condition32);
        condition3.put("$or", orList3);
        andList.add(condition3);

        BSONObject condition4 = new BasicBSONObject();
        BasicBSONList orList4 = new BasicBSONList();

        BSONObject condition41 = new BasicBSONObject();
        BSONObject value41 = new BasicBSONObject();
        value41.put("$gt", 10);
        condition41.put("age", value41);
        orList4.add(condition41);

        BSONObject condition42 = new BasicBSONObject();
        BSONObject value42 = new BasicBSONObject();
        value42.put("$ne", false);
        condition42.put("sex", value42);
        orList4.add(condition42);
        condition4.put("$or", orList4);
        andList.add(condition4);

        conditions.put("$and", andList);

        BSONObject selector = new BasicBSONObject();
        selector.put("id", null);

        DBCursor dbCursor = dbCollection.query(conditions, selector, null, null);

        Assert.assertTrue(check(dbCursor, iterator));
    }

    @Test
    public void valueEqualValue() {
        TableResult tableResult = tableEnvironment.executeSql("select * from SDBTable where id > 1 and age > 10 " +
                "and 1=1");

        CloseableIterator<Row> iterator = tableResult.collect();

        BSONObject conditions = new BasicBSONObject();
        BasicBSONList andList = new BasicBSONList();

        BSONObject condition1 = new BasicBSONObject();
        BSONObject value1 = new BasicBSONObject();
        value1.put("$gt", 1);
        condition1.put("id", value1);
        andList.add(condition1);

        BSONObject condition2 = new BasicBSONObject();
        BSONObject value2 = new BasicBSONObject();
        value2.put("$gt", 10.0);
        condition2.put("age", value2);
        andList.add(condition2);

        conditions.put("$and", andList);

        BSONObject selector = new BasicBSONObject();
        selector.put("id", null);

        DBCursor dbCursor = dbCollection.query(conditions, selector, null, null);

        Assert.assertTrue(check(dbCursor, iterator));
    }

    @Test
    public void reverse() {
        TableResult tableResult = tableEnvironment.executeSql("select * from SDBTable where 1 < id");

        CloseableIterator<Row> iterator = tableResult.collect();

        BSONObject condition = new BasicBSONObject();
        BSONObject value = new BasicBSONObject();
        value.put("$gt", 1);
        condition.put("id", value);

        BSONObject selector = new BasicBSONObject();
        selector.put("id", null);

        DBCursor dbCursor = dbCollection.query(condition, selector, null, null);

        Assert.assertTrue(check(dbCursor, iterator));
    }

    @Test
    public void errorType() {
        TableResult tableResult = tableEnvironment.executeSql("select * from SDBTable where id > 'asdasd'");

        CloseableIterator<Row> iterator = tableResult.collect();

        Assert.assertFalse(iterator.hasNext());
    }

    @Test
    public void expressionOptimizeTest() {
        TableResult tableResult = tableEnvironment.executeSql("select * from SDBTable where ((id = 1 or age = 10.0)" +
                " and property = 20) or id = 2");

        CloseableIterator<Row> iterator = tableResult.collect();

        Assert.assertFalse(iterator.hasNext());
    }

    @Test
    public void typeCast() {
        TableResult tableResult = tableEnvironment.executeSql("select * from SDBTable where " +
                "id > CAST('2121' AS INTEGER) and age > '25.5' and name = 'asdaqw' and property > '554546.54654'");

        CloseableIterator<Row> iterator = tableResult.collect();

        BSONObject conditions = new BasicBSONObject();
        BasicBSONList andList = new BasicBSONList();

        BSONObject condition1 = new BasicBSONObject();
        BSONObject value1 = new BasicBSONObject();
        value1.put("$gt", 2121);
        condition1.put("id", value1);
        andList.add(condition1);

        BSONObject condition2 = new BasicBSONObject();
        BSONObject value2 = new BasicBSONObject();
        value2.put("$gt", 25.5);
        condition2.put("age", value2);
        andList.add(condition2);

        BSONObject condition3 = new BasicBSONObject();
        BSONObject value3 = new BasicBSONObject();
        value3.put("$et", "asdaqw");
        condition3.put("sex", value3);
        andList.add(condition3);

        BSONObject condition4 = new BasicBSONObject();
        BSONObject value4 = new BasicBSONObject();
        value4.put("$gt", new BigDecimal("554546.54654"));
        condition4.put("property", value4);
        andList.add(condition4);

        conditions.put("$and", andList);

        BSONObject selector = new BasicBSONObject();
        selector.put("id", null);

        DBCursor dbCursor = dbCollection.query(conditions, selector, null, null);

        Assert.assertTrue(check(dbCursor, iterator));
    }

    @Test
    public void in() {
        TableResult tableResult = tableEnvironment.executeSql("select * from SDBTable where id in (1,2,3,4,5)");

        CloseableIterator<Row> iterator = tableResult.collect();

        BSONObject conditions = new BasicBSONObject();
        BasicBSONList orList = new BasicBSONList();

        BSONObject condition1 = new BasicBSONObject();
        BSONObject value1 = new BasicBSONObject();
        value1.put("$et", 1);
        condition1.put("id", value1);
        orList.add(condition1);

        BSONObject condition2 = new BasicBSONObject();
        BSONObject value2 = new BasicBSONObject();
        value2.put("$et", 2);
        condition2.put("id", value2);
        orList.add(condition2);

        BSONObject condition3 = new BasicBSONObject();
        BSONObject value3 = new BasicBSONObject();
        value3.put("$et", 3);
        condition3.put("id", value3);
        orList.add(condition3);

        BSONObject condition4 = new BasicBSONObject();
        BSONObject value4 = new BasicBSONObject();
        value4.put("$et", 4);
        condition4.put("id", value4);
        orList.add(condition4);

        BSONObject condition5 = new BasicBSONObject();
        BSONObject value5 = new BasicBSONObject();
        value5.put("$et", 4);
        condition5.put("id", value5);
        orList.add(condition5);

        conditions.put("$or", orList);

        BSONObject selector = new BasicBSONObject();
        selector.put("id", null);

        DBCursor dbCursor = dbCollection.query(conditions, selector, null, null);

        Assert.assertTrue(check(dbCursor, iterator));
    }

    @Test
    public void isNull() {
        TableResult tableResult = tableEnvironment.executeSql("select * from SDBTable where id > 10 or " +
                "name IS NOT NULL or age > 10 and property > 100");

        CloseableIterator<Row> iterator = tableResult.collect();

        BSONObject conditions = new BasicBSONObject();
        BasicBSONList andList = new BasicBSONList();

        BSONObject condition1 = new BasicBSONObject();
        BasicBSONList orList1 = new BasicBSONList();

        BSONObject condition11 = new BasicBSONObject();
        BSONObject value11 = new BasicBSONObject();
        value11.put("$gt", 10);
        condition11.put("id", value11);
        orList1.add(condition11);

        BSONObject condition12 = new BasicBSONObject();
        BSONObject value12 = new BasicBSONObject();
        value12.put("$isnull", 0);
        condition12.put("name", value12);
        orList1.add(condition12);

        BSONObject condition13 = new BasicBSONObject();
        BSONObject value13 = new BasicBSONObject();
        value13.put("$gt", 10);
        condition13.put("age", value13);
        orList1.add(condition13);
        condition1.put("$or", orList1);
        andList.add(condition1);

        BSONObject condition2 = new BasicBSONObject();
        BasicBSONList orList2 = new BasicBSONList();

        BSONObject condition21 = new BasicBSONObject();
        BSONObject value21 = new BasicBSONObject();
        value21.put("$gt", 10);
        condition21.put("id", value21);
        orList2.add(condition21);

        BSONObject condition22 = new BasicBSONObject();
        BSONObject value22 = new BasicBSONObject();
        value22.put("$isnull", 0);
        condition22.put("name", value22);
        orList2.add(condition22);

        BSONObject condition23 = new BasicBSONObject();
        BSONObject value23 = new BasicBSONObject();
        value23.put("$gt", 100);
        condition23.put("property", value23);
        orList2.add(condition23);
        condition2.put("$or", orList2);
        andList.add(condition1);

        conditions.put("$and", andList);

        BSONObject selector = new BasicBSONObject();
        selector.put("id", null);

        DBCursor dbCursor = dbCollection.query(conditions, selector, null, null);

        Assert.assertTrue(check(dbCursor, iterator));
    }

    @Test
    public void notIn() {
        TableResult tableResult = tableEnvironment.executeSql("select * from SDBTable where id not in (1,2,3,4,5)");

        CloseableIterator<Row> iterator = tableResult.collect();

        BSONObject conditions = new BasicBSONObject();
        BasicBSONList orList = new BasicBSONList();

        BSONObject condition1 = new BasicBSONObject();
        BSONObject value1 = new BasicBSONObject();
        value1.put("$ne", 1);
        condition1.put("id", value1);
        orList.add(condition1);

        BSONObject condition2 = new BasicBSONObject();
        BSONObject value2 = new BasicBSONObject();
        value2.put("$ne", 2);
        condition2.put("id", value2);
        orList.add(condition2);

        BSONObject condition3 = new BasicBSONObject();
        BSONObject value3 = new BasicBSONObject();
        value3.put("$ne", 3);
        condition3.put("id", value3);
        orList.add(condition3);

        BSONObject condition4 = new BasicBSONObject();
        BSONObject value4 = new BasicBSONObject();
        value4.put("$ne", 4);
        condition4.put("id", value4);
        orList.add(condition4);

        BSONObject condition5 = new BasicBSONObject();
        BSONObject value5 = new BasicBSONObject();
        value5.put("$ne", 4);
        condition5.put("id", value5);
        orList.add(condition5);

        conditions.put("$or", orList);

        BSONObject selector = new BasicBSONObject();
        selector.put("id", null);

        DBCursor dbCursor = dbCollection.query(conditions, selector, null, null);

        Assert.assertTrue(check(dbCursor, iterator));
    }

    @Test
    public void complex() {
        TableResult tableResult = tableEnvironment.executeSql("select * from SDBTable where id > 1 or " +
                "birth > '2021-01-01 10:11:10' and age > 10 or property > 1000 and btime > '2021-07-22 03:04:46.209000'" +
                " or (name = 'asdasd' and address = 'asds')");

        CloseableIterator<Row> iterator = tableResult.collect();

        BSONObject conditions = new BasicBSONObject();
        BasicBSONList orList = new BasicBSONList();

        BSONObject condition1 = new BasicBSONObject();
        BSONObject value1 = new BasicBSONObject();
        value1.put("$gt", 1);
        condition1.put("id", value1);
        orList.add(condition1);


        BSONObject condition2 = new BasicBSONObject();
        BasicBSONList andList2 = new BasicBSONList();

        BSONObject condition21 = new BasicBSONObject();
        BSONObject value21 = new BasicBSONObject();
        value21.put("$gt", Timestamp.from(Instant.parse("2021-07-22T03:04:46.2090Z")));
        condition2.put("birth", value21);
        andList2.add(condition21);

        BSONObject condition22 = new BasicBSONObject();
        BSONObject value22 = new BasicBSONObject();
        value22.put("$gt", 10.0);
        condition22.put("age", value22);
        andList2.add(condition22);
        condition2.put("$and", andList2);
        orList.add(condition2);


        BSONObject condition3 = new BasicBSONObject();
        BasicBSONList andList3 = new BasicBSONList();

        BSONObject condition31 = new BasicBSONObject();
        BSONObject value31 = new BasicBSONObject();
        value31.put("$gt", new BigDecimal("1000"));
        condition31.put("property", value31);
        andList3.add(condition31);

        BSONObject condition32 = new BasicBSONObject();
        BSONObject value32 = new BasicBSONObject();
        value32.put("$gt", Timestamp.from(Instant.parse("2021-07-22T03:04:46.2090Z")));
        condition32.put("btime", value32);
        andList3.add(condition32);
        condition3.put("$and", andList3);
        orList.add(condition3);

        BSONObject condition4 = new BasicBSONObject();
        BasicBSONList andList4 = new BasicBSONList();

        BSONObject condition41 = new BasicBSONObject();
        BSONObject value41 = new BasicBSONObject();
        value41.put("$et", "asdasd");
        condition41.put("name", value41);
        andList4.add(condition41);

        BSONObject condition42 = new BasicBSONObject();
        BSONObject value42 = new BasicBSONObject();
        value42.put("$et", "asds");
        condition42.put("address", value42);
        andList4.add(condition42);
        condition4.put("$and", andList4);
        orList.add(condition4);

        conditions.put("$or", orList);

        BSONObject selector = new BasicBSONObject();
        selector.put("id", null);

        DBCursor dbCursor = dbCollection.query(conditions, selector, null, null);

        Assert.assertTrue(check(dbCursor, iterator));
    }

    @Test
    public void trueTest() {
        TableResult tableResult = tableEnvironment.executeSql("select * from SDBTable where sex is true");

        //down pressure condition is empty,which takes a long time
        CloseableIterator<Row> iterator = tableResult.collect();

        BSONObject conditions = new BasicBSONObject();
        BSONObject value = new BasicBSONObject();
        value.put("$et", true);
        conditions.put("sex", value);

        BSONObject selector = new BasicBSONObject();
        selector.put("id", null);

        DBCursor dbCursor = dbCollection.query(conditions, selector, null, null);

        Assert.assertTrue(check(dbCursor, iterator));
    }

    @Test
    public void falseTest() {
        TableResult tableResult = tableEnvironment.executeSql("select * from SDBTable where sex is false");

        //down pressure condition is empty,which takes a long time
        CloseableIterator<Row> iterator = tableResult.collect();

        BSONObject conditions = new BasicBSONObject();
        BSONObject value = new BasicBSONObject();
        value.put("$et", false);
        conditions.put("sex", value);

        BSONObject selector = new BasicBSONObject();
        selector.put("id", null);

        DBCursor dbCursor = dbCollection.query(conditions, selector, null, null);

        Assert.assertTrue(check(dbCursor, iterator));
    }

    @Test
    public void notTrueTest() {
        TableResult tableResult = tableEnvironment.executeSql("select * from SDBTable where sex is not true");

        //down pressure condition is empty,which takes a long time
        CloseableIterator<Row> iterator = tableResult.collect();

        BSONObject conditions = new BasicBSONObject();
        BSONObject value = new BasicBSONObject();
        value.put("$ne", true);
        conditions.put("sex", value);

        BSONObject selector = new BasicBSONObject();
        selector.put("id", null);

        DBCursor dbCursor = dbCollection.query(conditions, selector, null, null);

        Assert.assertTrue(check(dbCursor, iterator));
    }

    @Test
    public void notFalseTest() {
        TableResult tableResult = tableEnvironment.executeSql("select * from SDBTable where sex is not false");

        //down pressure condition is empty,which takes a long time
        CloseableIterator<Row> iterator = tableResult.collect();

        BSONObject conditions = new BasicBSONObject();
        BSONObject value = new BasicBSONObject();
        value.put("$ne", false);
        conditions.put("sex", value);

        BSONObject selector = new BasicBSONObject();
        selector.put("id", null);

        DBCursor dbCursor = dbCollection.query(conditions, selector, null, null);

        Assert.assertTrue(check(dbCursor, iterator));
    }

    public boolean check(DBCursor conditions, CloseableIterator<Row> iterator) {
        boolean flag = true;

        while (iterator.hasNext()) {
            Row row = iterator.next();

            while (conditions.hasNext()) {
                BSONObject bson = conditions.getNext();
                Object value1 = bson.get("id");
                Object value2 = row.getField("id");

                if (value1.equals(value2)) {
                    flag = true;
                    break;
                } else {
                    flag = false;
                }
            }
            if (!flag) {
                break;
            }
        }

        return flag;
    }

}
