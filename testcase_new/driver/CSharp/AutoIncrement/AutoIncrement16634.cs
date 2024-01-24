using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB;
using CSharp.TestCommon;
using SequoiaDB.Bson;

namespace CSharp.AutoIncrement
{
    /**
     * TestCase : seqDB-16634
     * test interface:   CreateAutoIncrement ( BsonDocument options )
     *                   DropAutoIncrement ( String fieldName )
     * author:  chensiqin
     * date:    2018/11/19
     * version: 1.0
    */

    [TestClass]
    public class AutoIncrement16634
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private string clName = "cl16634";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);
        }

        [TestMethod]
        public void Test16634()
        {
            if (Common.IsStandalone(sdb))
            {
                return;
            }

            TestInvalidArg();

            //只传入自增字段名
            TestDefaultOption();

            //所有参数项均给非默认值
            TestNoDefaultOption();

            //删除自增字段
            TestDropAutoIncrement();

        }

        [TestCleanup()]
        public void TearDown()
        {
            if (cs.IsCollectionExist(clName))
            {
                cs.DropCollection(clName);
            }
            if (sdb != null)
            {
                sdb.Disconnect();
            }
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " end  : " + this.GetType().ToString());
        }

        public void TestInvalidArg()
        {
            DBCollection cl = cs.CreateCollection(clName);
            BsonDocument options;
            //empty
            try
            {
                cl.CreateAutoIncrement(new BsonDocument());
                Assert.Fail("expected throw BaseException");
            }
            catch (BaseException e)
            {
                Assert.AreEqual(-6, e.ErrorCode);
            }
            //invalid arg
            try
            {
                options = new BsonDocument();
                options.Add("test16634", true);
                cl.CreateAutoIncrement(options);
                Assert.Fail("expected throw BaseException");
            }
            catch (BaseException e)
            {
                Assert.AreEqual(-6, e.ErrorCode);
            }
            cs.DropCollection(clName);
        }

        public void TestDefaultOption()
        {
            DBCollection cl = cs.CreateCollection(clName);
            BsonDocument options = new BsonDocument();
            options.Add("Field", "num");
            cl.CreateAutoIncrement(options);
            BsonDocument actInfo = GetAutoIncrement(SdbTestBase.csName + "." + clName);
            Assert.AreEqual("num", actInfo.GetElement("Field").Value);
            Assert.AreEqual("default", actInfo.GetElement("Generated").Value);
            BsonDocument record = new BsonDocument();
            record.Add("_id", 1);
            cl.Insert(record);
            BsonDocument matcher = new BsonDocument();
            matcher.Add("_id", 1);
            matcher.Add("num", 1);
            Assert.AreEqual(1, cl.GetCount(matcher));
            cs.DropCollection(clName);
        }


        public void TestNoDefaultOption()
        {
            BsonDocument options = new BsonDocument();
            DBCollection cl = cs.CreateCollection(clName);
            options = new BsonDocument();
            options.Add("Field", "num");
            options.Add("StartValue", 1);
            options.Add("MinValue", 1);
            options.Add("MaxValue", 4000);
            options.Add("Increment", 2);
            options.Add("CacheSize", 2000);
            options.Add("AcquireSize", 2000);
            options.Add("Cycled", true);
            options.Add("Generated", "strict");
            cl.CreateAutoIncrement(options);
            BsonDocument autoIncrementInfo = GetAutoIncrement(SdbTestBase.csName + "." + clName);
            Assert.AreEqual("num", autoIncrementInfo.GetElement("Field").Value);
            Assert.AreEqual("strict", autoIncrementInfo.GetElement("Generated").Value);
            BsonDocument actOption = GetSnapshot15(autoIncrementInfo.GetElement("SequenceName").Value.ToString());
            Assert.AreEqual(options.GetElement("StartValue").Value.ToString(), actOption.GetElement("StartValue").Value.ToString());
            Assert.AreEqual(options.GetElement("MinValue").Value.ToString(), actOption.GetElement("MinValue").Value.ToString());
            Assert.AreEqual(options.GetElement("MaxValue").Value.ToString(), actOption.GetElement("MaxValue").Value.ToString());
            Assert.AreEqual(options.GetElement("Increment").Value.ToString(), actOption.GetElement("Increment").Value.ToString());
            Assert.AreEqual(options.GetElement("CacheSize").Value.ToString(), actOption.GetElement("CacheSize").Value.ToString());
            Assert.AreEqual(options.GetElement("AcquireSize").Value.ToString(), actOption.GetElement("AcquireSize").Value.ToString());
            Assert.AreEqual(true, actOption.GetElement("Cycled").Value);
            BsonDocument record = new BsonDocument();
            record.Add("_id", 1);
            cl.Insert(record);
            record = new BsonDocument();
            record.Add("_id", 2);
            cl.Insert(record);
            BsonDocument matcher = new BsonDocument();
            matcher.Add("_id", 2);
            matcher.Add("num", 3);
            Assert.AreEqual(1, cl.GetCount(matcher));
            cs.DropCollection(clName);
        }

        public void TestDropAutoIncrement()
        {
            DBCollection cl = cs.CreateCollection(clName);
            BsonDocument options = new BsonDocument();
            options.Add("Field", "num");
            cl.CreateAutoIncrement(options);
            //不存在的自增字段
            try
            {
                cl.DropAutoIncrement("Test16634");
                Assert.Fail("expected throw BaseException");
            }
            catch (BaseException e)
            {
                Assert.AreEqual(-333, e.ErrorCode);
            }
            //空
            try
            {
                cl.DropAutoIncrement("");
                Assert.Fail("expected throw BaseException");
            }
            catch (BaseException e)
            {
                Assert.AreEqual(-6, e.ErrorCode);
            }

            //已存在的自增字段
            try
            {
                cl.DropAutoIncrement("num");
            }
            catch (BaseException e)
            {
                Assert.Fail("DropAutoIncrement failed : " + e.ErrorCode);
            }
            cs.DropCollection(clName);
        }

        public BsonDocument GetAutoIncrement(string clFullName)
        {
            BsonDocument matcher = new BsonDocument();
            matcher.Add("Name", clFullName);
            BsonDocument selector = new BsonDocument();
            selector.Add("AutoIncrement", 1);
            DBCursor cur = sdb.GetSnapshot(SDBConst.SDB_SNAP_CATALOG, matcher, selector, null);
            BsonDocument doc = new BsonDocument();
            while (cur.Next() != null)
            {
                BsonElement element = cur.Current().GetElement("AutoIncrement");
                BsonArray arr = element.Value.AsBsonArray;
                doc = (BsonDocument)arr[0];
            }
            cur.Close();
            return doc;
        }

        public BsonDocument GetSnapshot15(string SequenceName)
        {
            BsonDocument doc = new BsonDocument();
            BsonDocument matcher = new BsonDocument();
            matcher.Add("Name", SequenceName);

            DBCursor cur = sdb.GetSnapshot(SDBConst.SDB_SNAP_SEQUENCES, matcher, null, null);
            while (cur.Next() != null)
            {
                doc = cur.Current();
            }
            cur.Close();
            return doc;
        }
    }
}
