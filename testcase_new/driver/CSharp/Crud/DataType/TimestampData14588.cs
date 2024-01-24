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
    * description: insert Timestamp data, then update / query / delete                   
    * testcase:    14588
    * author:      wuyan
    * date:        2018/3/15
    */
    [TestClass]
    public class TimestampData14588
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private const string clName = "TimestampData_14588";

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
        public void TestTimestampData14588()
        {
            InsertAndQueryTimestampData();
            UpdateTimestampData();
            DeleteTimestampData();
            jira_3348_BsonTimestampTest();
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

        public void jira_3348_BsonTimestampTest()
        {
            cs.DropCollection(clName);
            cl = cs.CreateCollection(clName);
            int maxIntSec = 2147483647;
            int minIntSec = -2147483648;
            int secmin = 0;
            int secmax = 999999;
            long timestamp1 = ((long)maxIntSec << 32) + 0;
            long timestamp2 = ((long)minIntSec << 32) + 0;
            BsonDocument insertor1 = new BsonDocument { { "a", new BsonTimestamp(timestamp1) }, { "b", 1 } };
            BsonDocument insertor2 = new BsonDocument { { "a", new BsonTimestamp(timestamp2) }, { "b", 2 } };
            BsonDocument insertor3 = new BsonDocument { { "a", new BsonTimestamp(maxIntSec, secmax) }, { "b", 3 } };
            BsonDocument insertor4 = new BsonDocument { { "a", new BsonTimestamp(maxIntSec, secmin) }, { "b", 4 } };
            BsonDocument insertor5 = new BsonDocument { { "a", new BsonTimestamp(minIntSec, secmax) }, { "b", 5 } };
            BsonDocument insertor6 = new BsonDocument { { "a", new BsonTimestamp(minIntSec, secmin) }, { "b", 6 } };
            BsonDocument insertor7 = new BsonDocument { { "a", new BsonTimestamp(minIntSec, -1) }, { "b", 7 } };
            BsonDocument insertor8 = new BsonDocument { { "a", new BsonTimestamp(minIntSec, 1000000) }, { "b", 8 } };
            BsonDocument insertor9 = new BsonDocument { { "a", new BsonTimestamp(maxIntSec, -1) }, { "b", 9 } };
            BsonDocument insertor10 = new BsonDocument { { "a", new BsonTimestamp(maxIntSec, 1000000) }, { "b", 10 } };
            cl.Insert(insertor1);
            cl.Insert(insertor2);
            cl.Insert(insertor3);
            cl.Insert(insertor4);
            cl.Insert(insertor5);
            cl.Insert(insertor6);
            cl.Insert(insertor7);
            cl.Insert(insertor8);
            cl.Insert(insertor9);
            cl.Insert(insertor10);
            DBCursor cursor = cl.Query();
            BsonDocument record;
            List<BsonDocument> bsonList = new List<BsonDocument>();
            while ((record = cursor.Next()) != null)
            {
                bsonList.Add(record);
            }

            Assert.AreEqual(maxIntSec, bsonList[0].GetValue("a").AsBsonTimestamp.Timestamp);
            Assert.AreEqual(0, bsonList[0].GetValue("a").AsBsonTimestamp.Increment);

            Assert.AreEqual(minIntSec, bsonList[1].GetValue("a").AsBsonTimestamp.Timestamp);
            Assert.AreEqual(0, bsonList[1].GetValue("a").AsBsonTimestamp.Increment);

            Assert.AreEqual(maxIntSec, bsonList[2].GetValue("a").AsBsonTimestamp.Timestamp);
            Assert.AreEqual(secmax, bsonList[2].GetValue("a").AsBsonTimestamp.Increment);

            Assert.AreEqual(maxIntSec, bsonList[3].GetValue("a").AsBsonTimestamp.Timestamp);
            Assert.AreEqual(secmin, bsonList[3].GetValue("a").AsBsonTimestamp.Increment);

            Assert.AreEqual(minIntSec, bsonList[4].GetValue("a").AsBsonTimestamp.Timestamp);
            Assert.AreEqual(secmax, bsonList[4].GetValue("a").AsBsonTimestamp.Increment);

            Assert.AreEqual(minIntSec, bsonList[5].GetValue("a").AsBsonTimestamp.Timestamp);
            Assert.AreEqual(secmin, bsonList[5].GetValue("a").AsBsonTimestamp.Increment);

            Assert.AreEqual(minIntSec - 1, bsonList[6].GetValue("a").AsBsonTimestamp.Timestamp);
            Assert.AreEqual(999999, bsonList[6].GetValue("a").AsBsonTimestamp.Increment);

            Assert.AreEqual(minIntSec + 1, bsonList[7].GetValue("a").AsBsonTimestamp.Timestamp);
            Assert.AreEqual(0, bsonList[7].GetValue("a").AsBsonTimestamp.Increment);

            Assert.AreEqual(maxIntSec - 1, bsonList[8].GetValue("a").AsBsonTimestamp.Timestamp);
            Assert.AreEqual(999999, bsonList[8].GetValue("a").AsBsonTimestamp.Increment);

            Assert.AreEqual(maxIntSec + 1, bsonList[9].GetValue("a").AsBsonTimestamp.Timestamp);
            Assert.AreEqual(0, bsonList[9].GetValue("a").AsBsonTimestamp.Increment);
        } 

        private void InsertAndQueryTimestampData()
        {
            //insert date           

            long timestamp1 = (-6847833601L << 32) + 0;
            long timestamp2 = (253402271999L << 32) + 0;
            //timestamp : 1752-12-31 23:59:59  
            BsonDocument insertor1 = new BsonDocument { { "a", new BsonTimestamp(timestamp1) }, { "b", 1 } };
            //timestamp : 9999-12-31
            BsonDocument insertor2 = new BsonDocument { { "a", new BsonTimestamp(timestamp2) }, { "b", 2 } };
            //timestamp : 1902-01-01 00:00:00
            BsonDocument insertor3 = new BsonDocument { { "a", new BsonTimestamp(-2145945600, 0) }, { "b", 3 } };
            //timestamp:  2037-12-31 23:59:59.999999
            BsonDocument insertor4 = new BsonDocument { { "a", new BsonTimestamp(2145887999, 999999) }, { "b", 4 } };
            //timestamp:  1970-01-01            
            BsonDocument insertor5 = new BsonDocument { { "a", new BsonTimestamp(-28801, 0) }, { "b", 5 } };
            cl.Insert(insertor1);
            cl.Insert(insertor2);
            cl.Insert(insertor3);
            cl.Insert(insertor4);
            cl.Insert(insertor5);

            //test query        
            BsonDocument matcherConf1 = new BsonDocument { { "a", new BsonTimestamp(timestamp1) } };
            QueryAndCheckResult(matcherConf1, insertor1);
            BsonDocument matcherConf2 = new BsonDocument { { "a", new BsonTimestamp(timestamp2) } };
            QueryAndCheckResult(insertor2, insertor2);
            BsonDocument matcherConf3 = new BsonDocument { { "a", new BsonTimestamp(2145887999, 999999) } };
            QueryAndCheckResult(insertor3, insertor3);
            BsonDocument matcherConf4 = new BsonDocument { { "a", new BsonTimestamp(-28801, 0) } };
            QueryAndCheckResult(insertor4, insertor4);

            //check insert records num 
            long count = cl.GetCount(null);
            Assert.AreEqual(5, count);
        }

        private void UpdateTimestampData()
        {
            // update timestamp
            BsonDocument modifier = new BsonDocument { { "$set", new BsonDocument { { "a", new BsonTimestamp(145887999, 999999) } } } };
            BsonDocument matcher = new BsonDocument { { "b", 4 } };
            cl.Update(matcher, modifier, null);
            long updateCount = cl.GetCount(new BsonDocument { { "a", new BsonTimestamp(145887999, 999999) }, { "b", 4 } });
            Assert.AreEqual(1, updateCount);

            //update timestamp to  other type            
            BsonDocument modifier1 = new BsonDocument { { "$set", new BsonDocument { { "a", true } } } };
            BsonDocument matcher1 = new BsonDocument { { "b", 5 } };
            cl.Update(matcher1, modifier1, null);
            long updateCount1 = cl.GetCount(new BsonDocument { { "a", true }, { "b", 5 } });
            Assert.AreEqual(1, updateCount1);
            Assert.AreEqual(5, cl.GetCount(null));
        }

        private void DeleteTimestampData()
        {
            //delete  data 
            BsonDocument matcher = new BsonDocument { { "a", new BsonTimestamp(-2145945600, 0) } };
            cl.Delete(matcher);
            //check result
            long deleteCount = cl.GetCount(matcher);
            Assert.AreEqual(0, deleteCount);
            long existNum = 4;
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
            Console.WriteLine("expRecord=" + expRecord.ToJson());
            Console.WriteLine("actRecord=" + actObj.ToJson());
            Assert.AreEqual(expRecord, actObj);
        }
    }
}
