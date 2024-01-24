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
     * description: GetIndexInfo(String name)
     *              IsIndexExist(String name)
     * testcase:    15934
     * author:      chensiqin
     * date:        2018/10/08
    */
    [TestClass]
    public class Index15934
    {

        private Sequoiadb sdb = null;
        private CollectionSpace cs;
        private DBCollection cl;
        private string clName = "cl15933";
        private string indexName = "aindex";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
        }


        [TestMethod]
        public void Test15934()
        {
            createCL();
            cl.CreateIndex(indexName, new BsonDocument{{"a",1}}, true, false);
            BsonDocument indexInfo = cl.GetIndexInfo(indexName);
            BsonElement element = indexInfo.GetElement("IndexDef");
            BsonDocument indexDefInfo = element.Value.ToBsonDocument();
            //check index info
            Assert.AreEqual(indexDefInfo.GetElement("name").Value.ToString(), indexName);
            Assert.AreEqual(indexDefInfo.GetElement("key").Value.ToString(), "{ \"a\" : 1 }");
            Assert.AreEqual(indexDefInfo.GetElement("unique").Value, true);
            Assert.AreEqual(indexDefInfo.GetElement("enforced").Value, false);
            //不存在的索引
            try
            {
                cl.GetIndexInfo("csqindex");
            }
            catch (BaseException e)
            {
                Assert.AreEqual(e.ErrorCode, -47);
            }
            Assert.AreEqual(cl.IsIndexExist(indexName), true);
            Assert.AreEqual(cl.IsIndexExist("csqindex"), false);
            DBCursor cur = null;
            cur = cl.GetIndex("csqindex");
            Assert.IsNotNull(cur);
            cur.Close();

            cur = cl.GetIndex(null);
            List<string> actual = new List<string>();
            actual.Add(indexName);
            actual.Add("$shard");
            List<string> expected = new List<string>();
            while (cur.Next() != null)
            {
                BsonDocument next = cur.Current();
                element = indexInfo.GetElement("IndexDef");
                indexDefInfo = element.Value.ToBsonDocument();
                expected.Add(indexDefInfo.GetElement("name").Value.ToString());
            }
            Assert.IsTrue(actual.Contains("$shard"));
            Assert.IsTrue(actual.Contains(indexName));
            Assert.AreEqual(2, actual.Count());
            cur.Close();

            cur = cl.GetIndex(indexName);
            while (cur.Next() != null)
            {
                BsonDocument next = cur.Current();
                element = indexInfo.GetElement("IndexDef");
                indexDefInfo = element.Value.ToBsonDocument(); 
                //check index info
                Assert.AreEqual(indexDefInfo.GetElement("name").Value.ToString(), indexName);
                Assert.AreEqual(indexDefInfo.GetElement("key").Value.ToString(), "{ \"a\" : 1 }");
                Assert.AreEqual(indexDefInfo.GetElement("unique").Value, true);
                Assert.AreEqual(indexDefInfo.GetElement("enforced").Value, false);
            }
            cur.Close();

            cs.DropCollection(clName);
        }

        public void createCL()
        {
            try
            {
                if (!sdb.IsCollectionSpaceExist(SdbTestBase.csName))
                {
                    sdb.CreateCollectionSpace(SdbTestBase.csName);
                }
            }
            catch (BaseException e)
            {
                Assert.AreEqual(-33, e.ErrorCode);
            }
            try
            {
                BsonDocument options = new BsonDocument {
                                                         {"ShardingKey", new BsonDocument{{"a",1}}},
                                                         {"ShardingType","hash"},
                                                         {"Partition",1024},
                                                         {"ReplSize",0},
                                                         {"Compressed",true},
                                                         {"AutoIndexId",false}
                                       };
                cs = sdb.GetCollectionSpace(SdbTestBase.csName);
                cl = cs.CreateCollection(clName, options);
            }
            catch (BaseException e)
            {
                Assert.IsTrue(false, "create cl fail " + e.Message);
            }
        }

        [TestCleanup()]
        public void TearDown()
        {
            if (sdb != null)
            {
                sdb.Disconnect();
            }
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " end  : " + this.GetType().ToString());
        }
    }
}
