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
     *                  Equals (Object obj)   Equals  (BsonMinKey rhs)
     *                  1.对比MinKey类型对比，分别验证如下情况：MinKey和MinKey比较、MinKey和其他类型比较、MinKey和null比较
     *                  2.使用Equals比较（覆盖object和BsonMinKey两种类型比较），检查结果；
     * testcase:    14608
     * author:      chensiqin
     * date:        2019/03/07
    */

    [TestClass]
    public class TestBsonMinKey14608
    {

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
        }

        [TestMethod]
        public void Test14608()
        {
            BsonDocument doc = new BsonDocument();
            BsonMinKey bsonMinKey1 = BsonMinKey.Value;
            BsonMinKey bsonMinKey2 = BsonMinKey.Value;
            doc.Add("minKey", bsonMinKey1);
            string otherTypeValue = "MinKey()";

            Assert.AreEqual(true, bsonMinKey1.Equals(bsonMinKey1));
            Assert.AreEqual(true, bsonMinKey1.Equals(bsonMinKey2));
            Assert.AreEqual(true, bsonMinKey1.Equals(doc.GetElement("minKey").Value));
            Assert.AreEqual(false, bsonMinKey1.Equals(null));
            Assert.AreEqual(false, bsonMinKey1.Equals(otherTypeValue));

        }

        [TestCleanup()]
        public void TearDown()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " end  : " + this.GetType().ToString());
        }
    }
}
