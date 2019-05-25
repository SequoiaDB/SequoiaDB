package com.sequoiadb.test.cl;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.test.common.Constants;
import com.sequoiadb.testdata.*;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.junit.*;

import java.util.*;

import static org.junit.Assert.*;


/**
 * @author tanzhaobo
 */
public class CLSaveWithListArg {
    static private Sequoiadb sdb = null;
    static private CollectionSpace cs = null;
    static private DBCollection cl = null;

    @BeforeClass
    public static void setUpBeforeClass() throws Exception {
        sdb = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");
    }

    @AfterClass
    public static void tearDownAfterClass() throws Exception {
        sdb.disconnect();
    }

    /**
     * @throws Exception
     */
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

    /**
     * @throws Exception
     */
    @After
    public void tearDown() throws Exception {
        sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
    }

    @Test
    public void testBasiceType() throws Exception {
        final int num = 10;
        List<BasicClass> list = new ArrayList<BasicClass>();
        try {
            for (int i = 1; i <= num; i++) {
                BasicClass basicObj = new BasicClass();
                basicObj.setId(i);
                list.add(basicObj);
            }
            cl.save(list);
            DBCursor cursor = cl.query();
            BasicClass asBasicObj = null;
            BSONObject bsonObj = null;
            long count = 0;
            while (cursor != null && cursor.hasNext()) {
                count++;
                bsonObj = cursor.getNext();
                asBasicObj = bsonObj.as(BasicClass.class);
                assertTrue(asBasicObj.getId() > 0 && asBasicObj.getId() <= num);
            }

        } catch (Exception e) {
            e.printStackTrace();
            fail();
        }
    }

    @Test
    public void testMapType() throws Exception {
        final int max = 50;
        final int min = 20;
        List<HaveMapPropClass> list = new ArrayList<HaveMapPropClass>();
        for (int num = min; num <= max; num++) {
            HaveMapPropClass basicObj = new HaveMapPropClass();
            Map<String, String> pros = new HashMap<String, String>();
            pros.put("company", "sequoiadb");
            pros.put("hr", "xiaoying");
            pros.put("address", "panyu");
            basicObj.setMapProp(pros);
            Map<String, User> users = new HashMap<String, User>();

            Student d = new Student(num, "d");
            Student e = new Student(num, "e");

            Map<String, Student> stu1 = new HashMap<String, Student>();
            stu1.put("d", d);
            stu1.put("e", e);

            Student f = new Student(num, "f");
            Student g = new Student(num, "g");

            Map<String, Student> stu2 = new HashMap<String, Student>();
            stu2.put("f", f);
            stu2.put("g", g);


            Student h = new Student(num, "h");
            Student i = new Student(num, "i");
            Map<String, Student> stu3 = new HashMap<String, Student>();
            stu3.put("h", h);
            stu3.put("i", i);


            User a = new User(33, "a", stu1);
            User b = new User(33, "b", stu2);
            User c = new User(33, "c", stu3);
            users.put("a", a);
            users.put("b", b);
            users.put("c", c);
            basicObj.setUserMap(users);

            list.add(basicObj);
        }
        cl.save(list);
        DBCursor cursor = cl.query();
        BSONObject bsonObj = null;
        HaveMapPropClass asBasicObj = null;
        while (cursor != null && cursor.hasNext()) {
            bsonObj = cursor.getNext();
            asBasicObj = bsonObj.as(HaveMapPropClass.class);
            Set<String> keys = asBasicObj.getUserMap().keySet();
            for (String str : keys) {
                Set<String> subKeys = asBasicObj.getUserMap().get(str).getStudent().keySet();
                for (String s : subKeys) {
                    Student student = asBasicObj.getUserMap().get(str).getStudent().get(s);
                    assertTrue((student.getAge() >= min) && (student.getAge() <= max));
                }
            }
        }
    }

    @Test
    public void testMapTypeWithMainKeys() throws Exception {
        List<HaveMapPropClass> list = new ArrayList<HaveMapPropClass>();
        final int max = 50;
        final int min = 20;
        for (int num = min; num < max; num++) {
            HaveMapPropClass basicObj = new HaveMapPropClass();
            Map<String, String> pros = new HashMap<String, String>();
            pros.put("company", "sequoiadb");
            pros.put("hr", "xiaoying");
            pros.put("address", "panyu");
            basicObj.setMapProp(pros);
            Map<String, User> users = new HashMap<String, User>();

            Student d = new Student(44, "d");
            Student e = new Student(44, "e");

            Map<String, Student> stu1 = new HashMap<String, Student>();
            stu1.put("d", d);
            stu1.put("e", e);

            Student f = new Student(44, "f");
            Student g = new Student(44, "g");

            Map<String, Student> stu2 = new HashMap<String, Student>();
            stu2.put("f", f);
            stu2.put("g", g);


            Student h = new Student(44, "h");
            Student i = new Student(44, "i");
            Map<String, Student> stu3 = new HashMap<String, Student>();
            stu3.put("h", h);
            stu3.put("i", i);


            User a = new User(33, "a", stu1);
            User b = new User(33, "b", stu2);
            User c = new User(33, "c", stu3);
            users.put("a", a);
            users.put("b", b);
            users.put("c", c);
            basicObj.setUserMap(users);
            list.add(basicObj);
        }
        cl.save(list);
        List<HaveMapPropClass> list1 = new ArrayList<HaveMapPropClass>();
        for (int num = min; num < max; num++) {
            HaveMapPropClass basicObj = new HaveMapPropClass();
            Map<String, String> pros = new HashMap<String, String>();
            pros.put("company", "sequoiadb");
            pros.put("hr", "xiaoying");
            pros.put("address", "panyu");
            basicObj.setMapProp(pros);
            Map<String, User> users = new HashMap<String, User>();

            Student d = new Student(44, "d");
            Student e = new Student(44, "e");

            Map<String, Student> stu1 = new HashMap<String, Student>();
            stu1.put("d", d);
            stu1.put("e", e);

            Student f = new Student(44, "f");
            Student g = new Student(44, "g");

            Map<String, Student> stu2 = new HashMap<String, Student>();
            stu2.put("f", f);
            stu2.put("g", g);


            Student h = new Student(44, "h");
            Student i = new Student(44, "i");
            Map<String, Student> stu3 = new HashMap<String, Student>();
            stu3.put("h", h);
            stu3.put("i", i);

            User aa = new User(99, "aa", stu1);
            User bb = new User(99, "bb", stu2);
            User cc = new User(99, "cc", stu3);
            users.put("aa", aa);
            users.put("bb", bb);
            users.put("cc", cc);
            basicObj.setUserMap(users);
            list1.add(basicObj);
        }
        String[] mainKeys = {"mapProp"};
        cl.setMainKeys(mainKeys);
        cl.save(list1);
        DBCursor cursor = cl.query();
        int count = 0;
        BSONObject bsonObj = null;
        HaveMapPropClass asBasicObj = null;
        while (cursor != null && cursor.hasNext()) {
            count++;
            bsonObj = cursor.getNext();
            SDBTestHelper.println("******************************");
            SDBTestHelper.println("obj=" + bsonObj.toString());
            asBasicObj = bsonObj.as(HaveMapPropClass.class);
            Map<String, User> users1 = asBasicObj.getUserMap();

            SDBTestHelper.println("list1.get(1).getUserMap():\t"
                + list1.get(1).getUserMap().toString());

            SDBTestHelper.println("");

            TreeMap<String, User> tmpTreeMap = new TreeMap<String, User>();
            tmpTreeMap.putAll(list1.get(1).getUserMap());
            SDBTestHelper.println("tmpTreeMap:\t" + tmpTreeMap.toString());

            SDBTestHelper.println("users1:\t" + users1.toString());
            SDBTestHelper.println("******************************");

            assertTrue(tmpTreeMap.toString().equals(users1.toString()));
        }
        assertEquals(max - min, count);
    }

    @Test
    public void testMapTypeWithNotExistMainKeys() throws Exception {
        List<HaveMapPropClass> list = new ArrayList<HaveMapPropClass>();
        final int max = 50;
        final int min = 20;
        for (int num = min; num < max; num++) {
            HaveMapPropClass basicObj = new HaveMapPropClass();
            Map<String, String> pros = new HashMap<String, String>();
            pros.put("company", "sequoiadb");
            pros.put("hr", "xiaoying");
            pros.put("address", "panyu");
            basicObj.setMapProp(pros);
            Map<String, User> users = new HashMap<String, User>();

            Student d = new Student(44, "d");
            Student e = new Student(44, "e");

            Map<String, Student> stu1 = new HashMap<String, Student>();
            stu1.put("d", d);
            stu1.put("e", e);

            Student f = new Student(44, "f");
            Student g = new Student(44, "g");

            Map<String, Student> stu2 = new HashMap<String, Student>();
            stu2.put("f", f);
            stu2.put("g", g);


            Student h = new Student(44, "h");
            Student i = new Student(44, "i");
            Map<String, Student> stu3 = new HashMap<String, Student>();
            stu3.put("h", h);
            stu3.put("i", i);


            User a = new User(33, "a", stu1);
            User b = new User(33, "b", stu2);
            User c = new User(33, "c", stu3);
            users.put("a", a);
            users.put("b", b);
            users.put("c", c);
            basicObj.setUserMap(users);
            list.add(basicObj);
        }
        cl.save(list);
        List<HaveMapPropClass> list1 = new ArrayList<HaveMapPropClass>();
        for (int num = min; num < max; num++) {
            HaveMapPropClass basicObj = new HaveMapPropClass();
            Map<String, String> pros = new HashMap<String, String>();
            pros.put("company", "sequoiadb");
            pros.put("hr", "xiaoying");
            pros.put("address", "panyu");
            basicObj.setMapProp(pros);
            Map<String, User> users = new HashMap<String, User>();

            Student d = new Student(44, "d");
            Student e = new Student(44, "e");

            Map<String, Student> stu1 = new HashMap<String, Student>();
            stu1.put("d", d);
            stu1.put("e", e);

            Student f = new Student(44, "f");
            Student g = new Student(44, "g");

            Map<String, Student> stu2 = new HashMap<String, Student>();
            stu2.put("f", f);
            stu2.put("g", g);


            Student h = new Student(44, "h");
            Student i = new Student(44, "i");
            Map<String, Student> stu3 = new HashMap<String, Student>();
            stu3.put("h", h);
            stu3.put("i", i);

            User aa = new User(99, "aa", stu1);
            User bb = new User(99, "bb", stu2);
            User cc = new User(99, "cc", stu3);
            users.put("aa", aa);
            users.put("bb", bb);
            users.put("cc", cc);
            basicObj.setUserMap(users);
            list1.add(basicObj);
        }
        String[] mainKeys = {"notExistMainKey1", "notExistMainKey2"};
        cl.setMainKeys(mainKeys);
        cl.save(list1);
        DBCursor cursor = cl.query();
        int count = 0;
        BSONObject bsonObj = null;
        HaveMapPropClass asBasicObj = null;
        while (cursor != null && cursor.hasNext()) {
            count++;
            bsonObj = cursor.getNext();
        }
        assertEquals((max - min) * 2, count);
    }

}
