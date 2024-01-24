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
     *                BsonDataTime (int value)         CompareTo (BsonDataTime other)
     *                CompareTo (BsonValue other)   Equals (BsonDataTime rhs)
     *                Equals (object obj)           GetHashCode ()
     *                ToString ()
     *                1.分别测试BsonDataTime类中所有接口,构造BsonDataTime类型数据（覆盖边界值），插入数据到sdb中
     *                2.使用Compare接口比较两个BsonDataTime类型数据（覆盖相同和不同值比较、不同类型相同值比较）
     *                3.使用equals接口比较两个BsonDataTime类型数据（覆盖相同和不同值比较、不同类型相同值比较）
     *                4、获取数据的hashCode值
     *                5、执行toString()方法，检查结果
     * testcase:    14640
     * author:      chensiqin
     * date:        2019/03/05
    */

    [TestClass]
    public class TestBsonDateTime14640
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private string clName = "cl14640";
        private DateTime date = new DateTime(2016, 9, 1);
        
        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
        }

        [TestMethod]
        public void Test14640()
        {
            prepareData();
            checkData();

            BsonDateTime bsonDateTime = new BsonDateTime(date);
            //Compare接口比较两个BsonString类型数据（覆盖相同和不同值比较、不同类型相同值比较
            //equals接口比较两个BsonString类型数据（覆盖相同和不同值比较、不同类型相同值比较）
            
            //相同值比较
            BsonDateTime bsonDateTime1 = new BsonDateTime(date);
            BsonDateTime bsonDateTime2 = new BsonDateTime(date);
            Assert.AreEqual(0, bsonDateTime1.CompareTo(bsonDateTime2));
            Assert.AreEqual(true, bsonDateTime1.Equals(bsonDateTime2));
            
            //获取数据的hashCode值,相同对象hashcode一致，不同对象相同值hashcode值一致
            Assert.AreEqual(bsonDateTime1.GetHashCode(), bsonDateTime1.GetHashCode());
            Assert.AreEqual(bsonDateTime1.GetHashCode(), bsonDateTime2.GetHashCode());

            bsonDateTime1 = new BsonDateTime(new DateTime(2016, 9, 10));
            bsonDateTime2 = new BsonDateTime(new DateTime(2016, 10, 9));
            Assert.AreEqual(-1, bsonDateTime1.CompareTo(bsonDateTime2));
            Assert.AreEqual(false, bsonDateTime1.Equals(bsonDateTime2));
            //获取数据的hashCode值,不同对象不同值hashcode不一致
            Assert.AreNotEqual(bsonDateTime1.GetHashCode(), bsonDateTime2.GetHashCode());
            
            //不同类型相同值比较
            Assert.AreEqual(1, bsonDateTime.CompareTo(new BsonString("2016-08-31T16:00:00Z")));
            Assert.AreEqual(false, bsonDateTime.Equals(new BsonString("2016-08-31T16:00:00Z")));
            
            //执行tostring
            Assert.AreEqual("2016-08-31T16:00:00Z", bsonDateTime.ToString());
            DateTime localDataTime =bsonDateTime.ToLocalTime();
            //ToString
            Assert.AreEqual(date.ToString(), localDataTime.ToString());
            //ToLocalTime
            Assert.AreEqual(true, localDataTime.Equals(date));
            //UTC
            Assert.AreEqual(date.ToUniversalTime(), bsonDateTime.ToUniversalTime());
            

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
            BsonDateTime bsonDateTime = new BsonDateTime(date);
            BsonDocument record = new BsonDocument();
            record.Add("value", bsonDateTime);
            cl.Insert(record);
        }

        private void checkData()
        {
            DBCursor cur = cl.Query();
            while (cur.Next() != null)
            {
                BsonDocument doc = cur.Current();
                Assert.AreEqual(0, doc.GetElement("value").Value.CompareTo(date));
            }
            cur.Close();
        }

    }
}
