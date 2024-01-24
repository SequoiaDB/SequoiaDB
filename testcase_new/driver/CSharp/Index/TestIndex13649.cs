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
     * description:  1、创建cs cl，创建索引，插入数据
     *               2、执行sdbGetQueryMeta获取查询元数据，cond指定为索引字段，覆盖验证指定hint为NULL,hint为空bson对象，hint为{ "":null }
     * testcase:    14549
     * author:      chensiqin
     * date:        2019/04/1
     */

    [TestClass]
    public class TestIndex13649
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private string clName = "cl13649";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
        }

        [TestMethod]
        public void Test13649()
        {
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);
            if (cs.IsCollectionExist(clName))
            {
                cs.DropCollection(clName);
            }
            cl = cs.CreateCollection(clName);
            string indexName = "index13649";
            cl.CreateIndex(indexName, new BsonDocument("a", -1), true, true, 64);
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
            BsonDocument query = new BsonDocument();
            query.Add("a", new BsonDocument().Add("$lt", 50).Add("$gt", 40));
            DBCursor cur = cl.GetQueryMeta(query, null, null, 0, 10);
            while (cur.Next() != null)
            {
                BsonDocument doc = cur.Current();
                Assert.AreEqual("ixscan", doc.GetElement("ScanType").Value.ToString());
            }
            cur.Close();

            cur = cl.GetQueryMeta(query, null, new BsonDocument("",BsonNull.Value), 0, 10);
            while (cur.Next() != null)
            {
                BsonDocument doc = cur.Current();
                Assert.AreEqual("tbscan", doc.GetElement("ScanType").Value.ToString());
            }
            cur.Close();

            cur = cl.GetQueryMeta(query, null, new BsonDocument(), 0, 10);
            while (cur.Next() != null)
            {
                BsonDocument doc = cur.Current();
                Assert.AreEqual("ixscan", doc.GetElement("ScanType").Value.ToString());
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
