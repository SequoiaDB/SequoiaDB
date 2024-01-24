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
     *                BsonDouble (int value)         CompareTo (BsonDouble other)
     *                CompareTo (BsonValue other)   Equals (BsonDouble rhs)
     *                Equals (object obj)           GetHashCode ()
     *                ToString ()
     *                1.分别测试BsonDouble类中所有接口,构造BsonDouble类型数据（覆盖边界值），插入数据到sdb中
     *                2.使用Compare接口比较两个BsonDouble类型数据（覆盖相同和不同值比较、不同类型相同值比较）
     *                3.使用equals接口比较两个BsonDouble类型数据（覆盖相同和不同值比较、不同类型相同值比较）
     *                4、获取数据的hashCode值
     *                5、执行toString()方法，检查结果
     * testcase:    14641
     * author:      chensiqin
     * date:        2019/03/04
    */

    [TestClass]
    public class TestBsonDouble14641
    {

        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private string clName = "cl14641";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
        }

        [TestMethod]
        public void Test14641()
        {

            prepareData();
            checkData();

            //Compare接口比较两个BsonDouble类型数据（覆盖相同和不同值比较、不同类型相同值比较
            //equals接口比较两个BsonDouble类型数据（覆盖相同和不同值比较、不同类型相同值比较）

            //相同值比较
            BsonDouble bsonDouble1 = new BsonDouble(131.11);
            BsonDouble bsonDouble2 = new BsonDouble(131.11);
            Assert.AreEqual(0, bsonDouble1.CompareTo(bsonDouble2));
            Assert.AreEqual(true, bsonDouble1.Equals(bsonDouble2));

            //获取数据的hashCode值,相同对象hashcode一致，不同对象相同值hashcode值一致
            Assert.AreEqual(bsonDouble1.GetHashCode(), bsonDouble1.GetHashCode());
            Assert.AreEqual(bsonDouble1.GetHashCode(), bsonDouble2.GetHashCode());

            bsonDouble1 = new BsonDouble(13.14);
            bsonDouble2 = new BsonDouble(13.1);
            Assert.AreEqual(1, bsonDouble1.CompareTo(bsonDouble2));
            Assert.AreEqual(false, bsonDouble1.Equals(bsonDouble2));
            //获取数据的hashCode值,不同对象不同值hashcode不一致
            Assert.AreNotEqual(bsonDouble1.GetHashCode(), bsonDouble2.GetHashCode());

            //不同类型相同值比较
            bsonDouble1 = new BsonDouble(13);
            Assert.AreEqual(0, bsonDouble1.CompareTo(new BsonInt32(13)));
            Assert.AreEqual(false, bsonDouble1.Equals(new BsonInt32(13)));
            Assert.AreEqual(true, bsonDouble1.Equals(float.Parse("13")));

            //执行tostring
            Assert.AreEqual("13", bsonDouble1.ToString());
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
            record.Add("doubleMaxValue", new BsonDouble(double.MaxValue));
            record.Add("doubleMinValue", new BsonDouble(double.MinValue));
            record.Add("doubleValue", new BsonDouble(146.42));
            cl.Insert(record);
        }

        private void checkData()
        {
            DBCursor cur = cl.Query();
            while (cur.Next() != null)
            {
                BsonDocument doc = cur.Current();
                Assert.AreEqual(0, doc.GetElement("doubleMaxValue").Value.CompareTo(double.MaxValue));
                Assert.AreEqual(0, doc.GetElement("doubleMinValue").Value.CompareTo(double.MinValue));
                Assert.AreEqual(0, doc.GetElement("doubleValue").Value.CompareTo(146.42));
            }
            cur.Close();
        }
    }
}
