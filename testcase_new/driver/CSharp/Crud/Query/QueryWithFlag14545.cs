using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB;
using CSharp.TestCommon;
using SequoiaDB.Bson;

namespace CSharp.Crud.Query
{
    /**
     * description: query with different flags
     *              interface: Query (BsonDocument query，BsonDocument selector，BsonDocument orderBy，
     *                                BsonDocument hint，long skipRows，long returnRows，int flag)
     * testcase:    14545
     * author:      linsuqiang
     * date:        2018/3/12
     */

    [TestClass]
    public class QueryWithFlag14545
    {
        private Sequoiadb sdb       = null;
        private CollectionSpace cs  = null;
        private DBCollection cl     = null;
        private const string clName = "cl14545";
        private List<BsonDocument> insertDocs = new List<BsonDocument>();

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);
            BsonDocument options = new BsonDocument("ShardingKey", new BsonDocument("a", 1));
            cl = cs.CreateCollection(clName, options);

            const int docNum = 5;
            for (int i = 0; i < docNum; ++i)
                insertDocs.Add(new BsonDocument{ { "a", i }, { "b", "hello" } });
            cl.BulkInsert(insertDocs, 0);
        }
        
        [TestMethod()]
        public void TestQueryWithFlag14545()
        {
            BsonDocument matcher        = null;
            BsonDocument selector       = null;
            BsonDocument orderBy        = null;
            BsonDocument hint           = new BsonDocument("", "$shard");
            long skipRows               = 0;
            long returnRows             = -1;
            List<BsonDocument> queryDocs = null;

            List<int> flags = new List<int>
            { 
                0, 
                10 /*invalid flag*/, 
                -1,
                1024,
                4096,
                DBQuery.FLG_QUERY_WITH_RETURNDATA, 
                DBQuery.FLG_QUERY_PARALLED, 
                DBQuery.FLG_QUERY_FORCE_HINT 
            };
            
            for (int i = 0; i < flags.Count; ++i)
            {
                int flag = flags.ElementAt(i);
                queryDocs = QueryAndReturnDoc(cl, matcher, selector, orderBy, hint, skipRows, returnRows, flag);
                Assert.IsTrue(Common.IsEqual(insertDocs, queryDocs), 
                    "expect:" + insertDocs.ToJson() + " actual:" + queryDocs.ToJson());
            }

            returnRows = 0;
            queryDocs = QueryAndReturnDoc(cl, matcher, selector, orderBy, hint, skipRows, returnRows, 0);
            Assert.AreEqual(0, queryDocs.Count, "query should return nothing, but get " + queryDocs.ToJson());
            try
            {
                queryDocs = QueryAndReturnDoc(cl, matcher, selector, orderBy, hint, skipRows, returnRows, -100);
            }
            catch (BaseException e)
            {
                Assert.AreEqual(-6, e.ErrorCode);
            }


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

        private List<BsonDocument> QueryAndReturnDoc(DBCollection cl,
                                                     BsonDocument matcher, 
                                                     BsonDocument selector,
                                                     BsonDocument orderBy,
                                                     BsonDocument hint,
                                                     long skipRows,
                                                     long returnRows,
                                                     int flag
                                                     )
        {
            List<BsonDocument> result = new List<BsonDocument>();
            DBCursor cursor = cl.Query(matcher, selector, orderBy, hint, skipRows, returnRows, flag);
            while (cursor.Next() != null)
            {
                result.Add(cursor.Current());
            }
            cursor.Close();
            return result;
        }
    }
}
