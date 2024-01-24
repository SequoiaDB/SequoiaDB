using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB.Bson;
using SequoiaDB;
using CSharp.TestCommon;

namespace CSharp.Split
{
    /**
     * description:  
     *               split (String sourceGroupName, String destGroupName, BsonDocument splitCondition, BsonDocument splitEndCondition)
     *               1、创建cl，其中分区方式设置为hash 
     *               2、插入数据（包括记录和lob对象，数据记录覆盖多种数据类型：如数组、对象、日期、时间、字符串等） 
     *               3、执行split，设置范围切分条件 
     *               4、查看数据切分结果，执行find/count查询数据 
     * testcase:    14551
     * author:      chensiqin
     * date:        2018/05/03
    */

    [TestClass]
    public class TestSplit14551
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private string clName = "cl14551";
        private List<string> dataGroupNames = new List<string>();
        List<BsonDocument> list = new List<BsonDocument>();
        private List<ObjectId> InsertedLobId = new List<ObjectId>();// 记录所有已插入的LOBID字串
        private ObjectId id;

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
        }

        [TestMethod]
        public void Test14551()
        {
            if (Common.isStandalone(sdb))
            {
                return;
            }
            dataGroupNames = Common.getDataGroupNames(sdb);
            if (dataGroupNames.Count < 2)
            {
                return;
            }
            cs = sdb.GetCollectionSpace(SdbTestBase.csName); 
            BsonDocument option = new BsonDocument();
            option.Add("Group", dataGroupNames[0]);
            option.Add("ShardingKey", new BsonDocument { { "a", 1 } });
            option.Add("ShardingType", "hash");
            option.Add("Partition", 4096);
            cl = cs.CreateCollection(clName, option);
            InsertData();
            //执行split，设置范围切分条件 
            BsonDocument splitCondition = new BsonDocument();
            BsonDocument splitEndCondition = new BsonDocument();
            splitCondition.Add("Partition", 2048);
            splitEndCondition.Add("Partition", 4096);
            cl.Split(dataGroupNames[0], dataGroupNames[1], splitCondition, splitEndCondition);
            //check result
            CheckLob();
            CheckCoord();
            CheckDataGroup();
            cl.RemoveLob(id);
            cs.DropCollection(clName);
        }

        private void CheckCoord()
        {
            BsonDocument matcher = new BsonDocument();
            long re = cl.GetCount(matcher);
            Assert.AreEqual(1200, re);
        }

        private void CheckLob()
        {
            DBCursor cur = cl.ListLobs();
            while (cur.Next() != null)
            {
                BsonDocument doc = cur.Current();
                Console.WriteLine(doc.ToString());
                Console.WriteLine(doc.GetElement("Oid").Value.ToString());
                Assert.AreEqual(id.ToString(), doc.GetElement("Oid").Value.ToString());
            }
        }

        private void CheckDataGroup()
        {
            Sequoiadb srcDataNode = sdb.GetReplicaGroup(dataGroupNames[0]).GetMaster().Connect();
            DBCollection localCL = srcDataNode.GetCollectionSpace(SdbTestBase.csName).GetCollection(clName);
            long srcActual = localCL.GetCount(null);

            Sequoiadb destDataNode = sdb.GetReplicaGroup(dataGroupNames[1]).GetMaster().Connect();
            localCL = destDataNode.GetCollectionSpace(SdbTestBase.csName).GetCollection(clName);
            long destActual = localCL.GetCount(null);
            Assert.AreEqual(1200, srcActual + destActual);
        }

        private void InsertData()
        {
            //插入数据（包括记录和lob对象，数据记录覆盖多种数据类型：如数组、对象、日期、时间、字符串等）
            BsonArray arr = new BsonArray();
            for (int i = 0; i < 200; i++)
            {
                list.Add(new BsonDocument("a", i*55));
                arr = new BsonArray();
                arr.Add(i * 1);
                list.Add(new BsonDocument("a", arr));
                list.Add(new BsonDocument("a", new BsonDocument { { "type", "test"+i } }));
                list.Add(new BsonDocument("a", new DateTime()));//date
                list.Add(new BsonDocument("a", new BsonTimestamp(1000000000L)));//timestamp
                list.Add(new BsonDocument("a", "testsplit14551"+i));
            }
            
            DBLob lob = cl.CreateLob();
            id = lob.GetID();
            byte[] buf = new byte[100];
            for (int i = 0; i < 100; i++)
            {
                buf[i] = 65;
            }
            lob.Write(buf);
            lob.Close();
            cl.BulkInsert(list, SDBConst.FLG_INSERT_CONTONDUP);

        }

        [TestCleanup()]
        public void TearDown()
        {
            if (sdb != null)
            {
                sdb.Disconnect();
            }
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " end  : " + this.GetType().ToString());
        }
    }
}
