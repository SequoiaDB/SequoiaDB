package com.sequoiadb.test.cl;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.test.common.Constants;
import com.sequoiadb.testdata.ArrayListBean;
import com.sequoiadb.testdata.DecimalBean;
import com.sequoiadb.testdata.ListBean;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BSONDecimal;
import org.bson.types.BasicBSONList;
import org.junit.*;

import java.math.BigDecimal;


public class CLSaveAndAs {
    private static Sequoiadb sdb;
    private static CollectionSpace cs;
    private static DBCollection cl;
    private static DBCursor cur;

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
        cl.truncate();
    }

    /*
     * ok
     */
    @Test
    public void testListBean() {
        BSONObject obj = new BasicBSONObject();
        BSONObject arr = new BasicBSONList();
        arr.put("0", 0);
        arr.put("1", 1);
        arr.put("2", 2);
        obj.put("list", arr);

        try {
            ListBean b1 = obj.as(ListBean.class);
            ListBean b2 = new ListBean();
            Assert.assertEquals(b2, b1);
        } catch (Exception e1) {
            e1.printStackTrace();
            Assert.fail();
        }

        BSONObject retObj = null;
        ListBean outBean = null;
        ListBean inBean = new ListBean();
        try {
            System.out.println("inBean is: " + inBean);
            cl.save(inBean);
            retObj = cl.query().getNext();
            System.out.println("query result is: " + retObj);
            outBean = retObj.as(ListBean.class);
            System.out.println("outBean is: " + outBean);
            Assert.assertEquals(inBean, outBean);
        } catch (Exception e) {
            e.printStackTrace();
            Assert.fail();
        }
    }

    /*
     * TODO:testing, it's not ok
     */
    @Test
    @Ignore
    public void testArrListBean() {
        BSONObject obj = new BasicBSONObject();
        BSONObject arr = new BasicBSONList();
        arr.put("0", 0);
        arr.put("1", 1);
        arr.put("2", 2);
        obj.put("list", arr);

        try {
            ArrayListBean b1 = obj.as(ArrayListBean.class);
            ArrayListBean b2 = new ArrayListBean();
            Assert.assertEquals(b2, b1);
        } catch (Exception e1) {
            e1.printStackTrace();
            Assert.fail();
        }

        BSONObject retObj = null;
        ArrayListBean outBean = null;
        ArrayListBean inBean = new ArrayListBean();
        try {
            System.out.println("inBean is: " + inBean);
            cl.save(inBean);
            retObj = cl.query().getNext();
            System.out.println("query result is: " + retObj);
            outBean = retObj.as(ArrayListBean.class);
            System.out.println("outBean is: " + outBean);
            Assert.assertEquals(inBean, outBean);
        } catch (Exception e) {
            e.printStackTrace();
            Assert.fail();
        }
    }

    @Test
    public void TestDecialSaveAndAs() {
        BSONDecimal d1 = new BSONDecimal("1");
        BigDecimal d2 = new BigDecimal("2");

        DecimalBean bean = new DecimalBean();
        bean.setBsonDecimal(d1);
        bean.setBigDecimal(d2);

        cl.save(bean);
        cur = cl.query();
        BSONObject obj = cur.getNext();

        DecimalBean retBean = null;
        try {
            retBean = obj.as(DecimalBean.class);
        } catch (Exception e) {
            e.printStackTrace();
            Assert.fail();
        }
        System.out.println("retBean is: " + retBean);
    }
}
