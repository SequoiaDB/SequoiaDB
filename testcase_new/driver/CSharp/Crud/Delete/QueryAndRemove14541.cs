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
     * description: QueryAndRemove
     *              QueryAndRemove (BsonDocument query，BsonDocument selector，BsonDocument orderBy，BsonDocument hint，long skipRows，long retrunRows，int flag) 
     * testcase:    14541
     * author:      chensiqin
     * date:        2018/3/13
    */
    [TestClass]
    public class QueryAndRemove14541
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private string clName = "cl14541";
         
        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);
        }

        [TestMethod()] 
        public void TestDelete14541()
        {
            TestQueryAndRemoveWithHint();
            TestQueryAndRemoveWithKey();
            TestQueryAndRemoveWithParalled();
            TestQueryAndRemoveWithReturn();
        }
 
        public void TestQueryAndRemoveWithHint() 
        {
            BsonDocument key = new BsonDocument();
            key.Add("age", 1);
            cl = cs.CreateCollection(clName); 
            cl.CreateIndex("ageIndex", key, true, true);
            List<BsonDocument> insertRecords = InsertDatas(20);
            BsonDocument query = new BsonDocument();
            BsonDocument selector = new BsonDocument();
            BsonDocument orderBy = new BsonDocument();
            BsonDocument hint = new BsonDocument();
            long skipRows, retrunRows;
            query.Add("sub", "test");
            selector.Add("_id", 1);
            selector.Add("name", 1);
            selector.Add("score", 1);
            orderBy.Add("age", 1);
            hint.Add("","ageIndex");
            skipRows = 2;
            retrunRows = 5;

            DBCursor cursor = cl.QueryAndRemove(query, selector, orderBy, hint, skipRows, retrunRows, DBQuery.FLG_QUERY_FORCE_HINT);
            while (cursor.Next() != null)
            {
            }
            cursor.Close();
            BsonDocument matcher = new BsonDocument();
            BsonDocument condition = new BsonDocument();
            condition.Add("$gte", 2);
            condition.Add("$lte", 6);
            matcher.Add("_id", condition);
            Assert.AreEqual(0, cl.GetCount(matcher));
            Assert.AreEqual(15, cl.GetCount(null));
        }

        public void TestQueryAndRemoveWithKey() 
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
            BsonDocument query = new BsonDocument();
            BsonDocument selector = new BsonDocument();
            BsonDocument orderBy = new BsonDocument();
            BsonDocument hint = new BsonDocument();
            long skipRows, retrunRows;
            query.Add("sub", "test");
            selector.Add("_id", -1);
            selector.Add("name", 1);
            orderBy.Add("age", 1);
            hint.Add("", "ageIndex");
            skipRows = -1;
            retrunRows = 5;

            DBCursor cursor = cl.QueryAndRemove(query, selector, orderBy, hint, skipRows, retrunRows, DBQuery.FLG_QUERY_KEEP_SHARDINGKEY_IN_UPDATE);
            while (cursor.Next() != null)
            {
            }
            cursor.Close();
            BsonDocument matcher = new BsonDocument();
            BsonDocument condition = new BsonDocument();
            condition.Add("$gte", 0);
            condition.Add("$lte", 4);
            matcher.Add("_id", condition);
            Assert.AreEqual(0, cl.GetCount(matcher));
            Assert.AreEqual(5, cl.GetCount(null));
        }

        public void TestQueryAndRemoveWithParalled() 
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
            BsonDocument query = new BsonDocument();
            BsonDocument selector = new BsonDocument();
            BsonDocument orderBy = new BsonDocument();
            BsonDocument hint = new BsonDocument();
            long skipRows, retrunRows;
            hint.Add("", "ageIndex");
            skipRows = 0;
            retrunRows = -1;

            DBCursor cursor = cl.QueryAndRemove(query, selector, orderBy, hint, skipRows, retrunRows, DBQuery.FLG_QUERY_PARALLED);
            while (cursor.Next() != null)
            {
            }
            cursor.Close();;
            Assert.AreEqual(0, cl.GetCount(null));
        }

        public void TestQueryAndRemoveWithReturn()
        {
            if (cs.IsCollectionExist(clName))
            {
                cs.DropCollection(clName);
            }
            cl = cs.CreateCollection(clName);
            List<BsonDocument> insertRecords = InsertDatas(10);
            long skipRows, retrunRows;
            skipRows = 0;
            retrunRows = 1;

            DBCursor cursor = cl.QueryAndRemove(null, null, null, null, skipRows, retrunRows, DBQuery.FLG_QUERY_WITH_RETURNDATA);
            while (cursor.Next() != null)
            {
                Console.WriteLine(cursor.Current().ToString());
            }
            cursor.Close(); ;
            BsonDocument matcher = new BsonDocument();
            BsonDocument condition = new BsonDocument();
            condition.Add("$gte", 1);
            condition.Add("$lte", 9);
            matcher.Add("_id", condition);
            Assert.AreEqual(9, cl.GetCount(matcher));
        }


        [TestCleanup()]
        public void TearDown()
        {
            try
            {
               cs.DropCollection(clName);
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
                Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " end: " + this.GetType().ToString());
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
                obj.Add("score", 90+i*0.1);
                obj.Add("height", 165+i);
                dataList.Add(obj);
            }
            cl.BulkInsert(dataList, 0);
            return dataList;
        }
    }
}
