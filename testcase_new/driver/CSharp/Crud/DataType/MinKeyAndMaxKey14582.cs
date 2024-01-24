using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB;
using CSharp.TestCommon;
using SequoiaDB.Bson;

namespace CSharp.Crud.DataType
{
    /**
    * description: insert MaxKey/MinKey data, then update / query / delete                   
    * testcase:    14582
    * author:      wuyan
    * date:        2018/3/13 
    */
    [TestClass]
    public class MinKeyAndMaxKey14582
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private string clName = "cl_14582";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);
            cl = cs.CreateCollection(clName);
        }

        [TestMethod()]
        public void TestMaxKeyAndMinKey14582()
        {
            InsertMinkeyAndMaxKeyData();
            UpdateAndQueryMinkeyAndMaxKeyData();
            DeleteMinkeyAndMaxKeyData();

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

        private void InsertMinkeyAndMaxKeyData()
        {
            //insert date
            List<BsonDocument> recordlists = new List<BsonDocument>();
            BsonDocument insertor1 = new BsonDocument { { "a", BsonMaxKey.Value }, { "b", 1 } };
            BsonDocument insertor2 = new BsonDocument { { "a", BsonMinKey.Value }, { "b", 2 } };
            BsonDocument insertor3 = new BsonDocument { { "a", new BsonDocument { { "$maxKey", 1 } } }, { "b", 3 } };
            BsonDocument insertor4 = new BsonDocument { { "a", new BsonDocument { { "$minKey", 1 } } }, { "b", 4 } };
            recordlists.Add(insertor1);
            recordlists.Add(insertor2);
            recordlists.Add(insertor3);
            recordlists.Add(insertor4);
            cl.BulkInsert(recordlists, 0);

            //query and check insert result 
            DBQuery query = new DBQuery();
            DBCursor cursor = cl.Query(query);
            List<BsonDocument> querylist = new List<BsonDocument>();
            while (cursor.Next() != null)
            {
                BsonDocument actObj = cursor.Current();
                querylist.Add(actObj);
            }
            cursor.Close();

            recordlists.Sort(delegate(BsonDocument x, BsonDocument y)
            {
                return x.CompareTo(y);
            });
            querylist.Sort(delegate(BsonDocument x, BsonDocument y)
            {
                return x.CompareTo(y);
            });
            Assert.AreEqual(recordlists.ToJson(), querylist.ToJson());
        }

        private void UpdateAndQueryMinkeyAndMaxKeyData()
        {
            //update the MaxKey data
            BsonDocument mValue = new BsonDocument { { "a", new BsonArray() { "testMaxKey" } } };
            BsonDocument modifier = new BsonDocument { { "$set", mValue } };
            BsonDocument matcher = new BsonDocument { { "b", 1 } };
            cl.Update(matcher, modifier, null);
            //check result           
            long updateCount = cl.GetCount(new BsonDocument 
                                        { 
                                            {"a", new BsonArray() { "testMaxKey" } }  ,
                                            { "b", 1}
                                        });
            Assert.AreEqual(1, updateCount);

            //update MinKey to  MaxKey data
            BsonDocument mValue1 = new BsonDocument { { "a", BsonMaxKey.Value } };
            BsonDocument modifier1 = new BsonDocument { { "$set", mValue1 } };
            BsonDocument matcher1 = new BsonDocument { { "b", 4 } };
            cl.Update(matcher1, modifier1, null);
            //check result and query Null           
            DBQuery query = new DBQuery();
            BsonDocument expMatch = new BsonDocument { { "a", BsonMaxKey.Value }, { "b", 4 } };
            query.Matcher = expMatch;
            query.Selector = new BsonDocument { { "_id", new BsonDocument { { "$include", 0 } } } };
            //Console.WriteLine(query.ToJson());
            DBCursor cursor = cl.Query(query);
            while (cursor.Next() != null)
            {
                BsonDocument actObj = cursor.Current();
                Assert.AreEqual(expMatch, actObj);
            }
            cursor.Close();
            Assert.AreEqual(4, cl.GetCount(null));

        }

        private void DeleteMinkeyAndMaxKeyData()
        {

            //delete the MinKey data 
            BsonDocument matcher = new BsonDocument { { "a", BsonMinKey.Value } };
            cl.Delete(matcher);
            //check result
            long deleteCount = cl.GetCount(new BsonDocument { { "a", BsonMinKey.Value } });
            Assert.AreEqual(0, deleteCount);
            long existNum = 3;
            Assert.AreEqual(existNum, cl.GetCount(null));

            //delete the MaxKey data            
            BsonDocument matcher1 = new BsonDocument { { "a", BsonMaxKey.Value } };
            cl.Delete(matcher1);
            //check result
            long deleteCount1 = cl.GetCount(new BsonDocument 
                                        { 
                                            {"a", BsonMaxKey.Value }  ,
                                            { "b", 4}
                                        });
            Assert.AreEqual(0, deleteCount1);
            long remainNum = 2;
            Assert.AreEqual(remainNum, cl.GetCount(null));
        }

    }
}
