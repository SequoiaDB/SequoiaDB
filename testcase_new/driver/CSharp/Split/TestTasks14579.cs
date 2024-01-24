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
     *               ListTasks (BsonDocument matcher,BsonDocument selector,BsonDocument orderBy,BsonDocument hint)
     *               CancelTask(long taskIDs,bool isAsync )；WaitTasks(List< long > taskIDs)
     *                1、列出当前执行的任务，如切分任务；分别覆盖如下场景：
     *                a、列出所有任务，listTask不带参数
     *                b、指定条件列出任务，listTask覆盖所有参数
     *                2、等待任务，分别验证如下情况：
     *                a、等待一个任务
     *                b、等待多个任务
     *                3、取消正在执行的任务，分别验证如下情况：
     *                a、只指定任务id参数
     *                b、指定任务id、isAsync参数
     *                4、检查操作结果
     *                2.在执行切分的过程中取消任务 3.在执行切分的过程中等待任务
     *                4、取消一个不存在的任务
     * testcase:    14579
     * author:      chensiqin
     * date:        2018/05/04
    */

    [TestClass]
    public class TestTasks14579
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private string localCSName = "cs14579";
        private string clName = "cl14579";
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
        public void Test14579()
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
            if (sdb.IsCollectionSpaceExist(localCSName))
            {
                sdb.DropCollectionSpace(localCSName);
            }
            cs = sdb.CreateCollectionSpace(localCSName);
            BsonDocument option = new BsonDocument();
            option.Add("Group", dataGroupNames[0]);
            option.Add("ShardingKey", new BsonDocument { { "b", 1 } });
            option.Add("ShardingType", "range");
            cl = cs.CreateCollection(clName, option);
            //wait tasks
            InsertData();
            long taskId = cl.SplitAsync(dataGroupNames[0], dataGroupNames[1], 50);
            List<long> tasks = new List<long>();
            tasks.Add(taskId);
            sdb.WaitTasks(tasks);
            cs.DropCollection(clName);

            option = new BsonDocument();
            option.Add("Group", dataGroupNames[0]);
            option.Add("ShardingKey", new BsonDocument { { "a", 1 } });
            option.Add("ShardingType", "range");
            cl = cs.CreateCollection(clName, option);
            InsertData();
            BsonDocument splitCondition = new BsonDocument();
            BsonDocument splitEndCondition = new BsonDocument();
            splitCondition.Add("a", 50);
            splitEndCondition.Add("a", 10000);
            taskId = cl.SplitAsync(dataGroupNames[0], dataGroupNames[1], splitCondition, splitEndCondition);
            //ListTasks 无参
            DBCursor cur = sdb.ListTasks(null, null, null, null);
            int count = 0;
            while (cur.Next() != null)
            {
                cur.Current();
                count++;
            }
            cur.Close();
            Assert.IsTrue(count >= 1);
            //ListTasks有参
            BsonDocument matcher = new BsonDocument();
            matcher.Add("Name", localCSName + "." + clName);
            matcher.Add("ShardingType", "range");
            matcher.Add("Source", dataGroupNames[0]);
            matcher.Add("Target", dataGroupNames[1]);
            cur = sdb.ListTasks(matcher, new BsonDocument("TaskID", 1), new BsonDocument("TaskID", 1), null);
            count = 0;
            while (cur.Next() != null)
            {
                cur.Current();
                count++;
            }
            cur.Close();
            Assert.IsTrue(count > 0);

            try
            {
                sdb.CancelTask(taskId, true);
            }
            catch (BaseException e)
            {
                if (e.ErrorCode != -219)
                {
                    throw e;
                }
            }

            sdb.DropCollectionSpace(localCSName);

            cur = sdb.ListTasks(new BsonDocument("TaskID", taskId), null, null, null);
            Assert.AreEqual(null, cur.Next());

            try
            {
                sdb.CancelTask(taskId, true);
                Assert.Fail("expected CancelTask throw baseexception, but success!");
            }
            catch (BaseException e)
            {
                Assert.AreEqual(-173, e.ErrorCode);
            }
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
            arr.Add(2);
            for (int i = 0; i < 10; i++)
            {
                Random rand = new Random();
                doc = new BsonDocument();
                doc.Add("a", arr);//整形
                doc.Add("b", rand.NextDouble());//浮点数
                doc.Add("c", "test14522" + i);//字符串
                doc.Add("_id", ObjectId.GenerateNewId());//OID
                doc.Add("e", true);//bool
                doc.Add("f", new DateTime());//date
                doc.Add("g", new BsonTimestamp(1000000000L));//timestamp
                doc.Add("h", BsonBinaryData.Create(buf));//二进制
                doc.Add("i", BsonRegularExpression.Create("/.*foo.*/"));//正则表达式
                doc.Add("j", new BsonDocument { { "type", "test" } });//对象
                doc.Add("k", i);//数组
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
