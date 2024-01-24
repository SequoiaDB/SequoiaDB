using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB.Bson;
using SequoiaDB;
using CSharp.TestCommon;

namespace CSharp.Index
{
    /**
     * description: createIdIndex (BsonDocument options)；dropIdIndex ()；GetIndex（string name）
     *              1、向cl中插入数据（cl中不存在Id索引） 
     *              2、创建ID索引，查询Id索引（GetIndex） 
     *              3、查询访问计划，查看是否走索引扫描 
     *              4、按指定的ID索引查询，指定查询条件，查看返回的结果是否正确 
     *              5、删除索引dropIdIndex() 
     *              6、向cl插入数据，查看数据插入情况  
     * testcase:    14546
     * author:      chensiqin
     * date:        2019/04/1
     */

    [TestClass]
    public class TestIndex14546
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private string clName = "cl14546";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
        }

        [TestMethod]
        public void Test14546()
        {
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);
            if (cs.IsCollectionExist(clName))
            {
                cs.DropCollection(clName);
            }
            cl = cs.CreateCollection(clName, new BsonDocument("AutoIndexId",false));

            //insert and check
            List<BsonDocument> insertData = new List<BsonDocument>();
            int dataNum = 50;
            for (int i = 0; i < dataNum; i++)
            {
                BsonDocument doc = new BsonDocument();
                doc.Add("_id", i);
                doc.Add("a", i);
                insertData.Add(doc);
            }
            cl.BulkInsert(insertData, 0);

            //can't update when has no id index
            BsonDocument modifier = new BsonDocument();
            modifier.Add("$set", new BsonDocument("a", 2));
            try
            {
                cl.Update(new BsonDocument("a", 1), modifier, null);
                Assert.Fail("expect can't update when has no id index!");
            }
            catch (BaseException e)
            {
                Assert.AreEqual(-279, e.ErrorCode);
            }
           
            //create id index
            cl.CreateIdIndex(new BsonDocument("SortBufferSize", 64));

            DBCursor cur = cl.GetIndex("$id");
            while (cur.Next() != null)
            {
                BsonDocument doc = cur.Current();
                BsonDocument indexDefInfo = doc.GetElement("IndexDef").Value.ToBsonDocument();
                Assert.AreEqual(true, indexDefInfo.GetElement("unique").Value.ToBoolean());
                Assert.AreEqual(true, indexDefInfo.GetElement("enforced").Value.ToBoolean());
                Assert.AreEqual("{ \"_id\" : 1 }", indexDefInfo.GetElement("key").Value.ToString());
            }
            cur.Close();
            //id index query
            BsonDocument matcher = new BsonDocument("_id", new BsonDocument("$lt", 5));
            BsonDocument order = new BsonDocument("_id", 1);

            cur = cl.Query(matcher, null, order, null);
            int num = 0;
            while (cur.Next() != null)
            {
                BsonDocument doc = cur.Current();
                Assert.AreEqual(insertData[num++].ToString(), doc.ToString());
            }
            cur.Close();
            Assert.AreEqual(5, num);

            //check explain
            cur = cl.Explain(matcher, null, order, null, 0, -1, 0, new BsonDocument("Run", true));
            while (cur.Next() != null)
            {
                BsonDocument doc = cur.Current();
                Assert.AreEqual("ixscan", doc.GetElement("ScanType").Value.ToString());
                Assert.AreEqual("$id", doc.GetElement("IndexName").Value.ToString());
            }
            cur.Close();

            //drop id index
            cl.DropIdIndex();
            cur = cl.GetIndexes();
            Assert.IsNull(cur.Next());
            cl.Insert(new BsonDocument("name", 14546));
        }

        [TestCleanup()]
        public void TearDown()
        {
            if (cs.IsCollectionExist(clName))
            {
                cs.DropCollection(clName);
            }
            if (sdb != null)
            {
                sdb.Disconnect();
            }
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " end  : " + this.GetType().ToString());
        }
    }
}
