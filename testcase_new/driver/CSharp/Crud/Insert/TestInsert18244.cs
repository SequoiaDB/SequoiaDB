using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB.Bson;
using SequoiaDB;
using CSharp.TestCommon;

namespace CSharp.Crud.Insert
{
    /**
     * description:  Insert ( List< BsonDocument > recordList,int flags )， Insert ( BsonDocument  record,int flags )
     *  	         1.创建cs，cl，指定字段创建索引
     *  	         2.插入记录，包含索引字段和非索引字段
     *  	         3.insert指定flag为FLG_INSERT_REPLACEONDUP类型插入索引字段值相同非索引字段不同的记录
     * testcase:    18244
     * author:      chensiqin
     * date:        2019/04/11
     */

    [TestClass]
    public class TestInsert18244
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private string clName = "cl18244";
        private string indexName = "index18244";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
        }

        [TestMethod]
        public void Test18244()
        {
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);
            if (cs.IsCollectionExist(clName))
            {
                cs.DropCollection(clName);
            }
            cl = cs.CreateCollection(clName);
            cl.CreateIndex(indexName, new BsonDocument("a", 1), true, true);
            BsonDocument record = new BsonDocument();
            record.Add("_id", 1);
            record.Add("a", 1);
            record.Add("b", 1);
            cl.Insert(record);
            DBCursor cur =  cl.Query();
            BsonDocument act = cur.Next();
            Console.WriteLine(act.ToString());
            cur.Close();
            Assert.AreEqual(record.GetElement("a").Value, act.GetElement("a").Value);
            Assert.AreEqual(record.GetElement("b").Value, act.GetElement("b").Value);

            record = new BsonDocument();
            record.Add("_id", 2);
            record.Add("a", 1);
            record.Add("b", 2);
            
            cl.Insert(record, SDBConst.FLG_INSERT_REPLACEONDUP);
            cur = cl.Query();
            act = cur.Next();
            Console.WriteLine(act.ToString());
            cur.Close();
            Assert.AreEqual(record.GetElement("a").Value, act.GetElement("a").Value);
            Assert.AreEqual(record.GetElement("b").Value, act.GetElement("b").Value);
        }

        [TestCleanup()]
        public void TearDown()
        {
            if(cs.IsCollectionExist(clName))
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
