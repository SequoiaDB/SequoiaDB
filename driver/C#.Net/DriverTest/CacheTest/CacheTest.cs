using SequoiaDB;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using System;
using System.Collections.Generic;
using SequoiaDB.Bson;
using System.Text;
using System.Collections;
using System.Reflection;
using System.Threading;

namespace DriverTest.CacheTest
{
    [TestClass()]
    public class CacheTest
    {
        private TestContext testContextInstance;
        private static Config config = null;
        Sequoiadb sdb = null;
        CollectionSpace cs = null;
        DBCollection coll = null;
        static string csName = "testfoo";
        static string cName = "testbar";
        string fullName = csName + "." + cName;
        private static String csName1 = "cs_cache_csharp1";
        private static String csName2 = "cs_cache_csharp2";
        private static String csName3 = "cs_cache_csharp3";
        private static String clName1_1 = "cl_cache_csharp1_1";
        private static String clName1_2 = "cl_cache_csharp1_2";
        private static String clName1_3 = "cl_cache_csharp1_3";
        private static String clName2_1 = "cl_cache_csharp2_1";
        private static String clName2_2 = "cl_cache_csharp2_2";
        private static String clName2_3 = "cl_cache_csharp2_3";
        private static String clName3_1 = "cl_cache_csharp3_1";
        private static String clName3_2 = "cl_cache_csharp3_2";
        private static String clName3_3 = "cl_cache_csharp3_3";
        private static String[] csArr = new String[3];
        private static String[] clArr1 = new String[3];
        private static String[] clArr2 = new String[3];
        private static String[] clArr3 = new String[3];

        public TestContext TestContext
        {
            get
            {
                return testContextInstance;
            }
            set
            {
                testContextInstance = value;
            }
        }

        #region 附加测试特性
        [ClassInitialize()]
        public static void SequoiadbInitialize(TestContext testContext)
        {
            if ( config == null )
                config = new Config();
        }
        [TestInitialize()]
        public void MyTestInitialize()
        {
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
            sdb = new Sequoiadb(config.conf.Coord.Address);
            sdb.Connect(config.conf.UserName, config.conf.Password);
            BsonDocument options = new BsonDocument();
            options.Add("ReplSize", 0);
            if (sdb.IsCollectionSpaceExist(csName))
                sdb.DropCollectionSpace(csName);
            cs = sdb.CreateCollectionSpace(csName);
            coll = cs.CreateCollection(cName, options);
        }

        [TestCleanup()]
        public void MyTestCleanup()
        {
            sdb.DropCollectionSpace(csName);
            sdb.Disconnect();
        }
        #endregion


        /// case1:测试initClient接口及ClientOptions是否生效
        [TestMethod()]
        public void InitClientTest()
        {
            // test1: test default value
            bool boolValue = true;
            long longValue = 300 * 1000;
            Sequoiadb db = new Sequoiadb(config.conf.Coord.Address);
            db.Connect(config.conf.UserName, config.conf.Password);
            // check
            Type t = typeof(Sequoiadb);
            BindingFlags flag = BindingFlags.Static | BindingFlags.NonPublic;
            FieldInfo f_enableCache = t.GetField("enableCache", flag);
            FieldInfo f_cacheInterval = t.GetField("cacheInterval", flag);
            Assert.AreEqual(boolValue, f_enableCache.GetValue(db));
            Assert.AreEqual(longValue, f_cacheInterval.GetValue(db));

            // test2: test user defined value
            boolValue = false;
            longValue = 100;
            ClientOptions opt = new ClientOptions();
            opt.CacheInterval = longValue;
            opt.EnableCache = boolValue;
            Sequoiadb.InitClient(opt);
            // check
            f_enableCache = t.GetField("enableCache", flag);
            f_cacheInterval = t.GetField("cacheInterval", flag);
            Assert.AreEqual(boolValue, f_enableCache.GetValue(db));
            Assert.AreEqual(longValue, f_cacheInterval.GetValue(db));

            // test3: test user definded value
            boolValue = true;
            longValue = -1;
            opt.CacheInterval = longValue;
            opt.EnableCache = boolValue;
            Sequoiadb.InitClient(opt);
            // check
            f_enableCache = t.GetField("enableCache", flag);
            f_cacheInterval = t.GetField("cacheInterval", flag);
            Assert.AreEqual(boolValue, f_enableCache.GetValue(db));
            Assert.AreEqual(300000L, f_cacheInterval.GetValue(db));

            // test4: opt to be null
            boolValue = true;
            longValue = 300000;
            opt = null;
            Sequoiadb.InitClient(opt);
            // check
            f_enableCache = t.GetField("enableCache", flag);
            f_cacheInterval = t.GetField("cacheInterval", flag);
            Assert.AreEqual(boolValue, f_enableCache.GetValue(db));
            Assert.AreEqual(300000L, f_cacheInterval.GetValue(db));

            db.Disconnect();
        }

        /// case2:测试ClientOptions设置的时间是否生效
        // 手工测试，需要在getCollection中添加(new Exception("xxxx"));
        // 来打印调用堆栈：
        /*
        public DBCollection GetCollection(string collectionName)
        {
            // get cl from cache
            if (sdb.FetchCache(collectionName))
            {
                //TODO:
                System.Exception e = new System.Exception("i am in fetch cache");
                System.Console.Out.WriteLine(e);
                return new DBCollection(this, collectionName);
            }
            // get cl from database
            if (IsCollectionExist(collectionName))
            {
                //TODO:
                System.Exception e = new System.Exception("i am in get cl from database");
                System.Console.Out.WriteLine(e);
                return new DBCollection(this, collectionName);
            }
            else
                throw new BaseException("SDB_DMS_NOTEXIST");
        }
         */
        [TestMethod()]
        [Ignore]
        public void CheckCacheWorksOrNotTest()
        {
            bool boolValue = true;
            long longValue = 6 * 1000;
            ClientOptions opt = new ClientOptions();
            opt.CacheInterval = longValue;
            opt.EnableCache = boolValue;
            Sequoiadb.InitClient(opt);
            Thread.Sleep(6000);
            CollectionSpace cs = sdb.GetCollecitonSpace(csName);
            Console.Out.WriteLine("begin, expect get cl from db");
            DBCollection cl = cs.GetCollection(cName);
            Thread.Sleep(3000);
            Console.Out.WriteLine("begin, expect get cl from cache");
            cl = cs.GetCollection(cName);
            Thread.Sleep(3500);
            Console.Out.WriteLine("begin, expect get cl from db");
            cl = cs.GetCollection(cName);
            Console.Out.WriteLine("haha");
        }

        /// case3:启用缓存时，测试与缓存相关的几个函数内部逻辑是否正确
        // upsertCache/removeCache/fetchCache
        [TestMethod()]
        public void CheckEnableCacheLogic()
        {
            // test1: test default value
            bool boolValue = true;
            long longValue = 300 * 1000;
            ClientOptions opt = new ClientOptions();
            opt.CacheInterval = longValue;
            opt.EnableCache = boolValue;
            Sequoiadb db = new Sequoiadb(config.conf.Coord.Address);
            db.Connect(config.conf.UserName, config.conf.Password);
            Sequoiadb.InitClient(opt);
            // check
            Type t = db.GetType();
            BindingFlags flag = BindingFlags.NonPublic | BindingFlags.Instance;
            FieldInfo f_nameCache = t.GetField("nameCache", flag);
            Dictionary<String, DateTime> dic = f_nameCache.GetValue(db) as Dictionary<String, DateTime>;
            Assert.AreEqual(0, dic.Count);

		    // create cs
		    CollectionSpace[] csObjArr = new CollectionSpace[3];
		    for(int i = 0; i < csArr.Length; i++) {
			    try{
				    db.DropCollectionSpace(csArr[i]);
			    }catch(BaseException e) {
			    }
		    }
		    for(int i = 0; i < csArr.Length; i++) {
			    csObjArr[i] = db.CreateCollectionSpace(csArr[i]);
		    }
		    // test1: 检测db对象中cs的缓存情况
		    // TODO:
		    Console.Out.WriteLine("point 1: after creating cs, nameCache.Count is: " + dic.Count);
		    Assert.AreEqual(csArr.Length, dic.Count);
		    for(int i = 0; i < csArr.Length; i++) {
                Assert.IsTrue(dic.ContainsKey(csArr[i]));
		    }
		    // drop one cs
		    db.DropCollectionSpace(csArr[0]);
		    // test2: 删除一个cs之后，检测db对象中cs的缓存情况
            Assert.AreEqual(csArr.Length - 1,dic.Count);
		    Assert.IsFalse(dic.ContainsKey(csArr[0]));
		    for(int i = 1; i < csArr.Length; i++) {
			    Assert.IsTrue(dic.ContainsKey(csArr[i]));
		    }
		    // create the drop cs
		    csObjArr[0] = db.CreateCollectionSpace(csArr[0]);
		    Assert.AreEqual(csArr.Length, dic.Count);
		    for(int i = 0; i < csArr.Length; i++) {
			    Assert.IsTrue(dic.ContainsKey(csArr[i]));
		    }
		    // create cl
            BsonDocument conf = new BsonDocument();
		    conf.Add("ReplSize", 0);
		    for(int i = 0; i < clArr1.Length; i++) {
			    //Console.Out.WriteLine("csObjArr[0] is: " + csObjArr[0].getName() + ", clArr1[x] is: " + clArr1[i]);
			    csObjArr[0].CreateCollection(clArr1[i], conf);
		    }
		    // test3: 检测db对象中cs,cl的缓存情况
		    Assert.AreEqual(csArr.Length + clArr1.Length, dic.Count);
		    for(int i = 0; i < csArr.Length; i++) {
			    Assert.IsTrue(dic.ContainsKey(csArr[i]));
		    }
		    for(int i = 0; i < clArr1.Length; i++) {
			    Assert.IsTrue(dic.ContainsKey(csArr[0] + "." + clArr1[i]));
		    }
            // drop one cl
		    csObjArr[0].DropCollection(clArr1[0]);
		    // test4: 检测删除cl之后，cs,cl的缓存情况
		    Assert.AreEqual(csArr.Length + clArr1.Length - 1, dic.Count);
		    for(int i = 0; i < csArr.Length; i++) {
			    Assert.IsTrue(dic.ContainsKey(csArr[i]));
		    }
		    Assert.IsFalse(dic.ContainsKey(csArr[0] + "." + clArr1[0]));
		    for(int i = 1; i < clArr1.Length; i++) {
			    Assert.IsTrue(dic.ContainsKey(csArr[0] + "." + clArr1[i]));
		    }
		    // drop one cs
		    db.DropCollectionSpace(csArr[0]);
		    // test5: 检测删除cs之后，cs,cl的缓存情况
		    Assert.AreEqual(csArr.Length - 1, dic.Count);
		    Assert.IsFalse(dic.ContainsKey(csArr[0]));
		    for(int i = 1; i < csArr.Length; i++) {
			    Assert.IsTrue(dic.ContainsKey(csArr[i]));
		    }
		    for(int i = 1; i < clArr1.Length; i++) {
			    Assert.IsFalse(dic.ContainsKey(csArr[0] + "." + clArr1[i]));
		    }
    //		for(int i = 0; i < clArr2.Length; i++) {
    //			Console.Out.WriteLine("csObjArr[1] is: " + csObjArr[1].getName() + ", clArr2[x] is: " + clArr2[i]);
    //			csObjArr[1].CreateCollection(clArr2[i], conf);
    //		}
    //		for(int i = 0; i < clArr3.Length; i++) {
    //			Console.Out.WriteLine("csObjArr[2] is: " + csObjArr[2].getName() + ", clArr3[x] is: " + clArr3[i]);
    //			csObjArr[2].CreateCollection(clArr3[i], conf);
    //		}
            db.Disconnect();
        }

        /// case4:关闭缓存时，测试与缓存相关的几个函数内部逻辑是否正确
        // upsertCache/removeCache/fetchCache
        [TestMethod()]
        public void CheckDisableCacheLogic()
        {
            // test1: test default value
            bool boolValue = false;
            long longValue = 300 * 1000;
            ClientOptions opt = new ClientOptions();
            opt.CacheInterval = longValue;
            opt.EnableCache = boolValue;
            Sequoiadb db = new Sequoiadb(config.conf.Coord.Address);
            db.Connect(config.conf.UserName, config.conf.Password);
            Sequoiadb.InitClient(opt);
            // check
            Type t = db.GetType();
            BindingFlags flag = BindingFlags.NonPublic | BindingFlags.Instance;
            FieldInfo f_nameCache = t.GetField("nameCache", flag);
            Dictionary<String, DateTime> dic = f_nameCache.GetValue(db) as Dictionary<String, DateTime>;
            Assert.AreEqual(0, dic.Count);

            // create cs
            CollectionSpace[] csObjArr = new CollectionSpace[3];
            for (int i = 0; i < csArr.Length; i++)
            {
                try
                {
                    db.DropCollectionSpace(csArr[i]);
                }
                catch (BaseException e)
                {
                }
            }
            for (int i = 0; i < csArr.Length; i++)
            {
                csObjArr[i] = db.CreateCollectionSpace(csArr[i]);
            }
            // test1: 检测db对象中cs的缓存情况
            Console.Out.WriteLine("point 1: after creating cs, nameCache.Count is: " + dic.Count);
            Assert.AreEqual(0, dic.Count);
            // drop one cs
            db.DropCollectionSpace(csArr[0]);
            // test2: 删除一个cs之后，检测db对象中cs的缓存情况
            Assert.AreEqual(0, dic.Count);
            // create the drop cs
            csObjArr[0] = db.CreateCollectionSpace(csArr[0]);
            Assert.AreEqual(0, dic.Count);
            // create cl
            BsonDocument conf = new BsonDocument();
            conf.Add("ReplSize", 0);
            for (int i = 0; i < clArr1.Length; i++)
            {
                //Console.Out.WriteLine("csObjArr[0] is: " + csObjArr[0].getName() + ", clArr1[x] is: " + clArr1[i]);
                csObjArr[0].CreateCollection(clArr1[i], conf);
            }
            // test3: 检测db对象中cs,cl的缓存情况
            Assert.AreEqual(0, dic.Count);
            // drop one cl
            csObjArr[0].DropCollection(clArr1[0]);
            // test4: 检测删除cl之后，cs,cl的缓存情况
            Assert.AreEqual(0, dic.Count);
            // drop one cs
            db.DropCollectionSpace(csArr[0]);
            // test5: 检测删除cs之后，cs,cl的缓存情况
            Assert.AreEqual(0, dic.Count);
            db.Disconnect();
        }

        /// case5: cache开启时，尝试异常场景
        [TestMethod()]
        public void CheckInvalidSituaction()
        {
            bool boolValue = true;
            long longValue = 300 * 1000;
            ClientOptions opt = new ClientOptions();
            opt.CacheInterval = longValue;
            opt.EnableCache = boolValue;
            Sequoiadb db = new Sequoiadb(config.conf.Coord.Address);
            db.Connect(config.conf.UserName, config.conf.Password);
            Sequoiadb.InitClient(opt);
            // check
            Type t = db.GetType();
            BindingFlags flag = BindingFlags.NonPublic | BindingFlags.Instance;
            FieldInfo f_nameCache = t.GetField("nameCache", flag);
            Dictionary<String, DateTime> dic = f_nameCache.GetValue(db) as Dictionary<String, DateTime>;
            Assert.AreEqual(0, dic.Count);
		
		    String csName = "foo_csharp.";
		    String clName = "bar_csharp.";
		    try {
			    db.DropCollectionSpace(csName);
		    } catch(BaseException e) {
		    }
		    try {
			    db.CreateCollectionSpace(csName);
			    Assert.Fail();
		    } catch(BaseException e) {
			    csName = "foo_csharp";
		    }
		    try {
			    db.DropCollectionSpace(csName);
		    } catch(BaseException e) {
		    }
		    // check
		    Assert.AreEqual(0, dic.Count);
		    CollectionSpace cs = db.CreateCollectionSpace(csName);
		    // check
		    Assert.AreEqual(1, dic.Count);
		    Assert.IsTrue(dic.ContainsKey(csName));
		    try {
			    cs.CreateCollection(clName);
			    Assert.Fail();
		    } catch(BaseException e) {
			    clName = "bar_csharp";
		    }
		    // check
		    Assert.AreEqual(1, dic.Count);
		    Assert.IsTrue(dic.ContainsKey(csName));
		    cs.CreateCollection(clName);
		    // check
		    Assert.AreEqual(2, dic.Count);
		    Assert.IsTrue(dic.ContainsKey(csName + "." + clName));
		    Assert.IsTrue(dic.ContainsKey(csName));
		
		    db.DropCollectionSpace(csName);
		    // check
		    Assert.AreEqual(0, dic.Count);
		
		    db.Disconnect();
        }
        
    }
}
