using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB.Bson;
using SequoiaDB;
using CSharp.TestCommon;

namespace CSharp.Transaction
{

    /**
     * description:    	  	TransactionBegin ()；commit ()
     *                      1、执行开启事务操作
     *                      2、执行事务操作：依次执行插入、更新、删除数据，分别更新、删除指定范围内数据 
     *                      3、提交事务 
     *                      4、查看事务处理结果 
     * testcase:         14565
     * author:           chensiqin
     * date:             2019/04/19
     */
    [TestClass]
    public class TestTransaction14565
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private string clName = "cl14565";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
        }

        [TestMethod]
        public void Test14565()
        {
            sdb.TransactionBegin();
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);
            if (cs.IsCollectionExist(clName))
            {
                cs.DropCollection(clName);
            }
            cl = cs.CreateCollection(clName);
            List<BsonDocument> datas = new List<BsonDocument>();
            for (int i = 1; i <= 100; i++)
            {
                BsonDocument doc = new BsonDocument();
                doc.Add("_id", i);
                doc.Add("number", i);
                doc.Add("name", "sequoiadb" + i);
                datas.Add(doc);
            }
            cl.BulkInsert(datas, 0);
            BsonDocument matcher = new BsonDocument();
            BsonArray arr = new BsonArray();
            arr.Add(new BsonDocument("number", new BsonDocument("$gte", 5)));
            arr.Add(new BsonDocument("number", new BsonDocument("$lt", 10)));
            matcher.Add("$and", arr);

            BsonDocument modifier = new BsonDocument();
            modifier.Add("$inc", new BsonDocument("number", 100));
            cl.Update(matcher, modifier, null);

            matcher = new BsonDocument();
            matcher.Add("number", new BsonDocument("$lt", 5));
            cl.Delete(matcher, null);
            sdb.TransactionCommit();
            matcher = new BsonDocument();
            matcher.Add("number", new BsonDocument("$gte", 10));
            Assert.AreEqual(96, cl.GetCount(matcher));
            matcher = new BsonDocument();
            matcher.Add("number", new BsonDocument("$lt", 10));
            Assert.AreEqual(0, cl.GetCount(matcher));
        }

        [TestCleanup()]
        public void TearDown()
        {
            try
            {
                
                cs = sdb.GetCollectionSpace(SdbTestBase.csName);
                if (cs.IsCollectionExist(clName))
                {
                    cs.DropCollection(clName);
                }
            }
            catch (BaseException e)
            {
                Assert.Fail(e.Message);
            }
            finally
            {
                if (sdb != null)
                {
                    sdb.Disconnect();
                }
            }
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " end  : " + this.GetType().ToString());
        } 
    }
}
