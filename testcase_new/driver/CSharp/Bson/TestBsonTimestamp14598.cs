using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB.Bson;
using SequoiaDB;
using CSharp.TestCommon;

namespace CSharp.Bson
{
    /**
     * description:  
     *                  BSONTimestamp (int time, int inc)   BSONTimestamp (long value)
     *                 1.插入一条timestamp类型的数据，使用BSONTimestamp (int time, int inc)接口，分别验证timestamp和increment参数合法值和非法值：
     *                 合法值：timestamp：1902-01-01-00.00.00.000000~2037-12-31-23.59.59.999999直接的时间，覆盖边界值
     *                 increment：【0,14】之间的整数，覆盖边界值
     *                 非法值：
     *                 a、超过边界值
     *                 b、非int/long类型，如string类型“a01”
     *                 2.查询timestamp数据的值，检查是否正确
     * testcase:    14598
     * author:      chensiqin
     * date:        2019/03/06
    */

    [TestClass]
    public class TestBsonTimestamp14598
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private string clName = "cl14598";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);
        }

        [TestMethod]
        public void Test14598()
        {
            if (cs.IsCollectionExist(clName))
            {
                cs.DropCollection(clName);
            }
            cl = cs.CreateCollection(clName);

            int maxIntSec = 2147483647;
            int minIntSec = -2147483648;
            int secmin = 0;
            int secmax = 999999;
            long timestamp1 = ((long)maxIntSec << 32) + 0;
            long timestamp2 = ((long)minIntSec << 32) + 0;
            BsonDocument insertor1 = new BsonDocument { { "a", new BsonTimestamp(timestamp1) }, { "b", 1 } };
            BsonDocument insertor2 = new BsonDocument { { "a", new BsonTimestamp(timestamp2) }, { "b", 2 } };
            BsonDocument insertor3 = new BsonDocument { { "a", new BsonTimestamp(maxIntSec, secmax) }, { "b", 3 } };
            BsonDocument insertor4 = new BsonDocument { { "a", new BsonTimestamp(maxIntSec, secmin) }, { "b", 4 } };
            BsonDocument insertor5 = new BsonDocument { { "a", new BsonTimestamp(minIntSec, secmax) }, { "b", 5 } };
            BsonDocument insertor6 = new BsonDocument { { "a", new BsonTimestamp(minIntSec, secmin) }, { "b", 6 } };
            BsonDocument insertor7 = new BsonDocument { { "a", new BsonTimestamp(minIntSec, -1) }, { "b", 7 } };
            BsonDocument insertor8 = new BsonDocument { { "a", new BsonTimestamp(minIntSec, 1000000) }, { "b", 8 } };
            BsonDocument insertor9 = new BsonDocument { { "a", new BsonTimestamp(maxIntSec, -1) }, { "b", 9 } };
            BsonDocument insertor10 = new BsonDocument { { "a", new BsonTimestamp(maxIntSec, 1000000) }, { "b", 10 } };
            cl.Insert(insertor1);
            cl.Insert(insertor2);
            cl.Insert(insertor3);
            cl.Insert(insertor4);
            cl.Insert(insertor5);
            cl.Insert(insertor6);
            cl.Insert(insertor7);
            cl.Insert(insertor8);
            cl.Insert(insertor9);
            cl.Insert(insertor10);
            DBCursor cursor = cl.Query();
            BsonDocument record;
            List<BsonDocument> bsonList = new List<BsonDocument>();
            while ((record = cursor.Next()) != null)
            {
                bsonList.Add(record);
            }

            Assert.AreEqual(maxIntSec, bsonList[0].GetValue("a").AsBsonTimestamp.Timestamp);
            Assert.AreEqual(0, bsonList[0].GetValue("a").AsBsonTimestamp.Increment);

            Assert.AreEqual(minIntSec, bsonList[1].GetValue("a").AsBsonTimestamp.Timestamp);
            Assert.AreEqual(0, bsonList[1].GetValue("a").AsBsonTimestamp.Increment);

            Assert.AreEqual(maxIntSec, bsonList[2].GetValue("a").AsBsonTimestamp.Timestamp);
            Assert.AreEqual(secmax, bsonList[2].GetValue("a").AsBsonTimestamp.Increment);

            Assert.AreEqual(maxIntSec, bsonList[3].GetValue("a").AsBsonTimestamp.Timestamp);
            Assert.AreEqual(secmin, bsonList[3].GetValue("a").AsBsonTimestamp.Increment);

            Assert.AreEqual(minIntSec, bsonList[4].GetValue("a").AsBsonTimestamp.Timestamp);
            Assert.AreEqual(secmax, bsonList[4].GetValue("a").AsBsonTimestamp.Increment);

            Assert.AreEqual(minIntSec, bsonList[5].GetValue("a").AsBsonTimestamp.Timestamp);
            Assert.AreEqual(secmin, bsonList[5].GetValue("a").AsBsonTimestamp.Increment);

            Assert.AreEqual(minIntSec - 1, bsonList[6].GetValue("a").AsBsonTimestamp.Timestamp);
            Assert.AreEqual(999999, bsonList[6].GetValue("a").AsBsonTimestamp.Increment);

            Assert.AreEqual(minIntSec + 1, bsonList[7].GetValue("a").AsBsonTimestamp.Timestamp);
            Assert.AreEqual(0, bsonList[7].GetValue("a").AsBsonTimestamp.Increment);

            Assert.AreEqual(maxIntSec - 1, bsonList[8].GetValue("a").AsBsonTimestamp.Timestamp);
            Assert.AreEqual(999999, bsonList[8].GetValue("a").AsBsonTimestamp.Increment);

            Assert.AreEqual(maxIntSec + 1, bsonList[9].GetValue("a").AsBsonTimestamp.Timestamp);
            Assert.AreEqual(0, bsonList[9].GetValue("a").AsBsonTimestamp.Increment);
            

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
