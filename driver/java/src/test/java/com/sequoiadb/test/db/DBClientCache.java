package com.sequoiadb.test.db;


import com.sequoiadb.base.ClientOptions;
import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.test.common.Constants;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.junit.*;
import org.junit.runners.MethodSorters;

import java.lang.reflect.Field;
import java.util.Map;

import static org.junit.Assert.assertNotNull;

@FixMethodOrder(MethodSorters.NAME_ASCENDING)
public class DBClientCache {
    private static String csName1 = "cs_cache_java1";
    private static String csName2 = "cs_cache_java2";
    private static String csName3 = "cs_cache_java3";
    private static String clName1_1 = "cl_cache_java1_1";
    private static String clName1_2 = "cl_cache_java1_2";
    private static String clName1_3 = "cl_cache_java1_3";
    private static String clName2_1 = "cl_cache_java2_1";
    private static String clName2_2 = "cl_cache_java2_2";
    private static String clName2_3 = "cl_cache_java2_3";
    private static String clName3_1 = "cl_cache_java3_1";
    private static String clName3_2 = "cl_cache_java3_2";
    private static String clName3_3 = "cl_cache_java3_3";
    private static String[] csArr = new String[3];
    private static String[] clArr1 = new String[3];
    private static String[] clArr2 = new String[3];
    private static String[] clArr3 = new String[3];


    @BeforeClass
    public static void setUpBeforeClass() throws Exception {
        csArr[0] = csName1;
        csArr[1] = csName2;
        csArr[2] = csName3;
        clArr1[0] = clName1_1;
        clArr1[1] = clName1_2;
        clArr1[2] = clName1_3;
        clArr2[0] = clName2_1;
        clArr2[1] = clName2_2;
        clArr2[2] = clName2_3;
        clArr3[0] = clName3_1;
        clArr3[1] = clName3_2;
        clArr3[2] = clName3_3;
    }

    @AfterClass
    public static void tearDownAfterClass() throws Exception {
    }

    @Before
    public void setUp() throws Exception {
    }

    @After
    public void tearDown() throws Exception {
    }

    /// case1:测试initClient接口及ClientOptions是否生效
    @Test
    public void test_1_InitClientWithDefaultValue() {
        ClientOptions result1 = new ClientOptions();
        Assert.assertTrue( result1.getEnableCache() );
        Assert.assertEquals( result1.getCacheInterval(), 300 * 1000 );
        Assert.assertFalse( result1.getExactlyDate() );
    }

    @Test
    @Ignore
    public void test_3_CacheIntervalWorksOrNot() throws InterruptedException {
        System.out.println("************test 3************");
        boolean definedBoolValue = true;
        long definedLongValue = 6 * 1000;
        ClientOptions options = new ClientOptions();
        options.setCacheInterval(definedLongValue);
        options.setEnableCache(definedBoolValue);
        Sequoiadb.initClient(options);
        Sequoiadb db = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");
        try {
            CollectionSpace cs1 = null;
            CollectionSpace cs2 = null;
            cs1 = db.getCollectionSpace(csName1);
            Thread.sleep(definedLongValue - 3000);
            // test1:获取cs，此时应该访问缓存
            cs1 = db.getCollectionSpace(csName1);
            Thread.sleep(3500);
            // test2: 获取cs，此时应该访问数据库
            cs1 = db.getCollectionSpace(csName1);
        } finally {
            db.disconnect();
        }
    }

    /// case3:测试与缓存相关的几个函数内部逻辑是否正确
    // upsertCache/removeCache/fetchCache
    @Test
    public void test_4_CacheLogicWithEnableCache() throws SecurityException, NoSuchFieldException, IllegalArgumentException, IllegalAccessException {
        System.out.println("************test 4************");
        boolean defaultBoolValue = true;
        long defaultLongValue = 60 * 1000;
        ClientOptions options = new ClientOptions();
        options.setEnableCache(defaultBoolValue);
        options.setCacheInterval(defaultLongValue);
        Sequoiadb.initClient(options);
        Sequoiadb db1 = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");
        try {
            // 获取缓存的map
            Class<?> c = db1.getClass();
            Field f_nameCache = c.getDeclaredField("nameCache");
            boolean accessFlag = f_nameCache.isAccessible();
            f_nameCache.setAccessible(true);
            @SuppressWarnings("unchecked")
            Map<String, Long> map1 = (Map<String, Long>) (f_nameCache.get(db1));

            // create cs
            CollectionSpace[] csObjArr = new CollectionSpace[3];
            for (int i = 0; i < csArr.length; i++) {
                try {
                    db1.dropCollectionSpace(csArr[i]);
                } catch (BaseException e) {
                }
            }
            for (int i = 0; i < csArr.length; i++) {
                csObjArr[i] = db1.createCollectionSpace(csArr[i]);
            }
            // test1: 检测db对象中cs的缓存情况
            // TODO:
            System.out.println("point 1: after creating cs, nameCache.size() is: " + map1.size());
            Assert.assertEquals(csArr.length, map1.size());
            for (int i = 0; i < csArr.length; i++) {
                Assert.assertTrue(map1.containsKey(csArr[i]));
            }
            // drop one cs
            db1.dropCollectionSpace(csArr[0]);
            // test2:删除一个cs之后，检测db对象中cs的缓存情况
            Assert.assertEquals(csArr.length - 1, map1.size());
            Assert.assertFalse(map1.containsKey(csArr[0]));
            for (int i = 1; i < csArr.length; i++) {
                Assert.assertTrue(map1.containsKey(csArr[i]));
            }
            // create the drop cs
            csObjArr[0] = db1.createCollectionSpace(csArr[0]);
            Assert.assertEquals(csArr.length, map1.size());
            for (int i = 0; i < csArr.length; i++) {
                Assert.assertTrue(map1.containsKey(csArr[i]));
            }
            // create cl
            BSONObject conf = new BasicBSONObject();
            conf.put("ReplSize", 0);
            for (int i = 0; i < clArr1.length; i++) {
                //System.out.println("csObjArr[0] is: " + csObjArr[0].getName() + ", clArr1[x] is: " + clArr1[i]);
                csObjArr[0].createCollection(clArr1[i], conf);
            }
            // test3: 检测db对象中cs,cl的缓存情况
            Assert.assertEquals(csArr.length + clArr1.length, map1.size());
            for (int i = 0; i < csArr.length; i++) {
                Assert.assertTrue(map1.containsKey(csArr[i]));
            }
            for (int i = 0; i < clArr1.length; i++) {
                Assert.assertTrue(map1.containsKey(csArr[0] + "." + clArr1[i]));
            }
            // drop one cl
            csObjArr[0].dropCollection(clArr1[0]);
            // test4: 检测删除cl之后，cs,cl的缓存情况
            Assert.assertEquals(csArr.length + clArr1.length - 1, map1.size());
            for (int i = 0; i < csArr.length; i++) {
                Assert.assertTrue(map1.containsKey(csArr[i]));
            }
            Assert.assertFalse(map1.containsKey(csArr[0] + "." + clArr1[0]));
            for (int i = 1; i < clArr1.length; i++) {
                Assert.assertTrue(map1.containsKey(csArr[0] + "." + clArr1[i]));
            }
            // drop one cs
            db1.dropCollectionSpace(csArr[0]);
            // test5:检测删除cs之后，cs,cl的缓存情况
            Assert.assertEquals(csArr.length - 1, map1.size());
            Assert.assertFalse(map1.containsKey(csArr[0]));
            for (int i = 1; i < csArr.length; i++) {
                Assert.assertTrue(map1.containsKey(csArr[i]));
            }
            for (int i = 1; i < clArr1.length; i++) {
                Assert.assertFalse(map1.containsKey(csArr[0] + "." + clArr1[i]));
            }
            //		for(int i = 0; i < clArr2.length; i++) {
            //			System.out.println("csObjArr[1] is: " + csObjArr[1].getName() + ", clArr2[x] is: " + clArr2[i]);
            //			csObjArr[1].createCollection(clArr2[i], conf);
            //		}
            //		for(int i = 0; i < clArr3.length; i++) {
            //			System.out.println("csObjArr[2] is: " + csObjArr[2].getName() + ", clArr3[x] is: " + clArr3[i]);
            //			csObjArr[2].createCollection(clArr3[i], conf);
            //		}
            //		// TODO:
            //		System.out.println("f_nameCache is: " + ((Object)f_nameCache.get(db)).toString());

            f_nameCache.setAccessible(accessFlag);
        } finally {
            db1.disconnect();
        }
    }

    @Test
    public void test_5_CacheLogicWithDisableCache() throws SecurityException, NoSuchFieldException, IllegalArgumentException, IllegalAccessException {
        System.out.println("************test 5************");
        boolean defaultBoolValue = false;
        long defaultLongValue = 60 * 1000;
        ClientOptions options = new ClientOptions();
        options.setCacheInterval(defaultLongValue);
        options.setEnableCache(defaultBoolValue);
        Sequoiadb.initClient(options);
        Sequoiadb db = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");
        try {
            // 获取缓存的map
            Class<?> c = db.getClass();
            Field f_nameCache = c.getDeclaredField("nameCache");
            boolean accessFlag = f_nameCache.isAccessible();
            f_nameCache.setAccessible(true);
            @SuppressWarnings("unchecked")
            Map<String, Long> map = (Map<String, Long>) (f_nameCache.get(db));

            // create cs
            CollectionSpace[] csObjArr = new CollectionSpace[3];
            for (int i = 0; i < csArr.length; i++) {
                try {
                    db.dropCollectionSpace(csArr[i]);
                } catch (BaseException e) {
                }
            }
            for (int i = 0; i < csArr.length; i++) {
                csObjArr[i] = db.createCollectionSpace(csArr[i]);
            }
            // test1: 检测db对象中cs的缓存情况
            // TODO:
            System.out.println("point 2: after creating cs, nameCache.size() is: " + map.size());
            Assert.assertEquals(0, map.size());
            // drop one cs
            db.dropCollectionSpace(csArr[0]);
            // test2: 删除一个cs之后，检测db对象中cs的缓存情况
            Assert.assertEquals(0, map.size());
            // create the drop cs
            csObjArr[0] = db.createCollectionSpace(csArr[0]);
            Assert.assertEquals(0, map.size());
            // create cl
            BSONObject conf = new BasicBSONObject();
            conf.put("ReplSize", 0);
            for (int i = 0; i < clArr1.length; i++) {
                //System.out.println("csObjArr[0] is: " + csObjArr[0].getName() + ", clArr1[x] is: " + clArr1[i]);
                csObjArr[0].createCollection(clArr1[i], conf);
            }
            // test3: 检测db对象中cs,cl的缓存情况
            Assert.assertEquals(0, map.size());
            // drop one cl
            csObjArr[0].dropCollection(clArr1[0]);
            // test4: 检测删除cl之后，cs,cl的缓存情况
            Assert.assertEquals(0, map.size());
            // drop one cs
            db.dropCollectionSpace(csArr[0]);
            // test5: ���ɾ��cs֮��cs,cl�Ļ������
            Assert.assertEquals(0, map.size());

            f_nameCache.setAccessible(accessFlag);
        } finally {
            db.disconnect();
        }
    }

    /// case4: cache开启/关闭时，使用连接池跑业务

    /// case5: cache开启时，尝试异常场景
    // 需要手工
    @Test
    public void test_6_InvalidSituaction() throws SecurityException, NoSuchFieldException, IllegalArgumentException, IllegalAccessException {
        System.out.println("************test 6************");
        ClientOptions options = new ClientOptions();
        options.setEnableCache(true);
        Sequoiadb.initClient(options);
        Sequoiadb db = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");
        try {
            // 获取缓存的map
            Class<?> c = db.getClass();
            Field f_nameCache = c.getDeclaredField("nameCache");
            boolean accessFlag = f_nameCache.isAccessible();
            f_nameCache.setAccessible(true);
            @SuppressWarnings("unchecked")
            Map<String, Long> map = (Map<String, Long>) (f_nameCache.get(db));

            String csName = "foo_java.";
            String clName = "bar_java.";
            try {
                db.dropCollectionSpace(csName);
            } catch (BaseException e) {
            }
            try {
                db.createCollectionSpace(csName);
                Assert.fail();
            } catch (BaseException e) {
                csName = "foo_java";
            }
            try {
                db.dropCollectionSpace(csName);
            } catch (BaseException e) {
            }
            // check
            Assert.assertEquals(0, map.size());
            CollectionSpace cs = db.createCollectionSpace(csName);
            // check
            Assert.assertEquals(1, map.size());
            Assert.assertTrue(map.containsKey(csName));
            try {
                cs.createCollection(clName);
                Assert.fail();
            } catch (BaseException e) {
                clName = "bar_java";
            }
            // check
            Assert.assertEquals(1, map.size());
            Assert.assertTrue(map.containsKey(csName));
            cs.createCollection(clName);
            // check
            Assert.assertEquals(2, map.size());
            Assert.assertTrue(map.containsKey(csName + "." + clName));
            Assert.assertTrue(map.containsKey(csName));

            db.dropCollectionSpace(csName);
            // check
            Assert.assertEquals(0, map.size());

            f_nameCache.setAccessible(accessFlag);
        } finally {
            db.disconnect();
        }
    }


}
