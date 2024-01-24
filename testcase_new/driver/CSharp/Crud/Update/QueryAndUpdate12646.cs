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
     * description: query and update sharding key
     *              interface: QueryAndUpdate(...) 
     * testcase:    12646
     * author:      linsuqiang
     * date:        2018/3/12
     */

    [TestClass]
    public class QueryAndUpdate12646
    {
        private Sequoiadb sdb       = null;
        private CollectionSpace cs  = null;
        private DBCollection cl     = null;

        private const string clName         = "cl12646";
        private const string SHARD_KEY      = "a";
        private const string NON_SHARD_KEY  = "b";

        //[TestInitialize()]
        [Ignore]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);
            BsonDocument options = new BsonDocument();
            options.Add("ShardingKey", new BsonDocument(SHARD_KEY, 1));
            cl = cs.CreateCollection(clName, options);
        }
        
        //[TestMethod()]
        [Ignore]
        public void TestQueryAndUpdate12646()
        {
            BsonDocument oldDoc = new BsonDocument();
            oldDoc.Add(SHARD_KEY, "oldValue");
            oldDoc.Add(NON_SHARD_KEY, "oldValue");
            cl.Insert(oldDoc);

            BsonDocument newDoc = new BsonDocument();
            newDoc.Add(SHARD_KEY, "newValue");
            newDoc.Add(NON_SHARD_KEY, "newValue");

            DBCursor cursor = QueryAndUpdateReturnNew(cl, newDoc);
            BsonDocument updatedDoc = cursor.Next();
            cursor.Close();
            updatedDoc.Remove("_id");
            Assert.AreEqual(updatedDoc, newDoc);
        }

        //[TestCleanup()]
        [Ignore]
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

        public DBCursor QueryAndUpdateReturnNew(DBCollection cl, BsonDocument newDoc)
        {
            BsonDocument query      = null;
            BsonDocument selector   = null;
            BsonDocument orderBy    = null;
            BsonDocument hint       = null;
            long skipRows           = 0;
            long returnRows         = -1;
            bool returnNew          = true;

            BsonDocument update = new BsonDocument("$set", newDoc);
            int flag = DBQuery.FLG_QUERY_KEEP_SHARDINGKEY_IN_UPDATE;

            return cl.QueryAndUpdate(query, selector, orderBy, hint, update, skipRows, returnRows, flag, returnNew);
        } 
    }
}
