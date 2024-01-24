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
     *               split (String sourceGroupName, String destGroupName, double percent)
     *               1、创建cl，其中分区方式设置为range 
     *               2、插入数据，覆盖多种数据类型（如整型、浮点数、字符串、OID、bool、date、timestamp、二进制、正则表达式、对象、数组），且插入数据中存在多条大记录 
     *               3、执行split，设置百分比切分条件 4、查看数据切分结果，执行find/count查询数据 
     * testcase:    14552
     * author:      chensiqin
     * date:        2018/05/03
    */

    [TestClass]
    public class TestSplit14552
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private string clName = "cl14552";
        private List<string> dataGroupNames = new List<string>();
        List<BsonDocument> list = new List<BsonDocument>();

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
        }

        [TestMethod]
        public void Test14552()
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
            option.Add("ShardingType", "range");
            cl = cs.CreateCollection(clName, option);
            InsertData();
            cl.Split(dataGroupNames[0], dataGroupNames[1], 50);
            CheckCoord();
            CheckSrcDataGroup();
            CheckDestDataGroup();
            cs.DropCollection(clName);
        }

        private void CheckCoord()
        {
            BsonDocument matcher = new BsonDocument();
            long re = cl.GetCount(matcher);
            Assert.AreEqual(12, re);
        }

        private void CheckSrcDataGroup()
        {
            Sequoiadb srcDataNode = sdb.GetReplicaGroup(dataGroupNames[0]).GetMaster().Connect();
            DBCollection localCL = srcDataNode.GetCollectionSpace(SdbTestBase.csName).GetCollection(clName);
            long actual = localCL.GetCount(null);
            Assert.AreEqual(6, actual,
                    "SrcDataGroup data count:" + actual);
        }

        private void CheckDestDataGroup()
        {
            Sequoiadb destDataNode = sdb.GetReplicaGroup(dataGroupNames[1]).GetMaster().Connect();
            DBCollection localCL = destDataNode.GetCollectionSpace(SdbTestBase.csName).GetCollection(clName);
            long actual = localCL.GetCount(null);
            Assert.AreEqual(6, actual,
                    "DestDataGroup data count:" + actual);
        }

        private void InsertData()
        {
            //插入数据，覆盖多种数据类型（如整型、浮点数、字符串、OID、bool、date、timestamp、二进制、正则表达式、对象、数组），且插入数据中存在多条大记录 
            list.Add(new BsonDocument("a", 123));//整形
            list.Add(new BsonDocument("a", 1.3));//浮点数
            list.Add(new BsonDocument("a", 1.333));//浮点数
            list.Add(new BsonDocument("a", "test14522"));//字符串
            list.Add(new BsonDocument("_id", ObjectId.GenerateNewId()));//OID
            list.Add(new BsonDocument("a", true));//bool
            list.Add(new BsonDocument("a", new DateTime()));//date
            list.Add(new BsonDocument("a", new BsonTimestamp(1000000000L)));//timestamp
            byte[] buf = new byte[100];
            for (int i = 0; i < 100; i++)
            {
                buf[i] = 65;
            }
            list.Add(new BsonDocument("a", BsonBinaryData.Create(buf)));//二进制
            list.Add(new BsonDocument("a", BsonRegularExpression.Create("/.*foo.*/")));//正则表达式
            list.Add(new BsonDocument("a", new BsonDocument { { "type", "test" } }));//对象
            BsonArray arr = new BsonArray();
            arr.Add(1);
            list.Add(new BsonDocument("a", arr));//数组
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
