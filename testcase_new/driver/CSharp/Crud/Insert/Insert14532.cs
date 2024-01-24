using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB;
using CSharp.TestCommon;
using SequoiaDB.Bson;

namespace CSharp.Crud.Insert
{
    /**
     * description: interface parameter check, legal validation is  overwritten in other use cases,only illegal validation is verified
     *              test interface:  insert (BsonDocument record)/bulkInsert (List< BsonDocument > records, int flag)
     * testcase:    14532
     * author:      wuyan    
     * date:        2018/3/9
    */
    [TestClass]
    public class Insert14532
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private string clName = "cl14532";

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
        public void TestInsert14532()
        {
            TestInsert();
            TestBulkInsert();
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

        //test the parameter input is null
        private void TestInsert()
        {
            //the insertor is "":""
            BsonDocument insertor = new BsonDocument();
            insertor.Add("", "");
            cl.Insert(insertor);
            long count = cl.GetCount(insertor);
            Assert.AreEqual(1, count);

            //the insertor is null
            try 
            {
                BsonDocument insertor1 = new BsonDocument();
                string date = DateTime.Now.ToString();
                insertor1.Add("date", date);                
                cl.Insert(null);
                Assert.Fail("insert must be fail,the errorcode is -6!");
            }
            catch (BaseException e)
            {
                Assert.AreEqual( -6, e.ErrorCode);
            }             
        }

         //test the parameter input is null
        private void TestBulkInsert()
        {
            List<BsonDocument> insertor = new List<BsonDocument>();
            for (int i = 0; i < 2; i++)
            {
                BsonDocument obj = new BsonDocument();
                obj.Add("date", DateTime.Now.ToString());
                obj.Add("", "");
                insertor.Add(obj);
            }  
            //the records is null
            try 
            {
                cl.BulkInsert(null, 0);
                Assert.Fail("bulkInsert must be fail,the errorcode is -6!");
            }
            catch (BaseException e)
            {
                Assert.AreEqual(-6, e.ErrorCode);
            }

            //the flag is error,eg:2
            try
            {
                cl.BulkInsert(insertor, 2);
                Assert.AreEqual(3, cl.GetCount(null));
            }
            catch (BaseException e)
            {
                Assert.Fail("the flag is 2, insert error :" + e.ErrorCode);
            }        
        }
        
    }
}
