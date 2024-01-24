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
     *                  1.MaxKey类型的数据
     *                  2.获取hashCode值，检查是否正确
     * testcase:    14606
     * author:      chensiqin
     * date:        2019/03/07
    */

    [TestClass]
    public class TestBsonMaxKey14606
    {
        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
        }

        [TestMethod]
        public void Test14606()
        {
            BsonDocument doc = new BsonDocument();
            BsonMaxKey bsonMaxKey1 = BsonMaxKey.Value;
            BsonMaxKey bsonMaxKey2 = BsonMaxKey.Value;
            doc.Add("maxKey", bsonMaxKey1);
            Assert.AreEqual(bsonMaxKey1.GetHashCode(), bsonMaxKey2.GetHashCode());
            Assert.AreEqual(bsonMaxKey1.GetHashCode(), doc.GetElement("maxKey").Value.GetHashCode());

            BsonMinKey bsonMinKey = BsonMinKey.Value;
            Assert.AreNotEqual(bsonMaxKey1.GetHashCode(), bsonMinKey.GetHashCode());
        }

        [TestCleanup()]
        public void TearDown()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " end  : " + this.GetType().ToString());
        }
    }
}
