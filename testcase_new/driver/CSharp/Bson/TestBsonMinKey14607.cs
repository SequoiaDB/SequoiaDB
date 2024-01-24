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
     *                   CompareTo (BsonMinKey other)   CompareTo (BsonValue other)
     *                  1.对比MinKey类型对比，分别验证如下情况：MinKey和MinKey比较、MinKey和其他类型比较、MinKey和null比较
     *                  2.使用CompareTo比较（覆盖object和BsonMinKey两种类型比较），检查结果；
     * testcase:    14607
     * author:      chensiqin
     * date:        2019/03/07
    */

    [TestClass]
    public class TestBsonMinKey14607
    {
        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
        }

        [TestMethod]
        public void Test14607()
        {
            BsonDocument doc = new BsonDocument();
            BsonMinKey bsonMinKey1 = BsonMinKey.Value;
            BsonMinKey bsonMinKey2 = BsonMinKey.Value;
            doc.Add("minKey", bsonMinKey1);
            string otherTypeValue = "MinKey()";

            Assert.AreEqual(0, bsonMinKey1.CompareTo(bsonMinKey1));
            Assert.AreEqual(0, bsonMinKey1.CompareTo(bsonMinKey2));
            Assert.AreEqual(0, bsonMinKey1.CompareTo(doc.GetElement("minKey").Value));
            Assert.AreEqual(1, bsonMinKey1.CompareTo(null));
            Assert.AreEqual(-1, bsonMinKey1.CompareTo(otherTypeValue));
        }

        [TestCleanup()]
        public void TearDown()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " end  : " + this.GetType().ToString());
        }
    }
}
