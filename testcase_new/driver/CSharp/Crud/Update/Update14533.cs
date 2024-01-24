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
    * description: insert Array data, then update / query / delete                   
    * testcase:    14533
    * author:      wuyan
    * date:        2018/3/16
    */
    [TestClass]
    public class Update14533
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private const string clName = "update14533";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);       
            cl = cs.CreateCollection(clName);
            cl.CreateIndex("testIndex", new BsonDocument { { "a", -1 }}, false, false);
            InsertData();          
        }

        [TestMethod()]
        public void TestUpdate14533()
        {
            // update arr data
            BsonDocument modifier = new BsonDocument { { "$set", new BsonDocument { { "a.1.0", new BsonArray() { "testupdate" } } } } };
            BsonDocument matcher = new BsonDocument { { "b", 2 } };
            cl.Update(matcher, modifier, new BsonDocument { { "", "testIndex" } });
            long updateCount = cl.GetCount(new BsonDocument { { "a.1.0", new BsonArray() { "testupdate" } } });
            Assert.AreEqual(1, updateCount);

            //update             
            BsonDocument modifier1 = new BsonDocument { { "$addtoset", new BsonDocument { { "a.arr", new BsonArray() { new BsonBinaryData(new byte[] { 1, 2 }) } } } } };
            BsonDocument matcher1 = new BsonDocument { { "b", 3 } };
            cl.Update(matcher1, modifier1, new BsonDocument { { "", "testIndex" } });
            long updateCount1 = cl.GetCount(new BsonDocument 
                { 
                    { "a.arr", new BsonArray() { "testarr", new BsonDocument { { "obj", "testobj" } } , new BsonBinaryData(new byte[] { 1,2 })} }
                });
            Assert.AreEqual(1, updateCount1);
            Assert.AreEqual(3, cl.GetCount(null));                     
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

        private void InsertData()
        {
            //insert date {a:"test",b:1}                        
            BsonDocument insertor1 = new BsonDocument 
                 { 
                     { "a", "test"},                          
                     { "b", 1 } 
                 };
            //insert { a:[test1,[{ "$regex": "^a","$options": "i"},{"test1":{"$binary": "F9Y=", "$type": "0"}}]],b:2}
            BsonDocument insertor2 = new BsonDocument 
                { 
                    { "a", new BsonArray() 
                           {
                                "test1", new BsonArray()
                                         { 
                                             new BsonRegularExpression("^a","i") , 
                                             new BsonDocument 
                                             {
                                                 { "test1", new BsonBinaryData(new byte[]{23,214})}
                                
                                             }
                                         }
                           }
                    }, 
                    { "b", 2 } };
            //insert {a:{arr:["testarr",{"obj": "testobj"},{"$binary": "AQI=", "$type": "0"}],c:{"$binary": "AAI=", "$type": "0"}},b:3}
            BsonDocument insertor3 = new BsonDocument 
                { 
                    { "a", new BsonDocument
                           {
                               new BsonDocument 
                               {
                                   {
                                       "arr", new BsonArray() 
                                              { "testarr", new BsonDocument { {"obj","testobj"} }
                                              }
                                   }
                               },
                               {"c", new BsonBinaryData(new byte[]{0,2}) }
                           }
                        
                    }, 
                    { "b", 3 }
                };                      
            cl.Insert(insertor1);
            cl.Insert(insertor2);
            cl.Insert(insertor3);      

            //check insert records num 
            long count = cl.GetCount(null);
            Assert.AreEqual(3 ,count);
        }       
    }
}
