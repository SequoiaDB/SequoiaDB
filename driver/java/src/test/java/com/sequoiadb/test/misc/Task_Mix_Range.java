package com.sequoiadb.test.misc;

import com.sequoiadb.base.*;
import com.sequoiadb.datasource.DatasourceOptions;
import com.sequoiadb.datasource.SequoiadbDatasource;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.net.ConfigOptions;
import com.sequoiadb.test.common.Constants;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.junit.*;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * Created by tanzhaobo on 2018/1/9.
 */
public class Task_Mix_Range {
    private static SequoiadbDatasource ds;
    private static Sequoiadb sdb;
    private static CollectionSpace cs;
    private static DBCollection cl;

    @BeforeClass
    public static void setConnBeforeClass() {
        ConfigOptions configOptions = new ConfigOptions();
        configOptions.setSocketTimeout(5000);
        DatasourceOptions datasourceOptions = new DatasourceOptions();
        ds = new SequoiadbDatasource(Arrays.asList(Constants.COOR_NODE_CONN), "", "",
                configOptions, datasourceOptions);
        // sdb
        try {
            sdb = ds.getConnection();
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
        // cs
        if (sdb.isCollectionSpaceExist(Constants.TEST_CS_NAME_1)) {
            sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
            cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
        } else
            cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
        // cl
        BSONObject clConf = new BasicBSONObject("ReplSize", 0)
                .append("ShardingKey", new BasicBSONObject("a", 1))
                .append("ShardingType", "range")
                .append("Group", "group1");
        cl = cs.createCollection(Constants.TEST_CL_NAME_1, clConf);
        // split
        double double1 = -29223372036854775808.12345;
        double double2 = 29223372036854775808.12345;
        cl.split("group1", "group2",
                new BasicBSONObject("a", double1),
                new BasicBSONObject("a", double2));
        cl.split("group1", "group3",
                new BasicBSONObject("a", double2), null);
        System.out.println(sdb.getHost());
    }

    @AfterClass
    public static void DropConnAfterClass() throws Exception {
        try {
//            sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
        } catch (Exception e) {
            e.printStackTrace();
        }
        ds.releaseConnection(sdb);
    }

    @Before
    public void setUp() throws Exception {
    }

    @After
    public void tearDown() throws Exception {
        try {
//            cl.delete("");
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    static double[] doubleValues = new double[10];
    static List<BSONObject> objList = new ArrayList<BSONObject>();

    static void init_data() {
        doubleValues[0] = -9223372036854775808.12345;
        doubleValues[1] = -19223372036854775808.12345;
        doubleValues[2] = -29223372036854775808.12345;
        doubleValues[3] = -39223372036854775808.12345;
        doubleValues[4] = -49223372036854775808.12345;

        doubleValues[5] = 9223372036854775808.12345;
        doubleValues[6] = 19223372036854775808.12345;
        doubleValues[7] = 29223372036854775808.12345;
        doubleValues[8] = 39223372036854775808.12345;
        doubleValues[9] = 49223372036854775808.12345;

//        doubleValues[0] = 8.12345;
//        doubleValues[1] = 808.12345;
//        doubleValues[2] = 5808.12345;
//        doubleValues[3] = 75808.12345;
//        doubleValues[4] = 775808.12345;
//        doubleValues[5] = 4775808.12345;
//        doubleValues[6] = 54775808.12345;
//        doubleValues[7] = 854775808.12345;
//        doubleValues[8] = 6854775808.12345;
//        doubleValues[9] = 36854775808.12345;

        double value = 0;
        value = doubleValues[0];
        objList.add(new BasicBSONObject().append("a", value).append("b", value).append("c", value));
        value = doubleValues[1];
        objList.add(new BasicBSONObject().append("a", value).append("b", value).append("c", value));
        value = doubleValues[2];
        objList.add(new BasicBSONObject().append("a", value).append("b", value).append("c", value));
        value = doubleValues[3];
        objList.add(new BasicBSONObject().append("a", value).append("b", value).append("c", value));
        value = doubleValues[4];
        objList.add(new BasicBSONObject().append("a", value).append("b", value).append("c", value));
        value = doubleValues[5];
        objList.add(new BasicBSONObject().append("a", value).append("b", value).append("c", value));
        value = doubleValues[6];
        objList.add(new BasicBSONObject().append("a", value).append("b", value).append("c", value));
        value = doubleValues[7];
        objList.add(new BasicBSONObject().append("a", value).append("b", value).append("c", value));
        value = doubleValues[8];
        objList.add(new BasicBSONObject().append("a", value).append("b", value).append("c", value));
        value = doubleValues[9];
        objList.add(new BasicBSONObject().append("a", value).append("b", value).append("c", value));
    }

    void prepare_data(DBCollection cl) {
        cl.insert(objList);
    }

    void query_explain_test(DBCollection cl) {
        BSONObject matcher;
        DBCursor cursor;
        BSONObject object;
        double value;

        // use index b
        value = doubleValues[0];
        matcher = new BasicBSONObject().append("b", value);
        cursor = cl.explain(matcher, null, null, null, 0, -1, 0,
                new BasicBSONObject("Run", true));
        display_records(cursor, "explain from index b:" + matcher);
        cursor = cl.query(matcher, null, null, null);
        object = cursor.getNext();
        Assert.assertEquals(value, object.get("a"));
        Assert.assertEquals(value, object.get("b"));
        Assert.assertEquals(value, object.get("c"));

        value = doubleValues[1];
        matcher = new BasicBSONObject().append("b", value);
        cursor = cl.explain(matcher, null, null, null, 0, -1, 0,
                new BasicBSONObject("Run", true));
        display_records(cursor, "explain from index b:" + matcher.toString());
        cursor = cl.query(matcher, null, null, null);
        object = cursor.getNext();
        Assert.assertEquals(value, object.get("a"));
        Assert.assertEquals(value, object.get("b"));
        Assert.assertEquals(value, object.get("c"));

        // use index ab
        value = doubleValues[0];
        matcher = new BasicBSONObject().append("a", value).append("b", value);
        cursor = cl.explain(matcher, null, null, null, 0, -1, 0,
                new BasicBSONObject("Run", true));
        display_records(cursor, "explain from index ab:" + matcher.toString());
        cursor = cl.query(matcher, null, null, null);
        object = cursor.getNext();
        Assert.assertEquals(value, object.get("a"));
        Assert.assertEquals(value, object.get("b"));
        Assert.assertEquals(value, object.get("c"));

        value = doubleValues[1];
        matcher = new BasicBSONObject().append("a", value).append("b", value);
        cursor = cl.explain(matcher, null, null, null, 0, -1, 0,
                new BasicBSONObject("Run", true));
        display_records(cursor, "explain from index ab:" + matcher.toString());
        cursor = cl.query(matcher, null, null, null);
        object = cursor.getNext();
        Assert.assertEquals(value, object.get("a"));
        Assert.assertEquals(value, object.get("b"));
        Assert.assertEquals(value, object.get("c"));

        // use idx ca
        value = doubleValues[0];
        matcher = new BasicBSONObject().append("c", value).append("a", value);
        cursor = cl.explain(matcher, null, null, null, 0, -1, 0,
                new BasicBSONObject("Run", true));
        display_records(cursor, "explain from index ca:" + matcher.toString());
        cursor = cl.query(matcher, null, null, null);
        object = cursor.getNext();
        Assert.assertEquals(value, object.get("a"));
        Assert.assertEquals(value, object.get("b"));
        Assert.assertEquals(value, object.get("c"));

        value = doubleValues[5];
        matcher = new BasicBSONObject().append("c", value).append("a", value);
        cursor = cl.explain(matcher, null, null, null, 0, -1, 0,
                new BasicBSONObject("Run", true));
        display_records(cursor, "explain from index ca:" + matcher.toString());
        cursor = cl.query(matcher, null, null, null);
        object = cursor.getNext();
        Assert.assertEquals(value, object.get("a"));
        Assert.assertEquals(value, object.get("b"));
        Assert.assertEquals(value, object.get("c"));

        // use idx cba
        value = doubleValues[0];
        matcher = new BasicBSONObject().append("c", value).append("b", value).append("a", value);
        cursor = cl.explain(matcher, null, null, null, 0, -1, 0,
                new BasicBSONObject("Run", true));
        display_records(cursor, "explain from index cba:" + matcher.toString());
        cursor = cl.query(matcher, null, null, null);
        object = cursor.getNext();
        Assert.assertEquals(value, object.get("a"));
        Assert.assertEquals(value, object.get("b"));
        Assert.assertEquals(value, object.get("c"));

        value = doubleValues[9];
        matcher = new BasicBSONObject().append("c", value).append("b", value).append("a", value);
        cursor = cl.explain(matcher, null, null, null, 0, -1, 0,
                new BasicBSONObject("Run", true));
        display_records(cursor, "explain from index cba:" + matcher.toString());
        cursor = cl.query(matcher, null, null, null);
        object = cursor.getNext();
        Assert.assertEquals(value, object.get("a"));
        Assert.assertEquals(value, object.get("b"));
        Assert.assertEquals(value, object.get("c"));

    }

    void display_records(DBCursor cursor, String msg) {
        System.out.println("---------------: " + msg);
        while (cursor.hasNext()) {
            System.out.println("record is: " + cursor.getNext());
        }
    }

    @Test
    @Ignore
    public void index_test() {

        init_data();
        cl.createIndex("b_idx", new BasicBSONObject("b", 1), false, false);
        cl.createIndex("ab_idx", new BasicBSONObject("a", 1).append("b", 1), false, false);
        cl.createIndex("ca_idx", new BasicBSONObject("c", 1).append("a", 1), true, false);
        cl.createIndex("bca_idx", new BasicBSONObject("b", 1).append("c", 1).append("a", 1), true, false);
        cl.insert(objList);
        query_explain_test(cl);
    }





}
