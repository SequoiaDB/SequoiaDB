/**
 *
 */
package com.sequoiadb.test.cl;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.test.common.Constants;
import com.sequoiadb.testdata.*;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;
import org.junit.*;

import java.util.*;

import static org.junit.Assert.*;


/**
 * @author qiushanggao
 */
public class testCollectionSaveAndAs {
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
     * @throws java.lang.Exception
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
     * @throws java.lang.Exception
     */
    @After
    public void tearDown() throws Exception {
        sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
    }

    @Test
    public void testBasiceType() throws Exception {

        try {
            BasicClass basicObj = new BasicClass();
            BSONObject bsonObj = BasicBSONObject.typeToBson(basicObj);

            BasicClass asBasicObj = bsonObj.as(BasicClass.class);

            assertEquals(asBasicObj, basicObj);

            cl.save(basicObj);
            DBCursor cursor = cl.query();
            while (cursor != null && cursor.hasNext()) {
                bsonObj = cursor.getNext();

                asBasicObj = bsonObj.as(BasicClass.class);

                assertEquals(asBasicObj, basicObj);
            }

        } catch (Exception e) {
            e.printStackTrace();
            fail();
        }
    }

    @Test
    public void testMapType() throws Exception {

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


        BSONObject bsonObj = BasicBSONObject.typeToBson(basicObj);

        HaveMapPropClass asBasicObj = bsonObj.as(HaveMapPropClass.class);

        assertEquals(asBasicObj, basicObj);

        cl.save(basicObj);
        DBCursor cursor = cl.query();
        while (cursor != null && cursor.hasNext()) {
            bsonObj = cursor.getNext();

            asBasicObj = bsonObj.as(HaveMapPropClass.class);

            assertEquals(asBasicObj, basicObj);
        }
    }

    @Test
    public void testMapTypeWithMainKeys() throws Exception {

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


        BSONObject bsonObj = BasicBSONObject.typeToBson(basicObj);

        HaveMapPropClass asBasicObj = bsonObj.as(HaveMapPropClass.class);

        assertEquals(asBasicObj, basicObj);
        cl.save(basicObj);
        Map<String, User> users1 = new HashMap<String, User>();
        User aa = new User(99, "aa", stu1);
        User bb = new User(99, "bb", stu2);
        User cc = new User(99, "cc", stu3);
        users1.put("aa", aa);
        users1.put("bb", bb);
        users1.put("cc", cc);
        basicObj.setUserMap(users1);
        String[] mainKeys = {"mapProp"};
        cl.setMainKeys(mainKeys);
        cl.save(basicObj);
        DBCursor cursor = cl.query();
        int count = 0;
        while (cursor != null && cursor.hasNext()) {
            count++;
            bsonObj = cursor.getNext();
        }
        asBasicObj = bsonObj.as(HaveMapPropClass.class);
        Map<String, User> users2 = asBasicObj.getUserMap();
        SDBTestHelper.println("users1:\t" + users1.toString());
        SDBTestHelper.println("users2:\t" + users2.toString());
        SDBTestHelper.println("count:\t" + count);
        TreeMap<String, User> tmpTreeMap = new TreeMap<String, User>();
        tmpTreeMap.putAll(users1);
        assertTrue(users2.toString().equals(tmpTreeMap.toString()));
        assertEquals(1, count);
    }

    @Test
    public void testNestedType() throws Exception {

        try {
            NestedClassA nestedObj = new NestedClassA();
            BSONObject bsonObj = BasicBSONObject.typeToBson(nestedObj);

            NestedClassA asnestedObj = bsonObj.as(NestedClassA.class);

            assertEquals(asnestedObj, nestedObj);

            cl.save(nestedObj);
            DBCursor cursor = cl.query();
            while (cursor != null && cursor.hasNext()) {
                bsonObj = cursor.getNext();

                asnestedObj = bsonObj.as(NestedClassA.class);

                assertEquals(asnestedObj, nestedObj);
            }

        } catch (Exception e) {
            fail();
        }
    }

    @Test
    public void testNestedTypeWithMainKeys() throws Exception {

        try {
            NestedClassA nestedObj = new NestedClassA();
            cl.save(nestedObj);
            String[] MainKeys = {"basicObjA", "basicObjC"};
            BasicClass bc = nestedObj.getBasicObjB();
            bc.setName("sequoiadb");
            cl.setMainKeys(MainKeys);
            cl.save(nestedObj);
            DBCursor cursor = cl.query();
            NestedClassA asnestedObj = null;
            BSONObject bsonObj = null;
            while (cursor != null && cursor.hasNext()) {
                bsonObj = cursor.getNext();
                asnestedObj = bsonObj.as(NestedClassA.class);
                String str = asnestedObj.getBasicObjB().getName();
                assertTrue(str.equals("sequoiadb"));
            }

        } catch (Exception e) {
            fail();
        }
    }

    @Test
    public void testListType() throws Exception {

        try {
            ListClassA listObj = new ListClassA();
            BSONObject bsonObj = BasicBSONObject.typeToBson(listObj);

            ListClassA aslistObj = bsonObj.as(ListClassA.class);

            assertEquals(aslistObj, listObj);

            cl.save(listObj);
            DBCursor cursor = cl.query();
            while (cursor != null && cursor.hasNext()) {
                bsonObj = cursor.getNext();

                aslistObj = bsonObj.as(ListClassA.class);

                assertEquals(aslistObj, listObj);
            }

        } catch (Exception e) {
            e.printStackTrace();
            fail();
        }
    }

    @Test
    public void testListTypeWithMainKeys() throws Exception {

        try {
            ListClassA listObj = new ListClassA();
            cl.save(listObj);
            List<String> list = new LinkedList<String>();
            list.add("sequoiadb");
            listObj.setListEmpty(list);
            String[] mainKeys = {"listHaveEles"};
            cl.setMainKeys(mainKeys);
            cl.save(listObj);
            DBCursor cursor = cl.query();
            BSONObject bsonObj = null;
            ListClassA aslistObj = null;
            while (cursor != null && cursor.hasNext()) {
                bsonObj = cursor.getNext();
                aslistObj = bsonObj.as(ListClassA.class);
                Iterator<String> it = aslistObj.getListEmpty().iterator();
                String str = null;
                while (it != null && it.hasNext()) {
                    str = it.next();
                }
                assertTrue(str.equals("sequoiadb"));
            }

        } catch (Exception e) {
            e.printStackTrace();
            fail();
        }
    }

    @Test
    public void testListNestedClassA() throws Exception {

        try {
            ListNestedClassA listNestedClassObj = new ListNestedClassA();
            BSONObject bsonObj = BasicBSONObject.typeToBson(listNestedClassObj);

            ListNestedClassA aslistNestedClassObj = bsonObj.as(ListNestedClassA.class);

            assertEquals(listNestedClassObj, aslistNestedClassObj);


            cl.save(listNestedClassObj);
            DBCursor cursor = cl.query();
            while (cursor != null && cursor.hasNext()) {
                BSONObject asbsonObj = cursor.getNext();

                aslistNestedClassObj = asbsonObj.as(ListNestedClassA.class);

                assertEquals(listNestedClassObj, aslistNestedClassObj);

                asbsonObj.removeField("_id");
            }

        } catch (Exception e) {
            e.printStackTrace();
            fail();
        }
    }

    @Test
    public void testListNestedClassAWithMainKeys() throws Exception {

        try {
            ListNestedClassA listNestedClassObj = new ListNestedClassA();

            cl.save(listNestedClassObj);
            BasicClass bsonObj = new BasicClass();
            bsonObj.setName("yaoming");
            List<BasicClass> value = new LinkedList<BasicClass>();
            value.add(bsonObj);
            listNestedClassObj.setListEmpty(value);
            String[] mainKeys = {"listHaveEles"};
            cl.setMainKeys(mainKeys);
            cl.save(listNestedClassObj);
            DBCursor cursor = cl.query();
            while (cursor != null && cursor.hasNext()) {
                BSONObject asbsonObj = cursor.getNext();
                ListNestedClassA aslistNestedClassObj = asbsonObj.as(ListNestedClassA.class);
                Iterator<BasicClass> it = aslistNestedClassObj.getListEmpty().iterator();
                BasicClass temp = null;
                while (it != null && it.hasNext()) {
                    temp = it.next();
                }
                assertTrue(temp.getName().equals("yaoming"));
            }

        } catch (Exception e) {
            e.printStackTrace();
            fail();
        }
    }

    @Test
    public void testSaveTopLevelMap() throws Exception {

        try {
            for (int i = 0; i < 1; i++) {
                BSONObject record = new BasicBSONObject();
                BSONObject addr = new BasicBSONObject();
                ArrayList phone = new ArrayList();
                BSONObject street = new BasicBSONObject();
                record.put("name", "sam" + i);
                record.put("age", 20 + i);
                record.put("addr", addr);
                record.put("phone", phone);

                addr.put("Nation", "China");
                addr.put("Province", "GuangDong");
                addr.put("City", "Guangzhou");
                addr.put("Street", street);

                phone.add("13427509700");
                phone.add("13432366221");
                phone.add("18090054789");

                street.put("Home", "zhongshan lu");
                street.put("WorkPlace", "beijing lu");
                cl.insert(record);
            }

            BSONObject matchor = new BasicBSONObject();
            matchor.put("age", 20);
            DBCursor cursor = cl.query(matchor, null, null, null);
            while (cursor != null && cursor.hasNext()) {
                BSONObject temp = cursor.getNext();
                System.out.println("temp is: " + temp.toString());

                BSONObject address = new BasicBSONObject();
                BSONObject street2 = new BasicBSONObject();
                address.put("Nation", "China");
                address.put("Province", "GuangDong");
                address.put("City", "Guangzhou");
                address.put("Street", street2);

                street2.put("Home", "xiao lu");
                street2.put("WorkPlace", "da lu");
                street2.put("NULL", null);


                Map map = new HashMap();
                Object id = temp.get("_id");
                String str = id.toString();
                ObjectId idd = new ObjectId(str);
                map.put("_id", idd);
                map.put("addr", address);
                cl.save(map);
            }
            DBCursor result = cl.query(matchor, null, null, null);
            while (result != null && result.hasNext()) {
                BSONObject temp = result.getNext();
            }
        } catch (Exception e) {
            e.printStackTrace();
            fail();
        }

    }

    @Test
    public void save_with_mainKeys() {

        try {
            BasicClass basicObj = new BasicClass();
            cl.save(basicObj);
            DBCursor cursor = cl.query();
            BSONObject bsonObj = new BasicBSONObject();
            int count = 0;
            while (cursor != null && cursor.hasNext()) {
                count++;
                bsonObj = cursor.getNext();
            }
            BasicClass newBasicObj = bsonObj.as(BasicClass.class);
            assertEquals(basicObj, newBasicObj);
            assertEquals(1, count);


            basicObj.setIdLong(9999L);
            basicObj.setScode(10.009);
            String[] keys = {"age", "name", "id"};
            cl.setMainKeys(keys);
            cl.save(basicObj);
            cursor = cl.query();
            BSONObject bsonObj1 = new BasicBSONObject();
            count = 0;
            while (cursor != null && cursor.hasNext()) {
                count++;
                bsonObj1 = cursor.getNext();
            }
            int age = (Integer) bsonObj1.get("age");
            Long id = (Long) bsonObj1.get("id");
            String name = (String) bsonObj1.get("name");
            assertEquals(basicObj.getAge(), age);
            assertTrue(basicObj.getId() == id);
            assertEquals(basicObj.getName(), name);
            assertEquals(1, count);

            basicObj.setAge(50);
            basicObj.setId(50);
            basicObj.setName("shanks");
            String[] keys1 = {"age", "name", "notExistField",};
            cl.setMainKeys(keys1);
            cl.save(basicObj);
            cursor = cl.query();
            BSONObject bsonObj2 = new BasicBSONObject();
            count = 0;
            while (cursor != null && cursor.hasNext()) {
                count++;
                bsonObj2 = cursor.getNext();
            }
            assertEquals(2, count);


        } catch (BaseException e) {
            e.printStackTrace();
            fail();
        } catch (Exception e1) {
            e1.printStackTrace();
            fail();
        }
    }

    @Test
    public void save_object_with_id() {

        try {
            ClassWithID basicObj = new ClassWithID();
            cl.save(basicObj);
            DBCursor cursor = cl.query();
            BSONObject bsonObj = new BasicBSONObject();
            int count = 0;
            while (cursor != null && cursor.hasNext()) {
                count++;
                bsonObj = cursor.getNext();
            }
            ClassWithID newBasicObj = bsonObj.as(ClassWithID.class);
            assertEquals(basicObj, newBasicObj);
            assertEquals(1, count);


            ClassWithID basicObj1 = new ClassWithID();
            cl.save(basicObj1);
            ObjectId _id = basicObj1.get_id();
            basicObj.set_id(_id);
            basicObj.setIdLong(9999L);
            int flag = 0;
            try {
                cl.save(basicObj);
            } catch (BaseException e) {
                System.out.println(e.getMessage());
                fail("In save_object_with_id(). The current case will never fail, but, it did.");
            }

            ClassWithID basicObj2 = new ClassWithID();
            cl.save(basicObj2);
            ObjectId _id2 = basicObj2.get_id();
            basicObj.set_id(_id2);
            basicObj.setAge(99);
            String[] keys = {"name", "id", "noExistField"};
            cl.setMainKeys(keys);
            flag = 0;
            try {
                cl.save(basicObj);
            } catch (BaseException e) {
                System.out.println(e.getMessage());
                assertTrue(e.getErrorType().equals("SDB_IXM_DUP_KEY"));
                flag = 1;
            } finally {
                if (flag == 0)
                    fail();
            }

        } catch (BaseException e) {
            e.printStackTrace();
            fail();
        } catch (Exception e1) {
            e1.printStackTrace();
            fail();
        }
    }


}
