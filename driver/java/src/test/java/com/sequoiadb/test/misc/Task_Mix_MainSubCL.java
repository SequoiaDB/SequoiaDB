package com.sequoiadb.test.misc;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.datasource.DatasourceOptions;
import com.sequoiadb.datasource.SequoiadbDatasource;
import com.sequoiadb.net.ConfigOptions;
import com.sequoiadb.test.common.Constants;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.MaxKey;
import org.bson.types.MinKey;
import org.junit.*;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * Created by tanzhaobo on 2018/1/9.
 */
public class Task_Mix_MainSubCL {
    private static SequoiadbDatasource ds;
    private static Sequoiadb sdb;
    private static CollectionSpace cs;
    private static DBCollection cl;
    private static DBCollection subcl1;
    private static DBCollection subcl2;
    private static DBCollection subcl3;
    private static boolean useSingleKey = false;

    @BeforeClass
    public static void setConnBeforeClass() {
        ConfigOptions configOptions = new ConfigOptions();
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
                .append("IsMainCL", true);
        cl = cs.createCollection(Constants.TEST_CL_NAME_1, clConf);
        subcl1 = cs.createCollection("subcl1");
        subcl2 = cs.createCollection("subcl2");
        subcl3 = cs.createCollection("subcl3");

        // attach
        double double1 = -29223372036854775808.12345;
        double double2 = 29223372036854775808.12345;
        BSONObject conf1;
        BSONObject conf2;
        BSONObject conf3;

        if (useSingleKey) {
            conf1 = new BasicBSONObject()
                    .append("LowBound", new BasicBSONObject("a", new MinKey()))
                    .append("UpBound", new BasicBSONObject("a", double1));
            conf2 = new BasicBSONObject()
                    .append("LowBound", new BasicBSONObject("a", double1))
                    .append("UpBound", new BasicBSONObject("a", double2));
            conf3 = new BasicBSONObject()
                    .append("LowBound", new BasicBSONObject("a", double2))
                    .append("UpBound", new BasicBSONObject("a", new MaxKey()));
        } else {
            conf1 = new BasicBSONObject()
                    .append("LowBound", new BasicBSONObject("a", new MinKey()).append("b", new MinKey()))
                    .append("UpBound", new BasicBSONObject("a", double1).append("b", double1));
            conf2 = new BasicBSONObject()
                    .append("LowBound", new BasicBSONObject("a", double1).append("b", double1))
                    .append("UpBound", new BasicBSONObject("a", double2).append("b", double2));
            conf3 = new BasicBSONObject()
                    .append("LowBound", new BasicBSONObject("a", double2).append("b", double2))
                    .append("UpBound", new BasicBSONObject("a", new MaxKey()).append("b", new MaxKey()));
        }

        cl.attachCollection(subcl1.getFullName(), conf1);
        cl.attachCollection(subcl2.getFullName(), conf2);
        cl.attachCollection(subcl3.getFullName(), conf3);

        System.out.println(sdb.getHost() + ", useSingleKey: " + useSingleKey);
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

    static List<Double> doubleValues = new ArrayList<Double>();
    static List<BSONObject> objList = new ArrayList<BSONObject>();

    static void init_data() {
        doubleValues.add(-9223372036854775808.12345);
        doubleValues.add(-19223372036854775808.12345);
        doubleValues.add(-29223372036854775808.12345);
        doubleValues.add(-39223372036854775808.12345);
        doubleValues.add(-49223372036854775808.12345);
        doubleValues.add(-59223372036854775808.12345);
        doubleValues.add(-69223372036854775808.12345);
        doubleValues.add(-79223372036854775808.12345);
        doubleValues.add(-89223372036854775808.12345);
        doubleValues.add(-99223372036854775808.12345);

//        doubleValues.add(-8.12345);
//        doubleValues.add(-808.12345);
//        doubleValues.add(-5808.12345);
//        doubleValues.add(-75808.12345);
//        doubleValues.add(-775808.12345);
//        doubleValues.add(-4775808.12345);
//        doubleValues.add(-54775808.12345);
//        doubleValues.add(-854775808.12345);
//        doubleValues.add(-6854775808.12345);
//        doubleValues.add(-36854775808.12345);

        doubleValues.add(9223372036854775808.12345);
        doubleValues.add(19223372036854775808.12345);
        doubleValues.add(29223372036854775808.12345);
        doubleValues.add(39223372036854775808.12345);
        doubleValues.add(49223372036854775808.12345);
        doubleValues.add(59223372036854775808.12345);
        doubleValues.add(69223372036854775808.12345);
        doubleValues.add(79223372036854775808.12345);
        doubleValues.add(89223372036854775808.12345);
        doubleValues.add(99223372036854775808.12345);

        for (int i = 0; i < doubleValues.size(); i++) {
            double value = doubleValues.get(i);
            objList.add(new BasicBSONObject()
                    .append("a", value)
                    .append("b", value)
                    .append("c", value)
                    .append("d", sdb.getHost()));
        }
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
        for (int i = 0; i < doubleValues.size(); i++) {
            value = doubleValues.get(i);
            matcher = new BasicBSONObject().append("b", value);
            cursor = cl.explain(matcher, null, null, null, 0, -1, 0,
                    new BasicBSONObject("Run", true));
            display_records(cursor, "explain from index b:" + matcher);
            cursor = cl.query(matcher, null, null, null);
            object = cursor.getNext();
            Assert.assertEquals(value, object.get("a"));
            Assert.assertEquals(value, object.get("b"));
            Assert.assertEquals(value, object.get("c"));
        }

        // use index ab
        for (int i = 0; i < doubleValues.size(); i++) {
            value = doubleValues.get(i);
            matcher = new BasicBSONObject().append("a", value).append("b", value);
            cursor = cl.explain(matcher, null, null, null, 0, -1, 0,
                    new BasicBSONObject("Run", true));
            display_records(cursor, "explain from index ab:" + matcher.toString());
            cursor = cl.query(matcher, null, null, null);
            object = cursor.getNext();
            Assert.assertEquals(value, object.get("a"));
            Assert.assertEquals(value, object.get("b"));
            Assert.assertEquals(value, object.get("c"));
        }

        // use idx ca
        for (int i = 0; i < doubleValues.size(); i++) {
            value = doubleValues.get(i);
            matcher = new BasicBSONObject().append("c", value).append("a", value);
            cursor = cl.explain(matcher, null, null, null, 0, -1, 0,
                    new BasicBSONObject("Run", true));
            display_records(cursor, "explain from index ca:" + matcher.toString());
            cursor = cl.query(matcher, null, null, null);
            object = cursor.getNext();
            Assert.assertEquals(value, object.get("a"));
            Assert.assertEquals(value, object.get("b"));
            Assert.assertEquals(value, object.get("c"));
        }

        // use idx cba
        for (int i = 0; i < doubleValues.size(); i++) {
            value = doubleValues.get(i);
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
        if (useSingleKey) {
            cl.createIndex("b_idx", new BasicBSONObject("b", 1), false, false);
            cl.createIndex("ab_idx", new BasicBSONObject("a", 1).append("b", 1), false, false);
            cl.createIndex("ca_idx", new BasicBSONObject("c", 1).append("a", 1), true, false);
            cl.createIndex("bca_idx", new BasicBSONObject("b", 1).append("c", 1)
                    .append("a", 1), true, false);
        } else {
            cl.createIndex("b_idx", new BasicBSONObject("b", 1), false, false);
            cl.createIndex("cab_idx", new BasicBSONObject("c", 1).append("a", 1)
                    .append("b", 1), true, false);
            cl.createIndex("bca_idx", new BasicBSONObject("b", 1).append("c", 1)
                    .append("a", 1), true, false);
        }
        cl.insert(objList);
        query_explain_test(cl);
    }





}
