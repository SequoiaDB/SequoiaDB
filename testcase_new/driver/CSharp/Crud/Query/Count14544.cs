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
     * description: do count
     *              interface: GetCount(BsonDocument matcher，BsonDocument hint)；
     *                         GetCount(BsonDocument matcher)
     * testcase:    14544
     * author:      linsuqiang
     * date:        2018/3/12
     */

    [TestClass]
    public class Count14544
    {
        private Sequoiadb sdb       = null;
        private CollectionSpace cs  = null;
        private DBCollection cl     = null;
        private const string clName = "cl14544";

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
        public void TestCount14544()
        {
            List<BsonDocument> docs = new List<BsonDocument>
            {
                new BsonDocument{ { "a", 0 }, { "b", "hello" } },
                new BsonDocument{ { "a", 1 }, { "b", "hello" } },
                new BsonDocument{ { "a", 2 }, { "b", "hello" } },
                new BsonDocument{ { "a", 3 }, { "b", "hello" } },
                new BsonDocument{ { "a", 4 }, { "b", "hello" } }
            };
            cl.BulkInsert(docs, 0);

            BsonDocument matcher = new BsonDocument("a", new BsonDocument("$lt", 2));
            long count = cl.GetCount(matcher);
            Assert.AreEqual(2, count);
            
            BsonDocument hint = new BsonDocument("", "$shard");
            count = cl.GetCount(matcher, hint);
            Assert.AreEqual(2, count);
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
    }
}
