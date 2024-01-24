using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB.Bson;
using SequoiaDB;
using CSharp.TestCommon;

namespace CSharp.Aggregate
{
    /**
     * description: aggregate (List< BsonDocument > obj)
     *              1、aggregate使用所有聚集符组合查询oup 2、检查返回的记录内容和记录条数正确性 
     * testcase:    14549
     * author:      chensiqin
     * date:        2019/03/29
     */

    [TestClass]
    public class TestAggregate14549
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private string clName = "cl14549";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
        }

        [TestMethod]
        public void Test14549()
        {
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);
            if (cs.IsCollectionExist(clName))
            {
                cs.DropCollection(clName);
            }
            cl = cs.CreateCollection(clName);
            List<BsonDocument> records = new List<BsonDocument>();
            for (int i = 0; i < 10; i++)
            {
                BsonDocument doc = new BsonDocument();
                doc.Add("_id", i);
                doc.Add("a", i);
                doc.Add("b", i);
                doc.Add("c", i);
                records.Add(doc);
            }
            cl.BulkInsert(records, 0);
            List<BsonDocument> conditions = new List<BsonDocument>();
            BsonDocument matcher = new BsonDocument();
            matcher.Add("$match", new BsonDocument("a", new BsonDocument("$gte", 5)));
            BsonDocument limit = new BsonDocument("$limit", 4);
            BsonDocument sort = new BsonDocument("$sort", new BsonDocument("b", -1));
            BsonDocument skip = new BsonDocument("$skip", 1);
            BsonDocument group = new BsonDocument();
            group.Add("$project", new BsonDocument().Add("a",1).Add("b",1).Add("c", 0));

            conditions.Add(matcher);
            conditions.Add(limit);
            conditions.Add(sort);
            conditions.Add(skip);
            conditions.Add(group);
            DBCursor cur = cl.Aggregate(conditions);
            string expected = "{ \"a\" : 7, \"b\" : 7 }{ \"a\" : 6, \"b\" : 6 }{ \"a\" : 5, \"b\" : 5 }";
            string actual = "";
            while (cur.Next() != null)
            {
                BsonDocument re = cur.Current();
                actual += re.ToString();
            }
            cur.Close();
            Assert.AreEqual(expected, actual);

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
