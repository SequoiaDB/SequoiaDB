using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB;
using CSharp.TestCommon;
using SequoiaDB.Bson;

namespace CSharp.Crud.Delete
{
    /**
     * description: delete interface param validate
     *              delete (BsonDocument matcher)；
     *              delete (BsonDocument matcher, BsonDocument hint)；
     *              QueryAndRemove (BsonDocument query，BsonDocument selector，BsonDocument orderBy，BsonDocument hint，long skipRows，long retrunRows，int flag) 
     * testcase:    14542
     * author:      chensiqin
     * date:        2018/3/15
    */ 
    [TestClass]
    public class Delete14542
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private string clName = "cl14542";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);
           
        }

        [TestMethod()]
        public void TestDelete14542()
        {
            TestDeleteMatcher();
            TestDeleteMatcherAndHint();
            TestQueryAndRemove(); 
        }

        public void TestDeleteMatcher()
        {
            if (cs.IsCollectionExist(clName))
            {
                cs.DropCollection(clName);
            }
            cl = cs.CreateCollection(clName);
            List<BsonDocument> insertRecords = InsertDatas(10);
            BsonDocument matcher = new BsonDocument();
            matcher.Add("age",0);
            cl.Delete(matcher);
            Assert.AreEqual(0, cl.GetCount(matcher));
            Assert.AreEqual(9, cl.GetCount(null));
            cl.Delete(null);
            Assert.AreEqual(0, cl.GetCount(null));
        }

        public void TestDeleteMatcherAndHint() 
        {
            BsonDocument key = new BsonDocument();
            key.Add("age", 1);
            if (cs.IsCollectionExist(clName))
            {
                cs.DropCollection(clName);
            }
            cl = cs.CreateCollection(clName);
            cl.CreateIndex("ageIndex", key, true, true);
            List<BsonDocument> insertRecords = InsertDatas(10);
            BsonDocument matcher = new BsonDocument();
            matcher.Add("name", "zhangsan11");
            cl.Delete(matcher,new BsonDocument("", "ageIndex"));
            Assert.AreEqual(10, cl.GetCount(null));
            cl.Delete(null, null);
            Assert.AreEqual(0, cl.GetCount(null));
        }

        public void TestQueryAndRemove()
        {
            BsonDocument key = new BsonDocument();
            key.Add("age", 1);
            if (cs.IsCollectionExist(clName))
            {
                cs.DropCollection(clName);
            }
            cl = cs.CreateCollection(clName);
            cl.CreateIndex("ageIndex", key, true, true);
            List<BsonDocument> insertRecords = InsertDatas(20);
            //flag not in range
            DBCursor cursor = cl.QueryAndRemove(null, null, null, new BsonDocument("", "$id"), 0, 2, -100);
            int delRecord = 0;
            while (cursor.Next() != null)
            {
                delRecord++;
            }
            cursor.Close();
            Assert.AreEqual(delRecord, 2);
            Assert.AreEqual(cl.GetCount(null), 18);
            cursor.Close(); 
        }

        [TestCleanup()]
        public void TearDown()
        {
            try
            {
                cs.DropCollection(clName);
                Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " end  : " + this.GetType().ToString());
            }
            catch (BaseException e)
            {
                Assert.Fail("Failed to clearup:", e.ErrorCode + e.Message);
            }
            finally
            {
                if (sdb != null)
                {
                    sdb.Disconnect();
                }
            }
        }

        private List<BsonDocument> InsertDatas(int len)
        {
            List<BsonDocument> dataList = new List<BsonDocument>();
            for (int i = 0; i < len; i++)
            {
                BsonDocument obj = new BsonDocument();
                obj.Add("_id", i);
                obj.Add("age", i);
                obj.Add("sub", "test");
                obj.Add("name", "zhangsan" + i);
                obj.Add("score", 90 + i * 0.1);
                obj.Add("height", 165 + i);
                dataList.Add(obj);
            }
            cl.BulkInsert(dataList, 0);
            return dataList;
        }
    }

}
