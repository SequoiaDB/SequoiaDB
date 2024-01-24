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
    * description: insert Object data, then update / query / delete                   
    * testcase:    14585
    * author:      wuyan
    * date:        2018/3/14
    */
    [TestClass]
    public class ObjectData14585
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private const string clName = "ObjectTypeData_14585";

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
        public void TestObjectData14585()
        {
            InsertAndQueryObjectData();
            UpdateObjectData();
            DeleteObjectData();
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

        private void InsertAndQueryObjectData()
        {
            //insert date                         
            BsonDocument insertor1 = new BsonDocument 
            { 
                {  "a", 
                    new BsonDocument 
                    { 
                        { "test", "testobj"} 
                    }
                }, 
                { "b", 1 } 
            };
            BsonDocument insertor2 = new BsonDocument 
            { 
                { "a", 
                    new BsonDocument
                    {
                        { "test1",
                            new BsonDocument
                            {
                                {"test2", long.MaxValue},
                                {"test22", 123.45}
                            }
                        }
                    }
                }, 
                { "b", 2 }
            };
            BsonDocument insertor3 = new BsonDocument 
            { 
                { "a",
                    new BsonDocument 
                    {
                        {"arr", new BsonArray() { "test", 123 }}
                    }
                }, 
                { "b", 3 }
            };                      
            cl.Insert(insertor1);
            cl.Insert(insertor2);
            cl.Insert(insertor3);   
          
            //test query        
            BsonDocument matcherConf1 = new BsonDocument { { "a.test", "testobj" } };
            QueryAndCheckResult(insertor1, insertor1);
            BsonDocument matcherConf2 = new BsonDocument { { "a.test1.test2", long.MaxValue } };
            QueryAndCheckResult(insertor2, insertor2);
            BsonDocument matcherConf3 = new BsonDocument { { "a.arr.1", 123 } };
            QueryAndCheckResult(insertor3, insertor3);
            

            //check insert records num 
            long count = cl.GetCount(null);
            Assert.AreEqual(3 ,count);
        }

        private void UpdateObjectData()
        {
            // update object data
            BsonDocument modifier = new BsonDocument { { "$set", new BsonDocument { { "a.test", new BsonArray() { "testupdate" } } } } };
            BsonDocument matcher = new BsonDocument { { "b", 1 } };
            cl.Update(matcher, modifier, null);
            long updateCount = cl.GetCount(new BsonDocument { { "a.test", new BsonArray() { "testupdate" } } });
            Assert.AreEqual(1, updateCount);

            //update             
            BsonDocument modifier1 = new BsonDocument { { "$inc", new BsonDocument { { "a.test1.test2", -807 } } } };
            BsonDocument matcher1 = new BsonDocument { { "b", 2 } };
            cl.Update(matcher1, modifier1, null);
            long updateCount1 = cl.GetCount(new BsonDocument { { "a.test1.test2", 9223372036854775000L } });
            Assert.AreEqual(1, updateCount1);
            Assert.AreEqual(3, cl.GetCount(null));
        }

        private void DeleteObjectData()
        {
            //delete  data 
            BsonDocument matcher = new BsonDocument { { "a.arr.1", 123 } };
            cl.Delete(matcher);
            //check result
            long deleteCount = cl.GetCount(matcher);
            Assert.AreEqual(0, deleteCount);
            long existNum = 2;
            Assert.AreEqual(existNum, cl.GetCount(null));

            //delete data
            BsonDocument matcher1 = new BsonDocument { { "a.test1.test22", 123.45 } };
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
