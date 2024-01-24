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
     * description: upsert shardingKey 
     *              test interface:  Upsert ( BsonDocument  matcher,BsonDocument  modifier,BsonDocument  hint,BsonDocument  setOnInsert,int flag )
     *                  flag = FLG_UPDATE_KEEP_SHARDINGKEY
     * testcase:    12647
     * author:      wuyan
     * date:        2018/3/12
    */
    [TestClass]
    public class UpsertShardingKey12647
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private string clName = "upsert12647";

       // [TestInitialize()]
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
                                    {"ShardingKey", new BsonDocument{ {"no", 1} } },
                                    {"ReplSize", 0 },
                                    {"Compressed", true},
                                    {"CompressionType", "lzw"}
                                };
            cl = cs.CreateCollection(clName, options);
            cl.CreateIndex("testIndex", new BsonDocument { { "no", 1 }, { "str", 1 } }, false, false);
            InsertDatas();
        }

       // [TestMethod()]
        [Ignore]
        public void TestUpsertShardingKey12647()
        {
            if (Common.IsStandalone(sdb))
            {
                return;
            }
            MatchUpsert();
            NoMatchUpsert();
            NoUpsertShardingKey();
        }

        //[TestCleanup()]
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
            for (int i = 0; i < 5; i++)
            {
                BsonDocument obj = new BsonDocument();
                obj.Add("no", i).
                    Add("str", "test" + i).
                    Add("arr", new BsonArray() { "test", 123 + i });
                insertor.Add(obj);
            }
            cl.BulkInsert(insertor, 0);
            return insertor;
        }

        //test a: no matching to condition,insert new data
        private void NoMatchUpsert()
        {

            BsonDocument mValue = new BsonDocument { { "no", 1000 } };
            BsonDocument mValue1 = new BsonDocument { { "arr", new BsonArray() { "testupsert2" } } };
            BsonDocument modifier = new BsonDocument { { "$inc", mValue }, { "$set", mValue1 } };
            BsonDocument matcher = new BsonDocument { { "str", "test1000" } };
            BsonDocument setOnInsert = new BsonDocument { { "add", "testadd" } };
            BsonDocument hint = new BsonDocument { { "", "testIndex" } };
            cl.Upsert(matcher, modifier, hint, setOnInsert, SDBConst.FLG_UPDATE_KEEP_SHARDINGKEY);

            //check result
            long updateCount = cl.GetCount(new BsonDocument 
                                        { 
                                            { "no", 1000 }, 
                                            { "arr", new BsonArray() { "testupsert2"}} ,
                                            { "str", "test1000" },
                                            { "add", "testadd"}
                                        });
            Assert.AreEqual(1, updateCount);
            long count = cl.GetCount(null);
            Assert.AreEqual(6, count);
        }

        //test b: matching to the condition
        private void MatchUpsert()
        {
            //List<BsonDocument> insertRecords = InsertDatas();           
            BsonDocument mValue = new BsonDocument { { "no", 200 } };
            BsonDocument mValue1 = new BsonDocument { { "arr.0", "testupsert1" } };
            BsonDocument modifier = new BsonDocument { { "$inc", mValue }, { "$set", mValue1 } };
            BsonDocument matcher = new BsonDocument { { "str", "test3" } };
            BsonDocument hint = new BsonDocument { { "", "testIndex" } };
            cl.Upsert(matcher, modifier, hint, null, SDBConst.FLG_UPDATE_KEEP_SHARDINGKEY);

            //check result
            /* Console.WriteLine(insertRecords.ToJson());
                insertRecords.ElementAt(3).Set("no", 203);
                insertRecords.ElementAt(3).Set("arr", new BsonArray() { "testupsert1", 126 });     */

            // Console.WriteLine(insertRecords.ToJson());               
            //checkUpsertRecords(insertRecords);
            long updateCount = cl.GetCount(new BsonDocument 
                                        { 
                                            { "no", 203 }, 
                                            { "arr", new BsonArray() { "testupsert1", 126 }} ,
                                            { "str", "test3" }
                                        });
            Assert.AreEqual(1, updateCount);
            long count = cl.GetCount(null);
            Assert.AreEqual(5, count);


        }

        //test c: set flag =0 
        private void NoUpsertShardingKey()
        {
            BsonDocument mValue = new BsonDocument { { "no", 20 } };
            BsonDocument mValue1 = new BsonDocument { { "arr.0", new BsonArray() { "testnoUpsert" } } };
            BsonDocument modifier = new BsonDocument { { "$inc", mValue }, { "$set", mValue1 } };
            BsonDocument matcher = new BsonDocument { { "str", "test0" } };
            cl.Upsert(matcher, modifier, null, null, 0);

            //check result
            long updateCount = cl.GetCount(new BsonDocument 
                                        { 
                                            { "no", 0 }, 
                                            { "arr", new BsonArray() { new BsonArray(){ "testnoUpsert"}, 123 }} ,
                                            { "str", "test0" }                                                
                                        });
            Assert.AreEqual(1, updateCount);
            long count = cl.GetCount(null);
            Assert.AreEqual(6, count);
        }


        private void checkUpsertRecords(List<BsonDocument> expRecords)
        {
            List<BsonDocument> actRecords = new List<BsonDocument>();
            DBQuery query = new DBQuery();
            DBCursor cursor = cl.Query(query);
            while (cursor.Next() != null)
            {
                BsonDocument actObj = cursor.Current();
                actRecords.Add(actObj);
            }
            cursor.Close();
            Console.WriteLine(actRecords.ToJson());
            expRecords.Sort(delegate(BsonDocument x, BsonDocument y)
            {
                return x.CompareTo(y);
            });
            /* actRecords.Sort(delegate(BsonDocument x, BsonDocument y)
            {
                return x.CompareTo(y);
            });*/

            Console.WriteLine(actRecords.ToJson());

            //Console.ReadLine();
            Assert.AreEqual(expRecords.ToJson(), actRecords.ToJson());
        }

    }
}
