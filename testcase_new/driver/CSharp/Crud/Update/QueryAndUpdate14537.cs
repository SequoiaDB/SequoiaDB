using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB;
using CSharp.TestCommon;
using SequoiaDB.Bson;

namespace CSharp.Crud.Update
{
    /**
     * description: query and update, specify all parameter
     *              interface: QueryAndUpdate(...) 
     * testcase:    14537
     * author:      linsuqiang
     * date:        2018/3/12
     */

    [TestClass]
    public class QueryAndUpdate14537
    {
        private Sequoiadb sdb       = null;
        private CollectionSpace cs  = null;
        private DBCollection cl     = null;
        private const string clName = "cl14537";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);
            BsonDocument options = new BsonDocument("ShardingKey", new BsonDocument("a", 1));
            cl = cs.CreateCollection(clName, options);
        }
        
        [TestMethod()]
        public void TestQueryAndUpdate14537()
        {
            InsertData(cl);
            List<BsonDocument> actList = QueryAndUpdateReturnNew(cl);
            List<BsonDocument> expList = GetExpList();
            Assert.IsTrue(Common.IsEqual(expList, actList), 
                "expect:" + expList.ToJson() + " actual:" + actList.ToJson());
        }

        [TestCleanup()]
        public void TearDown()
        {
            try
            {
                cs.DropCollection(clName);
            }
            finally
            {
                if (sdb != null)
                {
                    sdb.Disconnect();    
                }
                Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " end  : " + this.GetType().ToString());
            }
        }
        
        public List<BsonDocument> InsertData(DBCollection cl)
        {
            List<BsonDocument> insertList = new List<BsonDocument> 
            {
                new BsonDocument{ { "a", 1 }, { "b", 0 }, { "c", 0 } },
                new BsonDocument{ { "a", 2 }, { "b", 0 }, { "c", 0 } },
                new BsonDocument{ { "a", 3 }, { "b", 0 }, { "c", 0 } },
                new BsonDocument{ { "a", 4 }, { "b", 0 }, { "c", 0 } },
                new BsonDocument{ { "a", 5 }, { "b", 0 }, { "c", 0 } }
            };
            cl.BulkInsert(insertList, 0);
            return insertList;
        }

        public List<BsonDocument> QueryAndUpdateReturnNew(DBCollection cl)
        {
            BsonDocument query      = new BsonDocument("a", new BsonDocument("$lte", 3));
            BsonDocument selector   = new BsonDocument("c", new BsonDocument("$include", 0));
            BsonDocument orderBy    = new BsonDocument("a", -1);
            BsonDocument hint       = new BsonDocument("", "$shard");
            BsonDocument update     = new BsonDocument("$inc", new BsonDocument("b", 1));
            long skipRows           = 0;
            long returnRows         = -1;
            int flag                = DBQuery.FLG_QUERY_WITH_RETURNDATA;
            bool returnNew          = true;

            DBCursor cursor = cl.QueryAndUpdate(query, selector, orderBy, hint, update, skipRows, returnRows, flag, returnNew);
            List<BsonDocument> list = new List<BsonDocument>();
            while (cursor.Next() != null)
            {
                BsonDocument doc = cursor.Current();
                doc.Remove("_id");
                list.Add(doc);
            }
            cursor.Close();
            cursor = cl.QueryAndUpdate(query, selector, orderBy, hint, update, skipRows, 1, -100, returnNew);
            while (cursor.Next() != null)
            {
                BsonDocument doc = cursor.Current();
                doc.Remove("_id");
                Assert.AreEqual("{ \"a\" : 3, \"b\" : 2 }", doc.ToString());
            }
            cursor.Close();
            return list;
        } 

        public List<BsonDocument> GetExpList()
        {
            List<BsonDocument> list = new List<BsonDocument> 
            {
                new BsonDocument{ { "a", 3 }, { "b", 1 } },
                new BsonDocument{ { "a", 2 }, { "b", 1 } },
                new BsonDocument{ { "a", 1 }, { "b", 1 } }
            };
            return list;
        }
            
    }
}
