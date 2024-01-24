using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB;
using CSharp.TestCommon;
using SequoiaDB.Bson;

namespace CSharp.AutoIncrement
{
    /**
     * @Description seqDB-24419:Sequence相关接口测试
     * @Author liuli
     * @Date 2021.10.12
     * @UpdatreAuthor liuli
     * @UpdateDate 2021.10.12
     * @version 1.00
    */

    [TestClass]
    public class Sequoiace24119
    {
        private Sequoiadb sdb = null;
        private DBSequence sequence = null;
        private string sequoiaceName = "sequoiace24419";
        private string sequoiaceNewName = "sequoiace24419new";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
        }

        [TestMethod]
        public void Test24119()
        {
            // 创建Sequence
            sequence = sdb.CreateSequence(sequoiaceName);
            sdb.GetSequence(sequoiaceName);

            // 删除Sequence
            sdb.DropSequence(sequoiaceName);

            // 指定参数创建Sequence
            BsonDocument option1 = new BsonDocument();
            option1.Add("StartValue", 1);
            option1.Add("MinValue", 1);
            option1.Add("MaxValue", 200000);
            option1.Add("Increment", 1);
            option1.Add("CacheSize", 1000);
            option1.Add("AcquireSize", 1000);
            option1.Add("Cycled", false);
            sdb.CreateSequence(sequoiaceName, option1);

            // 查看快照
            BsonDocument selector = new BsonDocument();
            selector.Add("StartValue", "");
            selector.Add("MinValue", "");
            selector.Add("MaxValue", "");
            selector.Add("Increment", "");
            selector.Add("CacheSize", "");
            selector.Add("AcquireSize", "");
            selector.Add("Cycled", "");

            DBCursor cursor = null;
            cursor = sdb.GetSnapshot(SDBConst.SDB_SNAP_SEQUENCES, new BsonDocument("Name", sequoiaceName), selector, null);
            Assert.AreEqual(cursor.Current(), option1);

            // Sequence重命名
            sdb.RenameSequence(sequoiaceName, sequoiaceNewName);

            // 获取Sequence
            sequence = sdb.GetSequence(sequoiaceNewName);

            // fetch连续获取10个序列
            BsonDocument act = sequence.Fetch(10);
            BsonDocument exp = new BsonDocument();
            exp.Add("NextValue", 1);
            exp.Add("ReturnNum", 10);
            exp.Add("Increment", 1);
            Assert.AreEqual(act, exp);

            // getCurrentValue获取序列当前值
            long currentValue = sequence.GetCurrentValue();
            Assert.AreEqual(currentValue, 10);

            // getNextValue获取序列的下一个值
            long nexrValue = sequence.GetNextValue();
            Assert.AreEqual(nexrValue, 11);

            // resart设置序列从100开始重新计数
            sequence.Restart(100);
            Assert.AreEqual(sequence.GetNextValue(), 100);

            // setCurrentValue设置序列当前值为1000
            sequence.SetCurrentValue(1000);
            Assert.AreEqual(sequence.GetCurrentValue(), 1000);

            // setAttributes修改序列的属性
            BsonDocument option2 = new BsonDocument();
            option2.Add("AcquireSize", 2000);
            option2.Add("CacheSize", 2000);
            option2.Add("Cycled", false);
            option2.Add("Increment", 10);
            option2.Add("MaxValue", 1000001L);
            option2.Add("MinValue", 10);
            option2.Add("StartValue", 10);
            sequence.SetAttributes(option2);

            // 检测修改结果
            cursor = sdb.GetSnapshot(SDBConst.SDB_SNAP_SEQUENCES, new BsonDocument("Name", sequoiaceNewName), selector, null);
            Assert.AreEqual(cursor.Current(), option2);
        }

        [TestCleanup()]
        public void TearDown()
        {
            sdb.DropSequence(sequoiaceNewName);
            if (sdb != null)
            {
                sdb.Disconnect();
            }
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " end  : " + this.GetType().ToString());
        }
    }
}