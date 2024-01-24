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
     *                 Equals (Object obj) Equals（BsonTimestamp rhs）
     *                 1.插入BSONTimestamp类型数据，分别包括相等和不相等数值
     *                 2.对2个数据（两个timestamp比较，timestamp和date比较）使用equals比较(覆盖两种不同的equals接口)，检查结果；
     * testcase:    14602
     * author:      chensiqin
     * date:        2019/03/06
    */

    [TestClass]
    public class TestBsonTimestamp14602
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private string clName = "cl14602";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);
        }

        [TestMethod]
        public void Test14602()
        {
            if (cs.IsCollectionExist(clName))
            {
                cs.DropCollection(clName);
            }
            cl = cs.CreateCollection(clName);

            BsonTimestamp bsonTimetamp1 = new BsonTimestamp(1482396787, 999999);//6366845719861477951
            BsonTimestamp bsonTimetamp2 = new BsonTimestamp(1482396787, 888888);
            BsonDocument record = new BsonDocument();
            record.Add("value1", bsonTimetamp1);
            record.Add("value2", bsonTimetamp1);
            record.Add("value3", bsonTimetamp2);
            cl.Insert(record);

            DBCursor cur = cl.Query();
            while (cur.Next() != null)
            {
                BsonDocument doc = cur.Current();
                Assert.AreEqual(0, doc.GetElement("value1").Value.CompareTo(bsonTimetamp1));
                Assert.AreEqual(0, bsonTimetamp1.CompareTo(doc.GetElement("value1").Value));
                Assert.AreEqual(1, doc.GetElement("value2").Value.CompareTo(new BsonDateTime(6366845719861477951)));

                Assert.AreEqual(true, bsonTimetamp1.Equals(doc.GetElement("value1").Value));
                Assert.AreEqual(false, doc.GetElement("value2").Value.Equals(new BsonDateTime(6366845719861477951)));
            }
            cur.Close();

            Assert.AreEqual(0, bsonTimetamp1.CompareTo(bsonTimetamp1));
            Assert.AreEqual(1, bsonTimetamp1.CompareTo(bsonTimetamp2));
            Assert.AreEqual(1, bsonTimetamp1.CompareTo(new BsonDateTime(6366845719861477951)));


            Assert.AreEqual(true, bsonTimetamp1.Equals(bsonTimetamp1));
            Assert.AreEqual(false, bsonTimetamp1.Equals(bsonTimetamp2));

            Assert.AreEqual(false, bsonTimetamp1.Equals(new BsonDateTime(6366845719861477951)));
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
