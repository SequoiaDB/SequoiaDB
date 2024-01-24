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
     *               1、创建cl，其中分区方式为range，指定多个分区键（如3个分区键） 
     *               2、执行split，设置切分范围，其中范围切分条件字段名为分区键 
     *               3、插入数据，覆盖多种数据类型（包括整型、浮点数、字符串、OID、bool、date、timestamp、二进制、正则表达式、对象、数组，其中分区键对应字段值乱序，且记录中字段无序） 
     *               4、查看插入是否成功，执行find/count查询数据 
     * testcase:    14553
     * author:      chensiqin
     * date:        2018/05/04
    */
    [TestClass]
    public class TestSplit14553
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private string clName = "cl14553";
        private List<string> dataGroupNames = new List<string>();
        List<BsonDocument> list = new List<BsonDocument>();

        //[TestInitialize()]
        [Ignore]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
        }

        //[TestMethod]
        [Ignore]
        public void Test14553()
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
            option.Add("ShardingKey", new BsonDocument { { "a", 1 },{"b", -1}, {"c", 1} });
            option.Add("ShardingType", "range");
            cl = cs.CreateCollection(clName, option);
            InsertData();
            BsonDocument splitCondition = new BsonDocument();
            BsonDocument splitEndCondition = new BsonDocument();
            splitCondition.Add("a", 25);
            splitCondition.Add("b", 0.8);
            splitCondition.Add("c", "test145225");
            splitEndCondition.Add("a", 110);
            splitEndCondition.Add("b", 0.1);
            splitEndCondition.Add("c", "test145226");
            cl.Split(dataGroupNames[0], dataGroupNames[1], splitCondition, splitEndCondition);
            CheckCoord();
            CheckSrcDataGroup();
            CheckDestDataGroup();
            cs.DropCollection(clName);
        }

        private void CheckCoord()
        {
            BsonDocument matcher = new BsonDocument();
            long re = cl.GetCount(matcher);
            Assert.AreEqual(100, re);
        }

        private void CheckSrcDataGroup()
        {
            Sequoiadb srcDataNode = sdb.GetReplicaGroup(dataGroupNames[0]).GetMaster().Connect();
            DBCollection localCL = srcDataNode.GetCollectionSpace(SdbTestBase.csName).GetCollection(clName);
            BsonDocument matcher = new BsonDocument();
            matcher.Add("a", new BsonDocument { { "$gte", 0 }, { "$lt", 25 } });
            long actual = localCL.GetCount(matcher);
            Assert.AreEqual(25, actual, "SrcDataGroup data count:" + actual);
        }

        private void CheckDestDataGroup()
        {
            Sequoiadb destDataNode = sdb.GetReplicaGroup(dataGroupNames[1]).GetMaster().Connect();
            DBCollection localCL = destDataNode.GetCollectionSpace(SdbTestBase.csName).GetCollection(clName);
            BsonDocument matcher = new BsonDocument();
            matcher.Add("a", new BsonDocument { { "$gte", 25 }, { "$lt", 100 } });
            long actual = localCL.GetCount(null);
            Assert.AreEqual(75, actual, "DestDataGroup data count:" + actual);
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
            for (int i = 0; i < 100; i++)
            {
                Random rand = new Random();
                doc = new BsonDocument();
                doc.Add("a", i);//整形
                doc.Add("b", rand.NextDouble());//浮点数
                doc.Add("c", "test14522"+i);//字符串
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

        //[TestCleanup()]
        [Ignore]
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
