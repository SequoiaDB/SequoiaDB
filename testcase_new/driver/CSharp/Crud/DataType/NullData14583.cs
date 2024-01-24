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
    * description: insert Null data, then update / query / delete                   
    * testcase:    14583
    * author:      wuyan
    * date:        2018/3/13 
    */
    [TestClass]
    public class NullData14583
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private string clName = "nullTypeData_14583";

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
        public void TestNullData14583()
        {
            InsertNullData();
            UpdateAndQueryNullData();
            DeleteNullData();
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

        private void InsertNullData()
        {
            //insert date
            List<BsonDocument> recordlists = new List<BsonDocument>();
            BsonDocument insertor1 = new BsonDocument { { "a", BsonNull.Value }, { "b", 1 } };
            BsonDocument insertor2 = new BsonDocument { { "a", "" }, { "b", 2 } };
            BsonDocument insertor3 = new BsonDocument { { "a", null }, { "b", 3 } };
            recordlists.Add(insertor1);
            recordlists.Add(insertor2);
            recordlists.Add(insertor3);
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

        private void UpdateAndQueryNullData()
        {
            BsonDocument mValue = new BsonDocument { { "a", BsonNull.Value } };
            BsonDocument modifier = new BsonDocument { { "$set", mValue } };
            BsonDocument matcher = new BsonDocument { { "b", 2 } };
            cl.Update(matcher, modifier, null);
            //check result and query Null           
            DBQuery query = new DBQuery();
            query.Matcher = new BsonDocument { { "a", BsonNull.Value }, { "b", 2 } };
            query.Selector = new BsonDocument { { "_id", new BsonDocument { { "$include", 0 } } } };            
            DBCursor cursor = cl.Query(query);
            while (cursor.Next() != null)
            {
                BsonDocument actObj = cursor.Current();
                BsonDocument expRecord = new BsonDocument { { "a", BsonNull.Value }, { "b", 2 } };
                Assert.AreEqual(expRecord, actObj);     
            }
            cursor.Close();

            //update NULL to  other type
            BsonDocument mValue1 = new BsonDocument { { "a", "" } };
            BsonDocument modifier1 = new BsonDocument { { "$set", mValue1 } };
            BsonDocument matcher1 = new BsonDocument { { "b", 1 } };
            cl.Update(matcher1, modifier1, null);
            //check result
            long updateCount1 = cl.GetCount(new BsonDocument 
                                        { 
                                            {"a", "" }  ,
                                            { "b", 1}
                                        });
            Assert.AreEqual(1, updateCount1);
            Assert.AreEqual(3, cl.GetCount(null));
        }

        private void DeleteNullData()
        {
            //delete the null data 
            BsonDocument matcher = new BsonDocument { { "a", BsonNull.Value } };
            cl.Delete(matcher);
            //check result
            long deleteCount = cl.GetCount(new BsonDocument { { "a", BsonNull.Value } });
            Assert.AreEqual(0, deleteCount);
            long existNum = 2;
            Assert.AreEqual(existNum, cl.GetCount(null));
        }
    }
}
