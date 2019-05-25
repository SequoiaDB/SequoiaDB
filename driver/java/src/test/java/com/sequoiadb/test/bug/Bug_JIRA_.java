package com.sequoiadb.test.bug;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.test.common.Constants;
import java.lang.IllegalArgumentException;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BSONDecimal;
import org.bson.types.BSONTimestamp;
import org.bson.util.JSON;
import org.bson.util.JSONParseException;
import org.junit.*;
import org.junit.rules.ExpectedException;

import java.math.BigDecimal;
import java.util.Date;


public class Bug_JIRA_ {
    private static Sequoiadb sdb;
    private static CollectionSpace cs;
    private static DBCollection cl;
    @Rule
    public ExpectedException thrown = ExpectedException.none();

    @BeforeClass
    public static void setConnBeforeClass() throws Exception {
        sdb = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");
        if (sdb.isCollectionSpaceExist(Constants.TEST_CS_NAME_1)) {
            sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
            cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
        } else
            cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
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
        cl.delete("");
    }

    @Test
    public void jira2065_Decimal_toBigDecimal() {
        thrown.expect(UnsupportedOperationException.class);
        BSONDecimal bsonDecimal = new BSONDecimal("MIN", 20, 10);
        BigDecimal bigDecimal = bsonDecimal.toBigDecimal();
    }

    @Test
    public void jira2163_insert_invalid_binary() {
        thrown.expect(JSONParseException.class);
        cl.insert("{ a: { '$binary': 'd29ybGQ', '$type': '1' } } ");
    }

    @Test
    public void jira3089_timestamp_throw_null() {
        BSONTimestamp b=new BSONTimestamp();
        Assert.assertEquals(0, b.getTime());
        Assert.assertEquals(0, b.getInc());
    }

    @Test
    public void timestampTest() {
        Date date = new Date(2000 - 1900,0,1,0,0,0);
        BSONObject obj = new BasicBSONObject().append("a", new BSONTimestamp(date));
        cl.insert(obj);
        System.out.println(cl.query().getNext().toString());
    }

    @Test
    public void jira_2100() {
        Sequoiadb mydb = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");
        DBCollection mycl =
            mydb.getCollectionSpace(Constants.TEST_CS_NAME_1).getCollection(Constants.TEST_CL_NAME_1);
        DBCursor cur = mycl.query();
        cur.close();
        mydb.disconnect();
        try {
            mydb.closeAllCursors();
        } catch (BaseException e) {
            Assert.fail(e.toString());
        }
    }


}
