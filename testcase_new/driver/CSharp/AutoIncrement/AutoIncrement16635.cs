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
     * TestCase : seqDB-16635
     * test interface:   CreateAutoIncrement ( List< BsonDocument > optionsList )
     *                   DropAutoIncrement ( List< string > fieldNames )
     * author:  chensiqin
     * date:    2018/11/20
     * version: 1.0
    */

    [TestClass]
    public class AutoIncrement16635
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private string clName = "cl16635";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);
        }

        [TestMethod]
        public void Test16635()
        {
            if (Common.IsStandalone(sdb))
            {
                return;
            }
            TestInvalidArg();

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
            BsonDocument options = new BsonDocument();
            List<BsonDocument> optionList = new List<BsonDocument>();
            optionList.Add(options);
            //empty
            try
            {
                cl.CreateAutoIncrement(optionList);
                Assert.Fail("expected throw BaseException");
            }
            catch (BaseException e)
            {
                Assert.AreEqual(-6, e.ErrorCode);
            }
            //invalid arg list
            try
            {
                optionList = new List<BsonDocument>();

                options = new BsonDocument();
                options.Add("Field", "num");
                optionList.Add(options);

                options = new BsonDocument();
                options.Add("test16635_2", true);
                optionList.Add(options);

                cl.CreateAutoIncrement(optionList);
                Assert.Fail("expected throw BaseException");
            }
            catch (BaseException e)
            {
                Assert.AreEqual(-6, e.ErrorCode);
            }

            try
            {
                optionList = new List<BsonDocument>();

                options = new BsonDocument();
                options.Add("test16635_1", "num");
                optionList.Add(options);

                options = new BsonDocument();
                options.Add("test16635_2", true);
                optionList.Add(options);

                cl.CreateAutoIncrement(optionList);
                Assert.Fail("expected throw BaseException");
            }
            catch (BaseException e)
            {
                Assert.AreEqual(-6, e.ErrorCode);
            }
            cs.DropCollection(clName);
        }


        public void TestNoDefaultOption()
        {
            BsonDocument options = new BsonDocument();
            List<BsonDocument> optionList = new List<BsonDocument>();
            DBCollection cl = cs.CreateCollection(clName);
            options = new BsonDocument();
            options.Add("Field", "num16635");
            options.Add("StartValue", 1);
            options.Add("MinValue", 1);
            options.Add("MaxValue", 4000);
            options.Add("Increment", 2);
            options.Add("CacheSize", 2000);
            options.Add("AcquireSize", 2000);
            options.Add("Cycled", true);
            options.Add("Generated", "strict");
            optionList.Add(options);

            options = new BsonDocument();
            options.Add("Field", "age16635");
            options.Add("StartValue", 1);
            options.Add("MinValue", 1);
            options.Add("MaxValue", 4000);
            options.Add("Increment", 2);
            options.Add("CacheSize", 2000);
            options.Add("AcquireSize", 2000);
            options.Add("Cycled", true);
            options.Add("Generated", "strict");
            optionList.Add(options);
            cl.CreateAutoIncrement(optionList);

            List<BsonDocument> autoIncrementInfos = GetAutoIncrement(SdbTestBase.csName + "." + clName);
            //Assert.AreEqual("num16635", );
            List<string> autoIncrementFields = new List<string>();
            autoIncrementFields.Add(autoIncrementInfos[0].GetElement("Field").Value.ToString());
            autoIncrementFields.Add(autoIncrementInfos[1].GetElement("Field").Value.ToString());
            Assert.IsTrue(autoIncrementFields.Contains("num16635"));
            Assert.IsTrue(autoIncrementFields.Contains("age16635"));
            for (int i = 0; i < 2; i++)
            {
                Assert.AreEqual("strict", autoIncrementInfos[i].GetElement("Generated").Value);

                BsonDocument actOption = GetSnapshot15(autoIncrementInfos[i].GetElement("SequenceName").Value.ToString());
                Console.WriteLine(actOption.ToString());
                Assert.AreEqual(options.GetElement("StartValue").Value.ToString(), actOption.GetElement("StartValue").Value.ToString());
                Assert.AreEqual(options.GetElement("MinValue").Value.ToString(), actOption.GetElement("MinValue").Value.ToString());
                Assert.AreEqual(options.GetElement("MaxValue").Value.ToString(), actOption.GetElement("MaxValue").Value.ToString());
                Assert.AreEqual(options.GetElement("Increment").Value.ToString(), actOption.GetElement("Increment").Value.ToString());
                Assert.AreEqual(options.GetElement("CacheSize").Value.ToString(), actOption.GetElement("CacheSize").Value.ToString());
                Assert.AreEqual(options.GetElement("AcquireSize").Value.ToString(), actOption.GetElement("AcquireSize").Value.ToString());
                Assert.AreEqual(true, actOption.GetElement("Cycled").Value);
            }


            BsonDocument record = new BsonDocument();
            record.Add("_id", 1);
            cl.Insert(record);
            record = new BsonDocument();
            record.Add("_id", 2);
            cl.Insert(record);
            BsonDocument matcher = new BsonDocument();
            matcher.Add("_id", 2);
            matcher.Add("num16635", 3);
            matcher.Add("age16635", 3);
            Assert.AreEqual(1, cl.GetCount(matcher));
            cs.DropCollection(clName);
        }

        public void TestDropAutoIncrement()
        {
            List<BsonDocument> optionList = new List<BsonDocument>();
            DBCollection cl = cs.CreateCollection(clName);
            BsonDocument options = new BsonDocument();
            options.Add("Field", "num16635");
            optionList.Add(options);

            options = new BsonDocument();
            options.Add("Field", "age16635");
            optionList.Add(options);
            cl.CreateAutoIncrement(optionList);

            List<string> fieldNames = new List<string>();
            fieldNames.Add("Test16635");
            fieldNames.Add("Test16635_2");
            //不存在的自增字段
            try
            {
                cl.DropAutoIncrement(fieldNames);
                Assert.Fail("expected throw BaseException");
            }
            catch (BaseException e)
            {
                Assert.AreEqual(-333, e.ErrorCode);
            }
            //空
            try
            {
                cl.DropAutoIncrement(new List<string>());
                Assert.Fail("expected throw BaseException");
            }
            catch (BaseException e)
            {
                Assert.AreEqual(-6, e.ErrorCode);
            }

            //已存在的自增字段
            fieldNames = new List<string>();
            fieldNames.Add("num16635");
            fieldNames.Add("age16635");
            try
            {
                cl.DropAutoIncrement(fieldNames);
            }
            catch (BaseException e)
            {
                Assert.Fail("DropAutoIncrement failed : " + e.ErrorCode);
            }
            cs.DropCollection(clName);
        }

        public List<BsonDocument> GetAutoIncrement(string clFullName)
        {
            BsonDocument matcher = new BsonDocument();
            matcher.Add("Name", clFullName);
            BsonDocument selector = new BsonDocument();
            selector.Add("AutoIncrement", 1);
            DBCursor cur = sdb.GetSnapshot(SDBConst.SDB_SNAP_CATALOG, matcher, selector, null);
            List<BsonDocument> doc = new List<BsonDocument>();
            while (cur.Next() != null)
            {
                BsonElement element = cur.Current().GetElement("AutoIncrement");
                BsonArray arr = element.Value.AsBsonArray;
                doc.Add((BsonDocument)arr[0]);
                doc.Add((BsonDocument)arr[1]);
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
