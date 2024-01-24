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
    * description: insert Oid data, then update / query / delete                   
    * testcase:    14591
    * author:      wuyan
    * date:        2018/3/13 
    */
    [TestClass]
    public class OidData14591
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private const string clName = "oidTypeData_14591";

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
        public void TestOidData14591()
        {
            InsertAndQueryOidData();
            UpdateOidData();
            DeleteOidData();
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

        private void InsertAndQueryOidData()
        {
            //insert date
            List<BsonDocument> recordlists = new List<BsonDocument>();
            BsonDocument insertor1 = new BsonDocument { { "_id", new ObjectId() }, { "b", 1 } };
            BsonDocument insertor2 = new BsonDocument { { "_id", new BsonObjectId("12088e93b01cccc069000000") }, { "b", 2 } };
            BsonDocument insertor3 = new BsonDocument { { "_id", new ObjectId("5aa88e93b01dbea069000000") }, { "b", 3 } };
            recordlists.Add(insertor1);
            recordlists.Add(insertor2);
            recordlists.Add(insertor3);
            cl.BulkInsert(recordlists, 0);

            //test query by oid data           
            BsonDocument matcherConf = new BsonDocument { { "_id", new ObjectId() } };
            QueryAndCheckResult(matcherConf, insertor1);
            BsonDocument matcherConf1 = new BsonDocument { { "_id", new BsonObjectId("12088e93b01cccc069000000") } };
            QueryAndCheckResult(matcherConf1, insertor2);
            BsonDocument matcherConf2 = new BsonDocument { { "_id", new ObjectId("5aa88e93b01dbea069000000") } };
            QueryAndCheckResult(matcherConf2, insertor3);

            //check insert records num 
            long count = cl.GetCount(null);
            Assert.AreEqual(3, count);
        }

        private void UpdateOidData()
        {
            BsonDocument modifier = new BsonDocument { { "$set", new BsonDocument { { "_id", new BsonObjectId("2f088e93b01cccc069000000") } } } };
            BsonDocument matcher = new BsonDocument { { "b", 1 } };
            cl.Update(matcher, modifier, null);
            //check result
            long updateCount = cl.GetCount(new BsonDocument 
                                           {
                                               {"_id", new BsonObjectId("2f088e93b01cccc069000000") }  ,
                                               { "b", 1}
                                           });
            Assert.AreEqual(1, updateCount);

            //update Oid to  other type            
            BsonDocument modifier1 = new BsonDocument { { "$set", new BsonDocument { { "_id", new BsonString("testoid") } } } };
            BsonDocument matcher1 = new BsonDocument { { "b", 3 } };
            cl.Update(matcher1, modifier1, null);
            //check result
            long updateCount1 = cl.GetCount(new BsonDocument 
                                           { 
                                               { "_id", new BsonString("testoid") }  ,
                                               { "b", 3 }
                                           });
            Assert.AreEqual(1, updateCount1);
            Assert.AreEqual(3, cl.GetCount(null));
        }

        private void DeleteOidData()
        {
            //delete the null data 
            BsonDocument matcher = new BsonDocument { { "_id", new BsonObjectId("2f088e93b01cccc069000000") } };
            cl.Delete(matcher);
            //check result
            long deleteCount = cl.GetCount(new BsonDocument { { "_id", new BsonObjectId("2f088e93b01cccc069000000") } });
            Assert.AreEqual(0, deleteCount);
            long existNum = 2;
            Assert.AreEqual(existNum, cl.GetCount(null));
        }

        private void QueryAndCheckResult(BsonDocument cond, BsonDocument expRecord)
        {
            DBQuery query = new DBQuery();
            query.Matcher = cond;
            DBCursor cursor = cl.Query(query);
            BsonDocument actObj = null;
            while (cursor.Next() != null)
            {
                actObj = cursor.Current();
            }
            cursor.Close();
            Assert.AreEqual(expRecord, actObj);
        }
    }
}
