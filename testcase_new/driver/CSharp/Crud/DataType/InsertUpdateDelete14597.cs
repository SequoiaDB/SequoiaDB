using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB;
using CSharp.TestCommon;
using SequoiaDB.Bson;

namespace CSharp.Crud.DataType
{

    /**
      * description: insert/update/delete type
      *              insert int data ,then update and delete
      * testcase:    14597
      * author:      chensiqin
      * date:        2018/3/16
    */
    [TestClass]
    public class InsertUpdateDelete14597
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private string clName = "cl14597";

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
        public void TestDelete14597()
        {
            List<BsonDocument> insertRecords = InsertDatas(10);
            BsonDocument modifer = new BsonDocument();
            BsonDocument matcher = new BsonDocument();
            modifer.Add("$inc", new BsonDocument().Add("inttype", 1));
            cl.Update(null, modifer, null, 0);
            DBCursor cursor = cl.Query(matcher.Add("inttype", new BsonDocument().Add("$et", 1)), null, null, null);
            int i = 0;
            while (cursor.Next() != null)
            {
                i++;
            }
            Assert.AreEqual(10, i);
            cursor.Close();

            matcher = new BsonDocument();
            cursor = cl.Query(matcher.Add("inttype", new BsonDocument().Add("$et", 21474836490)), null, null, null);
            while (cursor.Next() != null)
            {
                Assert.AreEqual("SequoiaDB.Bson.BsonInt64", cursor.Current().GetValue("inttype").GetType().ToString());
            }
            cursor.Close();

            matcher = new BsonDocument();
            BsonDocument condition = new BsonDocument();
            condition.Add("$et", -2147483647);
            Assert.AreEqual(1, cl.GetCount(matcher.Add("inttype", condition)));

            matcher = new BsonDocument();
            Assert.AreEqual(1, cl.GetCount(matcher.Add("inttype", new BsonDocument().Add("$et", 2147483648))));

            matcher = new BsonDocument();
            Assert.AreEqual(1, cl.GetCount(matcher.Add("inttype", new BsonDocument().Add("$et", 21474836490))));

            matcher = new BsonDocument();
            cl.Delete(matcher.Add("inttype", new BsonDocument().Add("$et", 21474836490)), null);
            matcher = new BsonDocument();
            Assert.AreEqual(0, cl.GetCount(matcher.Add("inttype", new BsonDocument().Add("$et", 21474836490))));
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
            BsonDocument obj = null;
            for (int i = 0; i < len; i++)
            {
                obj = new BsonDocument();
                obj.Add("age", i);
                obj.Add("sub", "test");
                dataList.Add(obj);
            }
            dataList.Add(new BsonDocument().Add("inttype", -2147483648));
            dataList.Add(new BsonDocument().Add("inttype", 2147483647));
            dataList.Add(new BsonDocument().Add("inttype", 21474836489));
            cl.BulkInsert(dataList,0);
            return dataList;
        }
    }
}
