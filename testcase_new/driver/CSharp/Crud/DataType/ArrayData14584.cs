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
    * description: insert Array data, then update / query / delete                   
    * testcase:    14584
    * author:      wuyan
    * date:        2018/3/15
    */
    [TestClass]
    public class ArrayData14584
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private const string clName = "ArrayTypeData_14584";

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
        public void TestArrayData14584()
        {
            InsertAndQueryArrayData();
            UpdateArrayData();
            DeleteArrayData();
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

        private void InsertAndQueryArrayData()
        {
            //insert date : {a:["test", 123],b:1}                   
            BsonDocument insertor1 = new BsonDocument 
            { 
                { "a", new BsonArray() { "test", 123 }},                          
                { "b", 1 } 
            };
            //insert data { a: [ "test1",[ { "$regex": "^a","$options": "i"}, {"test1": {"$binary": "Fxg=", "$type": "0"}}]],b:2}
            BsonDocument insertor2 = new BsonDocument 
                { 
                    { "a", new BsonArray() {
                        "test1", new BsonArray(){ new BsonRegularExpression("^a","i") , new BsonDocument 
                                                    {
                                                        { "test1", new BsonBinaryData(new byte[]{23,24})}
                                
                                                    }
                                                }
                                           }
                    }, 
                    { "b", 2 } };
            //insert { a:[{ "arr": ["testarr",{"obj": "testobj"}] },496], b:3}
            BsonDocument insertor3 = new BsonDocument 
            { 
                { 
                    "a", 
                    new BsonArray()
                    {
                        new BsonDocument 
                        {
                            {
                                "arr", 
                                new BsonArray() 
                                { 
                                    "testarr", 
                                    new BsonDocument 
                                    { 
                                        {"obj","testobj"} 
                                    } 
                                }
                             }
                        },
                        496
                    }
                        
                }, 
                { "b", 3 }
            };                      
            cl.Insert(insertor1);
            cl.Insert(insertor2);
            cl.Insert(insertor3);   
          
            //test query        
            BsonDocument matcherConf1 = new BsonDocument { { "a.1", 123 } };
            QueryAndCheckResult(matcherConf1, insertor1);
            BsonDocument matcherConf2 = new BsonDocument { { "a.1.1.test1", new BsonBinaryData(new byte[] { 23, 24 }) } };
            QueryAndCheckResult(matcherConf2, insertor2);
            BsonDocument matcherConf3 = new BsonDocument { { "a.0.arr.1.obj", "testobj" } };
            QueryAndCheckResult(matcherConf3, insertor3);
            

            //check insert records num 
            long count = cl.GetCount(null);
            Assert.AreEqual(3 ,count);
        }

        private void UpdateArrayData()
        {
            // update arr data
            BsonDocument modifier = new BsonDocument { { "$set", new BsonDocument { { "a.1.1.test1", new BsonArray() { "testupdate" } } } } };
            BsonDocument matcher = new BsonDocument { { "b", 2 } };
            cl.Update(matcher, modifier, null);
            long updateCount = cl.GetCount(new BsonDocument { { "a.1.1.test1", new BsonArray() { "testupdate" } } });
            Assert.AreEqual(1, updateCount);

            //update             
            BsonDocument modifier1 = new BsonDocument { { "$addtoset", new BsonDocument { { "a.0.arr", new BsonArray() { new BsonBinaryData(new byte[] { 1,2 }) } } } } };
            BsonDocument matcher1 = new BsonDocument { { "b", 3 } };
            cl.Update(matcher1, modifier1, null);
            long updateCount1 = cl.GetCount(new BsonDocument 
                { 
                    { "a.0.arr", new BsonArray() { "testarr", new BsonDocument { { "obj", "testobj" } } , new BsonBinaryData(new byte[] { 1,2 })} }
                });
            Assert.AreEqual(1, updateCount1);
            Assert.AreEqual(3, cl.GetCount(null));
        }

        private void DeleteArrayData()
        {
            //delete  data 
            BsonDocument matcher = new BsonDocument { { "a.0.arr.2", new BsonBinaryData(new byte[] { 1, 2 }) } };
            cl.Delete(matcher);
            //check result
            long deleteCount = cl.GetCount(matcher);
            Assert.AreEqual(0, deleteCount);
            long existNum = 2;
            Assert.AreEqual(existNum, cl.GetCount(null));

            //delete data
            BsonDocument matcher1 = new BsonDocument { { "a", new BsonArray() { "test", 123 } } };
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
