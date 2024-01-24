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
     *                BsonBoolean (int value)         CompareTo (BsonBoolean other)
     *                CompareTo (BsonValue other)   Equals (BsonBoolean rhs)
     *                Equals (object obj)           GetHashCode ()
     *                ToString ()
     *                1.分别测试BsonBoolean类中所有接口,构造BsonBoolean类型数据（覆盖边界值），插入数据到sdb中
     *                2.使用Compare接口比较两个BsonBoolean类型数据（覆盖相同和不同值比较、不同类型相同值比较）
     *                3.使用equals接口比较两个BsonBoolean类型数据（覆盖相同和不同值比较、不同类型相同值比较）
     *                4、获取数据的hashCode值
     *                5、执行toString()方法，检查结果
     * testcase:    14619
     * author:      chensiqin
     * date:        2019/03/04
    */

    [TestClass]
    public class TestBsonBoolean14619
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private string clName = "cl14619";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
        }

        [TestMethod]
        public void Test14619()
        {
            prepareData();
            checkData();

            //Compare接口比较两个BsonBoolean类型数据（覆盖相同和不同值比较、不同类型相同值比较
            //equals接口比较两个BsonBoolean类型数据（覆盖相同和不同值比较、不同类型相同值比较）
            //相同值比较
            BsonBoolean bsonBoolean1 = BsonBoolean.Create(true);
            BsonBoolean bsonBoolean2 = BsonBoolean.Create(true);
            Assert.AreEqual(0, bsonBoolean1.CompareTo(bsonBoolean2));
            Assert.AreEqual(true, bsonBoolean1.Equals(bsonBoolean2));

            //获取数据的hashCode值,相同对象hashcode一致，不同对象相同值hashcode值一致
            Assert.AreEqual(bsonBoolean1.GetHashCode(), bsonBoolean1.GetHashCode());
            Assert.AreEqual(bsonBoolean1.GetHashCode(), bsonBoolean2.GetHashCode());

            bsonBoolean1 = BsonBoolean.Create(true);
            bsonBoolean2 = BsonBoolean.Create(false);
            Assert.AreEqual(1, bsonBoolean1.CompareTo(bsonBoolean2));
            Assert.AreEqual(false, bsonBoolean1.Equals(bsonBoolean2));
            //获取数据的hashCode值,不同对象不同值hashcode不一致
            Assert.AreNotEqual(bsonBoolean1.GetHashCode(), bsonBoolean2.GetHashCode());

            //不同类型相同值比较
            bsonBoolean1 = BsonBoolean.Create(true);
            Assert.AreEqual(1, bsonBoolean1.CompareTo(new BsonInt32(0)));
            Assert.AreEqual(false, bsonBoolean1.Equals(new BsonInt32(1)));

            //执行tostring
            Assert.AreEqual("true", bsonBoolean1.ToString());
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
            record.Add("booleanTrue", true);
            record.Add("booleanFalse", false);
            cl.Insert(record);
        }

        private void checkData()
        {
            DBCursor cur = cl.Query();
            while (cur.Next() != null)
            {
                BsonDocument doc = cur.Current();
                Assert.AreEqual(true, doc.GetElement("booleanTrue").Value.ToBoolean());
                Assert.AreEqual(false, doc.GetElement("booleanFalse").Value.ToBoolean());
            }
            cur.Close();
        }
    }
}
