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
    * description: insert Double data, then update / query / delete                   
    * testcase:    14596
    * author:      wuyan
    * date:        2018/3/14
    */
    [TestClass]
    public class DoubleData14596
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private const string clName = "DoubleTypeData_14596";

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
        public void TestDoubleData14596()
        {
            InsertAndQueryDoubleData();
            UpdateDoubleData();
            DeleteDoubleData();
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

        private void InsertAndQueryDoubleData()
        {
            //insert date                         
            BsonDocument insertor1 = new BsonDocument { { "a", Double.MaxValue }, { "b", 1 } };
            BsonDocument insertor2 = new BsonDocument { { "a", Double.MinValue }, { "b", 2 } };
            BsonDocument insertor3 = new BsonDocument { { "a", 1.7E+308 }, { "b", 3 } };
            BsonDocument insertor4 = new BsonDocument { { "a", new BsonDouble(-1.7E+308) }, { "b", 4 } };
            cl.Insert(insertor1);
            cl.Insert(insertor2);
            cl.Insert(insertor3);
            cl.Insert(insertor4);

            //test query by Double data           
            BsonDocument matcherConf1 = new BsonDocument { { "a", Double.MaxValue } };
            QueryAndCheckResult(matcherConf1, insertor1);
            BsonDocument matcherConf2 = new BsonDocument { { "a", Double.MinValue } };
            QueryAndCheckResult(matcherConf2, insertor2);
            BsonDocument matcherConf3 = new BsonDocument { { "a", 1.7E+308 } };
            QueryAndCheckResult(matcherConf3, insertor3);
            BsonDocument matcherConf4 = new BsonDocument { { "a", new BsonDouble(-1.7E+308) } };
            QueryAndCheckResult(matcherConf4, insertor4);

            //check insert records num 
            long count = cl.GetCount(null);
            Assert.AreEqual(4, count);
        }

        private void UpdateDoubleData()
        {
            // update Double
            BsonDocument modifier = new BsonDocument { { "$set", new BsonDocument { { "a", -1.7E+308 } } } };
            BsonDocument matcher = new BsonDocument { { "b", 1 } };
            cl.Update(matcher, modifier, null);
            long updateCount = cl.GetCount(new BsonDocument { { "a", -1.7E+308 }, { "b", 1 } });
            Assert.AreEqual(1, updateCount);

            //update Double to  other type            
            BsonDocument modifier1 = new BsonDocument { { "$set", new BsonDocument { { "a", "1.23" } } } };
            BsonDocument matcher1 = new BsonDocument { { "b", 4 } };
            cl.Update(matcher1, modifier1, null);
            long updateCount1 = cl.GetCount(new BsonDocument { { "a", "1.23" }, { "b", 4 } });
            Assert.AreEqual(1, updateCount1);
            Assert.AreEqual(4, cl.GetCount(null));
        }

        private void DeleteDoubleData()
        {
            //delete  data 
            BsonDocument matcher = new BsonDocument { { "a", -1.7e+308 } };
            cl.Delete(matcher);
            //check result
            long deleteCount = cl.GetCount(matcher);
            Assert.AreEqual(0, deleteCount);
            long existNum = 3;
            Assert.AreEqual(existNum, cl.GetCount(null));

            //delete the Double.MinValue data 
            BsonDocument matcher1 = new BsonDocument { { "a", Double.MinValue } };
            cl.Delete(matcher1);
            //check result
            long deleteCount1 = cl.GetCount(matcher1);
            Assert.AreEqual(0, deleteCount1);
            long existNum1 = 2;
            Assert.AreEqual(existNum1, cl.GetCount(null));
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
