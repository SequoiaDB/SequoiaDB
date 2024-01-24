using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB.Bson;
using SequoiaDB;
using CSharp.TestCommon;

namespace CSharp.Index
{
    /**
     * description: createIndex (string name，BsonDocument key，bool isUnique，bool isEnforced，int sortBufferSize)；
     *              dropIndex (string name)；
     *              GetIndexes（）；
     *              createIdIndex (BsonDocument options)；
     *              GetIndex（string name）
     *              1、分别验证索引操作各接口参数合法值和非法值； 
     *                合法值： a、输入合法参数 b、所有参数取默认值 c、非必填项为空值 
     *                非法值： a、必填项为null 
     *              2、检查操作结果  
     * testcase:    14548
     * author:      chensiqin
     * date:        2019/04/1
     */

    [TestClass]
    public class TestIndex14548
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private string clName = "cl14548";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
        }

        [TestMethod]
        public void Test14548()
        {
            string indexName = "index14548";
            cs = sdb.GetCollecitonSpace(SdbTestBase.csName);
            if (cs.IsCollectionExist(clName))
            {
                cs.DropCollection(clName);
            }
            cl = cs.CreateCollection(clName, new BsonDocument("AutoIndexId",false));
            //非法值
            try
            {
                cl.CreateIndex(null, null, true, true, 64);
                Assert.Fail("expect CreateIndex failed !");
            }
            catch (BaseException e)
            {
                Assert.AreEqual(-6, e.ErrorCode);
            }
            try
            {
                cl.DropIndex(null);
                Assert.Fail("expect CreateIndex failed !");
            }
            catch (BaseException e)
            {
                Assert.AreEqual(-6, e.ErrorCode);
            }
            try
            {
                cl.CreateIndex(indexName, new BsonDocument("a", null), new BsonDocument("NotNull", true));
                Assert.Fail("expect CreateIndex failed !");
            }
            catch (BaseException e)
            {
                Assert.AreEqual(-6, e.ErrorCode);
            }

            //合法值
            //createIndex (string name，BsonDocument key，bool isUnique，bool isEnforced，int sortBufferSize)
            cl.CreateIndex(indexName, new BsonDocument("a", 1), true, true, 0);
            DBCursor cur = cl.GetIndex(indexName);
            while (cur.Next() != null)
            {
                BsonDocument doc = cur.Current();
                BsonDocument indexDefInfo = doc.GetElement("IndexDef").Value.ToBsonDocument();
                Assert.AreEqual(true, indexDefInfo.GetElement("unique").Value.ToBoolean());
                Assert.AreEqual(true, indexDefInfo.GetElement("enforced").Value.ToBoolean());
                Assert.AreEqual("{ \"a\" : 1 }", indexDefInfo.GetElement("key").Value.ToString());
            }
            cur.Close();
            cl.DropIndex(indexName);

            BsonDocument options = new BsonDocument();
            options.Add("Unique", true);
            options.Add("Enforced", true);
            options.Add("NotNull", true);
            options.Add("SortBufferSize", 0);
            cl.Insert(new BsonDocument("a", null));
            try
            {
                cl.CreateIndex(indexName, new BsonDocument("a", 1), options);
                Assert.Fail("expect CreateIndex failed !");
            }
            catch (BaseException e)
            {
                Assert.AreEqual(-339, e.ErrorCode);
            }
            cl.Truncate();

            cl.CreateIndex(indexName, new BsonDocument("a", 1), options);
            cur = cl.GetIndex(indexName);
            while (cur.Next() != null)
            {
                BsonDocument doc = cur.Current();
                BsonDocument indexDefInfo = doc.GetElement("IndexDef").Value.ToBsonDocument();
                Assert.AreEqual(true, indexDefInfo.GetElement("unique").Value.ToBoolean());
                Assert.AreEqual(true, indexDefInfo.GetElement("enforced").Value.ToBoolean());
                Assert.AreEqual("{ \"a\" : 1 }", indexDefInfo.GetElement("key").Value.ToString());
            }
            cur.Close();
            cl.DropIndex(indexName);

            cl.CreateIndex(indexName, new BsonDocument("a", 1), false, false, 64);
            cur = cl.GetIndex(indexName);
            while (cur.Next() != null)
            {
                BsonDocument doc = cur.Current();
                BsonDocument indexDefInfo = doc.GetElement("IndexDef").Value.ToBsonDocument();
                Assert.AreEqual(false, indexDefInfo.GetElement("unique").Value.ToBoolean());
                Assert.AreEqual(false, indexDefInfo.GetElement("enforced").Value.ToBoolean());
            }
            cur.Close();
            cur = cl.GetIndexes();
            int count = 0;
            while (cur.Next() != null)
            {
                cur.Current();
                count++;
            }
            cur.Close();
            Assert.AreEqual(1, count);
            cl.DropIndex(indexName);

            cl.CreateIdIndex(new BsonDocument("SortBufferSize", 64));
            cur = cl.GetIndex("$id");
            count = 0;
            while (cur.Next() != null)
            {
                BsonDocument doc = cur.Current();
                count++;
            }
            cur.Close();
            Assert.AreEqual(1, count);
            cl.DropIdIndex();
            cl.CreateIdIndex(new BsonDocument("SortBufferSize", 128));
            
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
    }
}
