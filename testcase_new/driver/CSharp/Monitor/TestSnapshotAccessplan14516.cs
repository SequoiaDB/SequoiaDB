using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB;
using CSharp.TestCommon;
using SequoiaDB.Bson;

namespace CSharp.Monitor
{
    /**
     * description: get snapshot of the access plan
     *              test interface:  GetSnapshot(int snapType, BsonDocument matcher, BsonDocument selector, BsonDocument orderBy)
     * testcase:    14516
     * author:      wuyan    
     * date:        2018/4/12
    */
    [TestClass]
    public class TestSnapshotAccessplan14516
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private string clName = "cl14516";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());  
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);
            cl = cs.CreateCollection(clName);
            InsertDatas();
        }
        
        [TestMethod()]
        public void TestAccessplanSnapshot14516()
        {
            //query returnNum is 100
            BsonDocument queryConf = BsonDocument.Parse("{no:{$lt:100}}");
            QueryData(queryConf);

            //query returnNum is 500
            BsonDocument queryConf1 = BsonDocument.Parse("{no:{$gte:500}}");
            QueryData(queryConf1);

            //query returnNum is 200
            BsonDocument queryConf2 = BsonDocument.Parse("{'$and':[{no:{$lt:400}},{no:{$gte:200}}]}");
            QueryData(queryConf2);

            //get snapshot by accessplay
            BsonDocument matcher  = BsonDocument.Parse("{'Collection':'" + SdbTestBase.csName + "." + clName + "'}");
            BsonDocument selector = BsonDocument.Parse("{'MinTimeSpentQuery.ReturnNum':{$include:1}}");
            BsonDocument orderBy = BsonDocument.Parse("{'MinTimeSpentQuery.ReturnNum':1}");
            DBCursor cursor = sdb.GetSnapshot(SDBConst.SDB_SNAP_ACCESSPLANS, matcher, selector, orderBy);
            List<BsonDocument> actualList = new List<BsonDocument>();
            while (cursor.Next() != null)
            {
                BsonDocument actObj = cursor.Current();
                actualList.Add(actObj);
            }
            cursor.Close();

            //check the get accessplay snapshot result
            List<BsonDocument> expectList = new List<BsonDocument>();
            expectList.Add(BsonDocument.Parse("{'MinTimeSpentQuery':{'ReturnNum':100}}"));
            expectList.Add(BsonDocument.Parse("{'MinTimeSpentQuery':{'ReturnNum':200}}"));
            expectList.Add(BsonDocument.Parse("{'MinTimeSpentQuery':{'ReturnNum':500}}"));
            expectList.Sort(delegate(BsonDocument x, BsonDocument y)
            {
                return x.CompareTo(y);
            });
            actualList.Sort(delegate(BsonDocument x, BsonDocument y)
            {
                return x.CompareTo(y);
            });
            Assert.AreEqual(actualList.ToJson(), expectList.ToJson());
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
         
        private void InsertDatas()
        {
            List<BsonDocument> insertor = new List<BsonDocument>();
            for (int i = 0; i < 1000; i++)
            {
                BsonDocument obj = new BsonDocument();  
                obj.Add("date", DateTime.Now.ToString());
                obj.Add("no", i);
                insertor.Add(obj);
            }
            cl.BulkInsert(insertor, 0);            
        }

        private void QueryData(BsonDocument matcher)
        {
            DBCursor cursor = cl.Query( matcher, null, null, null);
            List<BsonDocument> querylist = new List<BsonDocument>();
            while (cursor.Next() != null)
            {
                BsonDocument actObj = cursor.Current();
                querylist.Add(actObj);
            }
            cursor.Close();
        }
        
    }
}
