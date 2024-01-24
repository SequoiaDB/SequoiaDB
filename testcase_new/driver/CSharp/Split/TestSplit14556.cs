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
     *               Split(String sourceGroupName, String destGroupName, percent) 
     *               SplitAsync(String sourceGroupName, String destGroupName, BsonDocument splitCondition, BsonDocument splitEndCondition)
     *               1、创建cl，指定分区方式为hash 
     *               2、向cl中插入大量数据 
     *               3、并发执行多个split，其中一个任务按百分比同步切分，另一个任务按异步范围切分 
     *               4、查看数据切分是否正确 
     * testcase:    14556
     * author:      chensiqin
     * date:        2018/05/04
    */

    [TestClass]
    public class TestSplit14556
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private string clName = "cl14556";
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
        public void Test14556()
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
            BsonDocument splitCondition = new BsonDocument();
            BsonDocument splitEndCondition = new BsonDocument();
            splitCondition.Add("Partition", 2048);
            splitEndCondition.Add("Partition", 4096);
            long taskId = -1;
            List<long> tasks = new List<long>();
            try
            {
                taskId = cl.SplitAsync(dataGroupNames[0], dataGroupNames[1], splitCondition, splitEndCondition);
                tasks.Add(taskId);
                cl.Split(dataGroupNames[0], dataGroupNames[1], 50);
            }
            catch (BaseException e)
            {
                Assert.AreEqual(-175, e.ErrorCode);
            }
            finally
            {
                sdb.WaitTasks(tasks);
            }
            CheckCoord();
            CheckDataGroup();
            cs.DropCollection(clName);
        }

        private void CheckCoord()
        {
            BsonDocument matcher = new BsonDocument();
            long re = cl.GetCount(matcher);
            Assert.AreEqual(100000, re);
        }

        private void CheckDataGroup()
        {
            Sequoiadb srcDataNode = sdb.GetReplicaGroup(dataGroupNames[0]).GetMaster().Connect();
            DBCollection localCL = srcDataNode.GetCollectionSpace(SdbTestBase.csName).GetCollection(clName);
            long srcActual = localCL.GetCount(null);

            Sequoiadb destDataNode = sdb.GetReplicaGroup(dataGroupNames[1]).GetMaster().Connect();
            localCL = destDataNode.GetCollectionSpace(SdbTestBase.csName).GetCollection(clName);
            long destActual = localCL.GetCount(null);
            Assert.AreEqual(100000, srcActual + destActual);
            Assert.AreNotEqual(0, srcActual);
            Assert.AreNotEqual(0, destActual);
        }

        private void InsertData()
        {
            //插入数据，覆盖多种数据类型（如整型、浮点数、字符串、OID、bool、date、timestamp、二进制、正则表达式、对象、数组）
            BsonDocument doc = new BsonDocument();
            byte[] buf = new byte[100];
            for (int i = 0; i < 100; i++)
            {
                buf[i] = 65;
            }
            BsonArray arr = new BsonArray();
            arr.Add(1);
            for (int i = 0; i < 100000; i++)
            {
                Random rand = new Random();
                doc = new BsonDocument();
                doc.Add("a", i);//整形
                doc.Add("b", rand.NextDouble());//浮点数
                doc.Add("c", "test14522" + i);//字符串
                doc.Add("_id", ObjectId.GenerateNewId());//OID
                doc.Add("e", true);//bool
                doc.Add("f", new DateTime());//date
                doc.Add("g", new BsonTimestamp(1000000000L));//timestamp
                doc.Add("h", BsonBinaryData.Create(buf));//二进制
                doc.Add("i", BsonRegularExpression.Create("/.*foo.*/"));//正则表达式
                doc.Add("j", new BsonDocument { { "type", "test" } });//对象
                doc.Add("k", arr);//数组
                list.Add(doc);
            }

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
