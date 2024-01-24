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
     * description:  createIndex (string name，BsonDocument key，bool isUnique，bool isEnforced，int sortBufferSize)
     *               createIndex (string name，BsonDocument key，bool isUnique，bool isEnforced)
     *               dropIndex (string name)
     *               GetIndexes（）
     *               1、向cl中插入数据 
     *               2、创建索引，分别覆盖如下： 
     *                  a、不指定isUnique，isEnforced，sortBufferSize等参数
     *                  b、不指定sortBufferSize参数 
     *                  c、指定所有参数 
     *               3、执行getIndexs查看所有索引信息 
     *               4、查询数据，查看访问计划是否走索引扫描 
     *               5、删除索引 
     *               6、查询记录，查看索引信息
     * testcase:    14547
     * author:      chensiqin
     * date:        2019/04/1
     */

    [TestClass]
    public class TestIndex14547
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private string clName = "cl14547";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
        }

        [TestMethod]
        public void Test14547()
        {
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);
            if (cs.IsCollectionExist(clName))
            {
                cs.DropCollection(clName);
            }
            cl = cs.CreateCollection(clName);

            //不指定isUnique，isEnforced，sortBufferSize等参数,不指定sortBufferSize参数
            string indexName = "index14547";
            cl.CreateIndex(indexName, new BsonDocument("a", -1), false, false);
            List<BsonDocument> insertData = new List<BsonDocument>();
            int dataNum = 100;
            for (int i = 0; i < dataNum; i++)
            {
                BsonDocument doc = new BsonDocument();
                doc.Add("_id", i);
                doc.Add("a", i);
                insertData.Add(doc);
            }
            cl.BulkInsert(insertData, 0);

            DBCursor cur = cl.GetIndexes();
            int count = 0;
            int myIndex = 0;
            while (cur.Next() != null)
            {
                BsonDocument doc = cur.Current();
                count++;
                BsonDocument indexDefInfo = doc.GetElement("IndexDef").Value.ToBsonDocument();
                if (indexDefInfo.GetElement("name").Value.ToString() == indexName)
                {
                    myIndex++;
                    Assert.AreEqual(false, indexDefInfo.GetElement("unique").Value.ToBoolean());
                    Assert.AreEqual(false, indexDefInfo.GetElement("enforced").Value.ToBoolean());
                    Assert.AreEqual("{ \"a\" : -1 }", indexDefInfo.GetElement("key").Value.ToString());
                }
                

            }
            cur.Close();
            Assert.AreEqual(2, count);
            Assert.AreEqual(1, myIndex);

            BsonDocument matcher = new BsonDocument("a", new BsonDocument("$lt", 5));
            BsonDocument order = new BsonDocument("a", 1);
            cur = cl.Explain(matcher, null, order, null, 0, -1, 0, new BsonDocument("Run", true));
            while (cur.Next() != null)
            {
                BsonDocument doc = cur.Current();
                Assert.AreEqual("ixscan", doc.GetElement("ScanType").Value.ToString());
                Assert.AreEqual("index14547", doc.GetElement("IndexName").Value.ToString());
            }
            cur.Close();

            cl.DropIndex(indexName);
            Assert.IsFalse(cl.IsIndexExist(indexName));
            //指定所有参数 
            cl.CreateIndex(indexName, new BsonDocument("a", -1), true, true, 64);
           
            cur = cl.Explain(matcher, null, order, null, 0, -1, 0, new BsonDocument("Run", true));
            while (cur.Next() != null)
            {
                BsonDocument doc = cur.Current();
                Assert.AreEqual("ixscan", doc.GetElement("ScanType").Value.ToString());
                Assert.AreEqual("index14547", doc.GetElement("IndexName").Value.ToString());
            }
            cur.Close();
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
