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
     *               SplitAsync(String sourceGroupName, String destGroupName, BsonDocument splitCondition, BsonDocument splitEndCondition)
     *               1、创建cl，其中分区方式设置为range 
     *               2、插入数据，覆盖所有数据类型（包括整型、浮点数、字符串、OID、bool、date、timestamp、二进制、正则表达式、对象、数组） 
     *               3、执行splitAsync，设置范围切分条件 
     *               4、查看数据切分结果，执行find/count查询数据 
     * testcase:    14554
     * author:      chensiqin
     * date:        2018/05/03
    */

    [TestClass]
    public class TestSplit14554
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private string clName = "cl14554";
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
        public void Test14554()
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
            BsonDocument splitCondition = new BsonDocument();
            BsonDocument splitEndCondition = new BsonDocument();
            splitCondition.Add("a", 0);
            splitEndCondition.Add("a", 500);
            long taskId = cl.SplitAsync(dataGroupNames[0], dataGroupNames[1], splitCondition, splitEndCondition);
            List<long> tasks = new List<long>();
            tasks.Add(taskId);
            sdb.WaitTasks(tasks);//等待异步切分任务完成后再校验结果
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
            Assert.AreEqual(8, actual, "SrcDataGroup data count:" + actual);
        }

        private void CheckDestDataGroup()
        {
            Sequoiadb destDataNode = sdb.GetReplicaGroup(dataGroupNames[1]).GetMaster().Connect();
            DBCollection localCL = destDataNode.GetCollectionSpace(SdbTestBase.csName).GetCollection(clName);
            long actual = localCL.GetCount(null);
            Assert.AreEqual(4, actual, "DestDataGroup data count:" + actual);
        }

        private void InsertData()
        {
            //插入数据，覆盖多种数据类型（如整型、浮点数、字符串、OID、bool、date、timestamp、二进制、正则表达式、对象、数组），且插入数据中存在多条大记录 
            list.Add(new BsonDocument("a", 456));//整形
            list.Add(new BsonDocument("a", 3.1415));//浮点数
            list.Add(new BsonDocument("a", 3.1415926));//浮点数
            list.Add(new BsonDocument("a", "test14544"));//字符串
            list.Add(new BsonDocument("_id", ObjectId.GenerateNewId()));//OID
            list.Add(new BsonDocument("a", false));//bool
            list.Add(new BsonDocument("a", new DateTime()));//date
            list.Add(new BsonDocument("a", new BsonTimestamp(1000000000L)));//timestamp
            byte[] buf = new byte[100];
            for (int i = 0; i < 100; i++)
            {
                buf[i] = 65;
            }
            list.Add(new BsonDocument("a", BsonBinaryData.Create(buf)));//二进制
            list.Add(new BsonDocument("a", BsonRegularExpression.Create("/.*test.*/")));//正则表达式
            list.Add(new BsonDocument("a", new BsonDocument { { "type", "test14554" } }));//对象
            BsonArray arr = new BsonArray();
            arr.Add(2);
            list.Add(new BsonDocument("a", arr));//数组
            cl.BulkInsert(list, SDBConst.FLG_INSERT_CONTONDUP);
        }
    }
}
