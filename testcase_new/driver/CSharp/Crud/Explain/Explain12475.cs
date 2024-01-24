using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB;
using CSharp.TestCommon;
using SequoiaDB.Bson;

namespace CSharp.Crud.Explain
{
    /**
      * description: explain
      *              Explain (BsonDocument query, BsonDocument selector, BsonDocument orderBy, BsonDocument hint, long skipRows, long returnRows, int flag, BsonDocumentoptions)
      * testcase:    12475 
      * author:      chensiqin
      * date:        2018/3/16
     */
    [TestClass]
    public class Explain12475
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private string clName = "cl12475";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);

        }

        [TestMethod()]
        public void TestDelete12475()
        {
            BsonDocument key = new BsonDocument();
            key.Add("age", 1);
            cl = cs.CreateCollection(clName);
            cl.CreateIndex("ageIndex", key, true, true);
            List<BsonDocument> insertRecords = InsertDatas(20);
            CheckExplainWithFlag(DBQuery.FLG_QUERY_FORCE_HINT);
            CheckExplainWithFlag(DBQuery.FLG_QUERY_PARALLED);
            CheckExplainWithFlag(DBQuery.FLG_QUERY_WITH_RETURNDATA);
        }

        public void CheckExplainWithFlag(int flag)
        {
            
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
            hint.Add("", "ageIndex");
            skipRows = 2;
            retrunRows = 5;

            DBCursor cursor = cl.Explain(query, selector, orderBy, hint, skipRows, retrunRows, flag, new BsonDocument().Add("Run", true));
            while (cursor.Next() != null)
            {
                BsonDocument doc = cursor.Current();
                Assert.AreEqual("ixscan", doc.GetValue("ScanType"));
                Assert.AreEqual(5, doc.GetValue("ReturnNum"));
                Assert.AreEqual("ageIndex", doc.GetValue("IndexName"));
                Assert.AreEqual("{ \"$and\" : [{ \"sub\" : { \"$et\" : \"test\" } }] }", doc.GetValue("Query").ToString());

            }
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
