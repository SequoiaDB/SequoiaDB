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
import org.junit.*;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * Created by tanzhaobo on 2018/1/9.
 */
public class Task_Mix_Func {
    private static SequoiadbDatasource ds;
    private static Sequoiadb sdb;
    private static CollectionSpace cs;
    private static DBCollection cl;

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
        BSONObject csConf = new BasicBSONObject("Domain", "domain3");
        if (sdb.isCollectionSpaceExist(Constants.TEST_CS_NAME_1)) {
            sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
            cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1, csConf);
        } else
            cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1, csConf);
        // cl
        BSONObject clConf;
        clConf = new BasicBSONObject("ReplSize", 0)
                .append("ShardingKey", new BasicBSONObject("a", 1))
                .append("Group", "group1");
        cl = cs.createCollection(Constants.TEST_CL_NAME_1, clConf);
        System.out.println(sdb.getHost() + ", " + clConf);
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
                    .append("d", sdb.getHost())
                    .append("e", i));
        }
    }

    void prepare_data(DBCollection cl) {
        cl.insert(objList);
    }

    void floor_ceiling_test(DBCollection cl) {
        BSONObject matcher;
        BSONObject selector;
        DBCursor cursor;
        BSONObject object;
        double value;

        // use index b
        for (int i = 0; i < doubleValues.size(); i++) {
            value = doubleValues.get(i);
            matcher = new BasicBSONObject().append("a", value);
            selector = new BasicBSONObject()
                    .append("a", new BasicBSONObject("$floor", 1))
                    .append("b", new BasicBSONObject("$ceiling", 1));
            cursor = cl.query(matcher, selector, null, null);
            object = cursor.getNext();
            Assert.assertEquals(value, object.get("a"));
            Assert.assertEquals(value, object.get("b"));
            Assert.assertEquals(value, object.get("c"));
        }
    }

    void mod_test(DBCollection cl) {
        BSONObject matcher;
        BSONObject selector;
        DBCursor cursor;
        BSONObject object;
        double value;

        for (int i = 0; i < doubleValues.size(); i++) {
            value = doubleValues.get(i);
            matcher = new BasicBSONObject().append("a", value);
            selector = new BasicBSONObject()
                    .append("a", new BasicBSONObject("$mod", 10000));
            cursor = cl.query(matcher, selector, null, null);
            display_records(cursor, "return record is: ");
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
        cl.insert(objList);
        mod_test(cl);
    }





}
