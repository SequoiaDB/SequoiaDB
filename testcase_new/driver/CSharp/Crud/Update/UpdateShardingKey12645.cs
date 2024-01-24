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
     * description: update shardingKey 
     *              test interface:  Update ( BsonDocument  matcher,BsonDocument  modifier,BsonDocument  hint,int flag )
     *                  flag = FLG_UPDATE_KEEP_SHARDINGKEY
     * testcase:    12645
     * author:      wuyan
     * date:        2018/3/13
    */
    [TestClass]  
    public class UpdateShardingKey12645
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private string clName = "update12645";

        //[TestInitialize()]
          [Ignore]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
            if (Common.IsStandalone(sdb))
            {
                return;
            }
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);
            BsonDocument options = new BsonDocument
                                {
                                    {"ShardingType", "hash"},
                                    {"ShardingKey", new BsonDocument{ {"no", 1}, {"a", -1} } },
                                    {"ReplSize", 0 },
                                    {"Compressed", true},
                                    {"CompressionType", "lzw"}
                                };
            cl = cs.CreateCollection(clName, options);
            cl.CreateIndex("testIndex", new BsonDocument { { "no", 1 }, { "str", 1 } }, false, false);
            InsertDatas();
        }

        //[TestMethod(),Ignore]   
        [Ignore]
        public void TestUpdateShardingKey12645()
        {
            if (Common.IsStandalone(sdb))
            {
                return;
            }
            UpdateShardingKey();
        }

       // [TestCleanup()]
          [Ignore]
        public void TearDown()
        {
            try
            {
                if (Common.IsStandalone(sdb))
                {
                    return;
                }
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

        private List<BsonDocument> InsertDatas()
        {
            List<BsonDocument> insertor = new List<BsonDocument>();
            for (int i = 0; i < 10000; i++)
            {
                BsonDocument obj = new BsonDocument();
                obj.Add("no", i).
                    Add("str", "test" + i).
                    Add("a", i).
                    Add("arr", new BsonArray() { "test", i, "test" + i });
                insertor.Add(obj);
            }
            cl.BulkInsert(insertor, 0);
            return insertor;
        }

        //update ShardingKey 
        private void UpdateShardingKey()
        {
            BsonDocument mValue = new BsonDocument { { "no", 1000 }, { "a", 1000 } };
            BsonDocument mValue1 = new BsonDocument { { "arr.0", "testupdate" } };
            BsonDocument mValue2 = new BsonDocument { { "a", 1000 } };
            BsonDocument modifier = new BsonDocument { { "$inc", mValue }, { "$set", mValue1 } };
            BsonDocument matcher = new BsonDocument { { "str", "test2000" } };
            BsonDocument hint = new BsonDocument { { "", "testIndex" } };
            cl.Update(matcher, modifier, hint, SDBConst.FLG_UPDATE_KEEP_SHARDINGKEY);

            //check result
            long updateCount = cl.GetCount(new BsonDocument 
                                        { 
                                            { "no", 3000 }, 
                                            { "a", 3000 }, 
                                            { "arr", new BsonArray() { "testupdate", 2000, "test2000" }} ,
                                            { "str", "test2000" }                                                
                                        });
            Assert.AreEqual(1, updateCount);
            long count = cl.GetCount(null);
            Assert.AreEqual(10000, count);
        }
    }
}
