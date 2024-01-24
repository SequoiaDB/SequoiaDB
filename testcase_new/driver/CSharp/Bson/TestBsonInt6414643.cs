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
     *                BsonInt64 (int value)         CompareTo (BsonInt64 other)
     *                CompareTo (BsonValue other)   Equals (BsonInt64 rhs)
     *                Equals (object obj)           GetHashCode ()
     *                ToString ()
     *                1.分别测试BsonInt64类中所有接口,构造BsonInt64类型数据（覆盖边界值），插入数据到sdb中
     *                2.使用Compare接口比较两个BsonInt64类型数据（覆盖相同和不同值比较、不同类型相同值比较）
     *                3.使用equals接口比较两个BsonInt64类型数据（覆盖相同和不同值比较、不同类型相同值比较）
     *                4、获取数据的hashCode值
     *                5、执行toString()方法，检查结果
     * testcase:    14643
     * author:      chensiqin54
    */

    [TestClass]
    public class TestBsonInt6414643
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private string clName = "cl14643";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
        }

        [TestMethod]
        public void Test14643()
        {

            prepareData();
            checkData();

            //Compare接口比较两个BsonInt64类型数据（覆盖相同和不同值比较、不同类型相同值比较
            //equals接口比较两个BsonInt64类型数据（覆盖相同和不同值比较、不同类型相同值比较）

            //相同值比较
            BsonInt64 bsonInt641 = new BsonInt64(131L);
            BsonInt64 bsonInt642 = new BsonInt64(131L);
            Assert.AreEqual(0, bsonInt641.CompareTo(bsonInt642));
            Assert.AreEqual(true, bsonInt641.Equals(bsonInt642));

            //获取数据的hashCode值,相同对象hashcode一致，不同对象相同值hashcode值一致
            Assert.AreEqual(bsonInt641.GetHashCode(), bsonInt641.GetHashCode());
            Assert.AreEqual(bsonInt641.GetHashCode(), bsonInt642.GetHashCode());

            bsonInt641 = new BsonInt64(1314L);
            bsonInt642 = new BsonInt64(131L);
            Assert.AreEqual(1, bsonInt641.CompareTo(bsonInt642));
            Assert.AreEqual(false, bsonInt641.Equals(bsonInt642));
            //获取数据的hashCode值,不同对象不同值hashcode不一致
            Assert.AreNotEqual(bsonInt641.GetHashCode(), bsonInt642.GetHashCode());

            //不同类型相同值比较
            bsonInt641 = new BsonInt64(13L);
            Assert.AreEqual(0, bsonInt641.CompareTo(new BsonInt32(13)));
            //TODO:SEQUOIADBMAINSTREAM-4280
            //Assert.AreEqual(true, bsonInt641.Equals(new BsonInt32(13)));

            //执行tostring
            Assert.AreEqual("13", bsonInt641.ToString());
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

        private void prepareData()
        {
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);
            if (cs.IsCollectionExist(clName))
            {
                cs.DropCollection(clName);
            }
            cl = cs.CreateCollection(clName);
            BsonDocument record = new BsonDocument();
            record.Add("maxValue", new BsonInt64(long.MaxValue));
            record.Add("minValue", new BsonInt64(long.MinValue));
            record.Add("value", new BsonInt64(14643));
            cl.Insert(record);
        }

        private void checkData()
        {
            DBCursor cur = cl.Query();
            while (cur.Next() != null)
            {
                BsonDocument doc = cur.Current();
                Assert.AreEqual(0, doc.GetElement("maxValue").Value.CompareTo(long.MaxValue));
                Assert.AreEqual(0, doc.GetElement("minValue").Value.CompareTo(long.MinValue));
                Assert.AreEqual(0, doc.GetElement("value").Value.CompareTo(14643));
            }
            cur.Close();
        }
    }
}
