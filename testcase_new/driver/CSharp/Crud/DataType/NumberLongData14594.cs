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
    * description: insert NumberLong data, then update / query / delete                   
    * testcase:    14594
    * author:      wuyan
    * date:        2018/3/14
    */
    [TestClass]
    public class NumberLongData14594
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private const string clName = "LongTypeData_14594";

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
        public void TestLongData14594()
        {
            InsertAndQueryLongData();
            UpdateLongData();
            DeleteLongData();
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

        private void InsertAndQueryLongData()
        {
            //insert date                         
            BsonDocument insertor1 = new BsonDocument { { "a", long.MaxValue }, { "b", 1 } };
            BsonDocument insertor2 = new BsonDocument { { "a", long.MinValue }, { "b", 2 } };
            BsonDocument insertor3 = new BsonDocument { { "a", -9223372036854775808L }, { "b", 3 } };
            BsonDocument insertor4 = new BsonDocument { { "a", new BsonInt64(9223372036854775807L) }, { "b", 4 } };           
            cl.Insert(insertor1);
            cl.Insert(insertor2);
            cl.Insert(insertor3);
            cl.Insert(insertor4);
          
            //test query        
            BsonDocument matcherConf1 = new BsonDocument { { "a", long.MaxValue } };
            QueryAndCheckResult(insertor1, insertor1);
            BsonDocument matcherConf2 = new BsonDocument { { "a", long.MinValue } };
            QueryAndCheckResult(insertor2, insertor2);
            BsonDocument matcherConf3 = new BsonDocument { { "a", -9223372036854775808L } };
            QueryAndCheckResult(insertor3, insertor3);
            BsonDocument matcherConf4 = new BsonDocument { { "a", new BsonInt64(9223372036854775807L) } };
            QueryAndCheckResult(insertor4, insertor4);

            //check insert records num 
            long count = cl.GetCount(null);
            Assert.AreEqual(4, count);
        }

        private void UpdateLongData()
        {
            // update numberlong
            BsonDocument modifier = new BsonDocument { { "$set", new BsonDocument { { "a", long.MaxValue } } } };
            BsonDocument matcher = new BsonDocument { { "b", 2 } };
            cl.Update(matcher, modifier, null);
            long updateCount = cl.GetCount(new BsonDocument { { "a", 9223372036854775807L }, { "b", 2 } });
            Assert.AreEqual(1, updateCount);

            //update numberlong to  other type            
            BsonDocument modifier1 = new BsonDocument { { "$set", new BsonDocument { { "a", true } } } };
            BsonDocument matcher1 = new BsonDocument { { "b", 4 } };
            cl.Update(matcher1, modifier1, null);
            long updateCount1 = cl.GetCount(new BsonDocument { { "a", true }, { "b", 4 } });
            Assert.AreEqual(1, updateCount1);
            Assert.AreEqual(4, cl.GetCount(null));
        }

        private void DeleteLongData()
        {
            //delete  data 
            BsonDocument matcher = new BsonDocument { { "a", -9223372036854775808L } };
            cl.Delete(matcher);
            //check result
            long deleteCount = cl.GetCount(matcher);
            Assert.AreEqual(0, deleteCount);
            long existNum = 3;
            Assert.AreEqual(existNum, cl.GetCount(null));

            //delete the Max Value data 
            BsonDocument matcher1 = new BsonDocument { { "a", long.MaxValue } };
            cl.Delete(matcher1);
            //check result
            long deleteCount1 = cl.GetCount(matcher1);
            Assert.AreEqual(0, deleteCount1);
            long existNum1 = 1;
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
