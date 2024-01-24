using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB.Bson;
using SequoiaDB;
using CSharp.TestCommon;

namespace CSharp.Bson
{
    /**
     * description:  
     *                   构造value为特殊值的decimal数据 
     *                   构造value为max/min/nan（不区分大小写），覆盖带精度和不带精度2种情况，检查构造结果
     * testcase:     seqDB-14635 seqDB-14636
     * author:       chensiqin
     * date:         2019/03/14
    */

    [TestClass]
    public class TestBsonDecimal14635
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private string clName = "cl14635";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
        }

        [TestMethod]
        public void Test14635()
        {
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);
            if (cs.IsCollectionExist(clName))
            {
                cs.DropCollection(clName);
            }
            cl = cs.CreateCollection(clName);
            List<BsonDocument> records = new List<BsonDocument>
            {
                new BsonDocument("MAX", new BsonDecimal("MAX")),
                new BsonDocument("MAX", new BsonDecimal("max")),
                new BsonDocument("MAX", new BsonDecimal("INF")),
                new BsonDocument("MAX", new BsonDecimal("inf")),
                new BsonDocument("MIN", new BsonDecimal("MIN")),
                new BsonDocument("MIN", new BsonDecimal("min")),
                new BsonDocument("MIN", new BsonDecimal("-INF")),
                new BsonDocument("MIN", new BsonDecimal("-inf")),
                new BsonDocument("NaN", new BsonDecimal("NAN")),
                new BsonDocument("NaN", new BsonDecimal("nan")),
                new BsonDocument("MAX", new BsonDecimal("MAX", 10, 2)),
                new BsonDocument("MAX", new BsonDecimal("max", 10, 2)),
                new BsonDocument("MAX", new BsonDecimal("INF", 10, 2)),
                new BsonDocument("MAX", new BsonDecimal("inf", 10, 2)),
                new BsonDocument("MIN", new BsonDecimal("MIN", 10, 2)),
                new BsonDocument("MIN", new BsonDecimal("min", 10, 2)),
                new BsonDocument("MIN", new BsonDecimal("-INF", 10, 2)),
                new BsonDocument("MIN", new BsonDecimal("-inf", 10, 2)),
                new BsonDocument("NaN", new BsonDecimal("NAN", 10, 2)),
                new BsonDocument("NaN", new BsonDecimal("nan", 10, 2))
               
            };

            CheckSpecialDoc(records);
            cl.BulkInsert(records, 0);
            List<BsonDocument> results = QueryAndReturnAll(cl);
            CheckSpecialDoc(results);
            cl.Delete(null);
            Assert.AreEqual(0, cl.GetCount(null));
        }

        private List<BsonDocument> QueryAndReturnAll(DBCollection cl)
        {
            List<BsonDocument> result = new List<BsonDocument>();
            DBCursor cursor = cl.Query();
            while (cursor.Next() != null)
            {
                BsonDocument doc = cursor.Current();
                result.Add(doc);
            }
            cursor.Close();
            return result;
        }

        private void CheckSpecialDoc(List<BsonDocument> docs)
        {
            for (int i = 0; i < docs.Count; ++i)
            {
                BsonDocument doc = docs.ElementAt(i);
                doc.Remove("_id");
                Assert.AreEqual(1, doc.ElementCount);
                BsonElement elem = doc.First();
                BsonDocument expDoc = new BsonDocument(elem.Name, new BsonDecimal(elem.Name)) ;
                Assert.AreEqual(expDoc, doc);
            }
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
