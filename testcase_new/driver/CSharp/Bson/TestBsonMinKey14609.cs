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
     *                  GetHashCode()
     *                  1.MinKey类型的数据
     *                  2.获取hashCode值，检查是否正确
     * testcase:    14609
     * author:      chensiqin
     * date:        2019/03/07
    */

    [TestClass]
    public class TestBsonMinKey14609
    {
        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
        }

        [TestMethod]
        public void Test14609()
        {
            BsonDocument doc = new BsonDocument();
            BsonMinKey bsonMinKey1 = BsonMinKey.Value;
            BsonMinKey bsonMinKey2 = BsonMinKey.Value;
            doc.Add("minKey", bsonMinKey1);
            Assert.AreEqual(bsonMinKey1.GetHashCode(), bsonMinKey2.GetHashCode());
            Assert.AreEqual(bsonMinKey1.GetHashCode(), doc.GetElement("minKey").Value.GetHashCode());

            BsonMaxKey bsonMaxKey = BsonMaxKey.Value;
            Assert.AreNotEqual(bsonMinKey1.GetHashCode(), bsonMaxKey.GetHashCode());
        }

        [TestCleanup()]
        public void TearDown()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " end  : " + this.GetType().ToString());
        }
    }
}
