package com.sequoiadb.test.cl;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.test.common.Constants;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.junit.*;

import java.util.ArrayList;
import java.util.List;

import static org.junit.Assert.assertEquals;


public class CLAggregate {

    private static Sequoiadb sdb;
    private static CollectionSpace cs;
    private static DBCollection cl;
    private static DBCursor cursor;

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
    }

    @AfterClass
    public static void DropConnAfterClass() throws Exception {

        sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
        sdb.disconnect();
    }

    @Before
    public void setUp() throws Exception {
    }

    @After
    public void tearDown() throws Exception {
    }

    @Test
    public void aggregate() {

        String[] command = new String[2];
        command[0] = "{$match:{status:\"A\"}}";
        command[1] = "{$group:{_id:\"$cust_id\",total:{$sum:\"$amount\"}}}";
        String[] record = new String[4];
        record[0] = "{cust_id:\"A123\",amount:500,status:\"A\"}";
        record[1] = "{cust_id:\"A123\",amount:250,status:\"A\"}";
        record[2] = "{cust_id:\"B212\",amount:200,status:\"A\"}";
        record[3] = "{cust_id:\"A123\",amount:300,status:\"D\"}";
        // insert record into database
        for (int i = 0; i < record.length; i++) {
            BSONObject obj = new BasicBSONObject();
            obj = (BSONObject) JSON.parse(record[i]);
            System.out.println("Record is: " + obj.toString());
            cl.insert(obj);
        }
        List<BSONObject> list = new ArrayList<BSONObject>();
        for (int i = 0; i < command.length; i++) {
            BSONObject obj = new BasicBSONObject();
            obj = (BSONObject) JSON.parse(command[i]);
            list.add(obj);
        }

        cursor = cl.aggregate(list);

        int i = 0;
        while (cursor.hasNext()) {
            System.out.println("Result is: " + cursor.getNext().toString());
            i++;
        }
        assertEquals(2, i);
    }
}
