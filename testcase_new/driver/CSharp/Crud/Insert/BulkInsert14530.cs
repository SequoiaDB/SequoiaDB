using System;
using System.Text;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB;
using CSharp.TestCommon;
using SequoiaDB.Bson;
using System.Collections.Generic;

namespace CSharp.Crud.Insert
{
    /**
     * description: insert data without oid, generating oid without repetition     *              
     * testcase:    14530
     * author:      wuyan
     * date:        2018/3/19
    */
    [TestClass]
    public class BulkInsert14530
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private string clName = "cl14530";

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
        public void TestBulkInsert14530()
        {
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

        public void TestBulkInsert()
        {
            List<BsonDocument> insertor = new List<BsonDocument>();
            for (int i = 0; i < 20000; i++)
            {
                BsonDocument obj = new BsonDocument();
                obj.Add("date", DateTime.Now.ToString());                          
                insertor.Add(obj);
            }
            cl.BulkInsert(insertor, 0);

            //check the result
            DBQuery query = new DBQuery();
            BsonDocument selector = new BsonDocument{{ "date",  new BsonDocument { {"$include", 0 } } } };
            query.Selector = selector;
            DBCursor cursor = cl.Query(query);
            List<BsonDocument> querylist = new List<BsonDocument>();
            while (cursor.Next() != null)
            {
                BsonDocument actObj = cursor.Current();
                querylist.Add(actObj);
            }            
            cursor.Close();
            checkRepeatValue(querylist);           
        }

        private void checkRepeatValue(List<BsonDocument> list)
        {
            for (int i = 0; i < list.Count; i++)
            {
                for (int j = i + 1; j < list.Count; j++)
                {
                    if (list[i] == list[j])
                    {
                        Assert.Fail("exist the same oid "+ list[i].ToJson());
                    }
                }
            }
        }

        

        
    }
}
