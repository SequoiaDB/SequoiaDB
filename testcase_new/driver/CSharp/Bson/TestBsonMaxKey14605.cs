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
     *                  Equals (Object obj)   Equals  (BsonMaxKey rhs)
     *                  1.对比MaxKey类型对比，分别验证如下情况：MaxKey和MaxKey比较、MaxKey和其他类型比较、MaxKey和null比较
     *                  2.使用equals比较（覆盖object和BsonMaxKey两种类型比较），检查结果；
     * testcase:    14605
     * author:      chensiqin
     * date:        2019/03/07
    */

    [TestClass]
    public class TestBsonMaxKey14605
    {

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
        }

        [TestMethod]
        public void Test14605()
        {
            BsonDocument doc = new BsonDocument();
            BsonMaxKey bsonMaxKey1 = BsonMaxKey.Value;
            BsonMaxKey bsonMaxKey2 = BsonMaxKey.Value;
            doc.Add("maxKey", bsonMaxKey1);
            string otherTypeValue = "MaxKey()";

            Assert.AreEqual(true, bsonMaxKey1.Equals(bsonMaxKey1));
            Assert.AreEqual(true, bsonMaxKey1.Equals(bsonMaxKey2));
            Assert.AreEqual(true, bsonMaxKey1.Equals(doc.GetElement("maxKey").Value));
            Assert.AreEqual(false, bsonMaxKey1.Equals(null));
            Assert.AreEqual(false, bsonMaxKey1.Equals(otherTypeValue));
            
        }

        [TestCleanup()]
        public void TearDown()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " end  : " + this.GetType().ToString());
        }
    }
}
