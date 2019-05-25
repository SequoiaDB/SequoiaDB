package com.sequoiadb.test.db;

import com.sequoiadb.base.*;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.test.common.Constants;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.junit.*;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import static org.junit.Assert.assertTrue;

public class SdbConnect {

    private static Sequoiadb sdb;
    private static CollectionSpace cs;
    private static DBCollection cl;
    private static ReplicaGroup rg;
    private static Node node;
    private static DBCursor cursor;

    @BeforeClass
    public static void setConnBeforeClass() throws Exception {
        sdb = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");
    }

    @AfterClass
    public static void DropConnAfterClass() throws Exception {
        sdb.disconnect();
    }

    @Before
    public void setUp() throws Exception {
        if (sdb.isCollectionSpaceExist(Constants.TEST_CS_NAME_1)) {
            sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
            cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
        } else
            cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
        BSONObject conf = new BasicBSONObject();
        conf.put("ReplSize", 0);
        cl = cs.createCollection(Constants.TEST_CL_NAME_1, conf);
    }

    @After
    public void tearDown() throws Exception {
        sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
    }

    @Test
    public void sdbDisconnect() {
        Sequoiadb db = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");
        db.disconnect();
        db.disconnect();
    }

    @Test
    public void sdbConnect() {
        List<String> list = new ArrayList<String>();
        try {
            list.add("192.168.20.35:12340");
            list.add("192.168.20.36:12340");
            list.add("123:123");
            list.add("");
            list.add(":12340");
            list.add("localhost:50000");
            list.add("localhost:11810");
            list.add("localhost:12340");
            list.add(Constants.COOR_NODE_CONN);

            ConfigOptions options = new ConfigOptions();
            options.setMaxAutoConnectRetryTime(0);
            options.setConnectTimeout(10000);
            long begin = 0;
            long end = 0;
            begin = System.currentTimeMillis();
            Sequoiadb sdb1 = new Sequoiadb(list, "", "", options);
            end = System.currentTimeMillis();
            System.out.println("Takes " + (end - begin));
            options.setConnectTimeout(15000);
            sdb1.changeConnectionOptions(options);
            DBCursor cursor = sdb1.getList(4, null, null, null);
            assertTrue(cursor != null);
            sdb1.disconnect();
        } catch (BaseException e) {
            SimpleDateFormat df = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
            System.out.println("Debug info: failed to testcase at: " + df.format(new Date()));
            System.out.println("Debug info: address list is: " + list.toString());
            throw e;
        }
    }


}
