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
     *                  CompareTo (BsonMaxKey other)   CompareTo (BsonValue other)
     *                  1.对比MaxKey类型对比，分别验证如下情况：MaxKey和MaxKey比较、MaxKey和其他类型比较、MaxKey和null比较
     *                  2.使用CompareTo比较（覆盖BsonValue和BsonMaxKey两种类型参数比较），检查结果；
     * testcase:    14604
     * author:      chensiqin
     * date:        2019/03/07
    */

    [TestClass]
    public class TestBsonMaxKey14604
    {

       [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
        }

       [TestMethod]
       public void Test14604()
       {
           BsonDocument doc = new BsonDocument();
           BsonMaxKey bsonMaxKey1 = BsonMaxKey.Value;
           BsonMaxKey bsonMaxKey2 = BsonMaxKey.Value;
           doc.Add("maxKey", bsonMaxKey1);
           string otherTypeValue = "MaxKey()";

           Assert.AreEqual(0, bsonMaxKey1.CompareTo(bsonMaxKey1));
           Assert.AreEqual(0, bsonMaxKey1.CompareTo(bsonMaxKey2));
           Assert.AreEqual(0, bsonMaxKey1.CompareTo(doc.GetElement("maxKey").Value));
           Assert.AreEqual(1, bsonMaxKey1.CompareTo(null));
           Assert.AreEqual(1, bsonMaxKey1.CompareTo(otherTypeValue));
       }

       [TestCleanup()]
       public void TearDown()
       {
           Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " end  : " + this.GetType().ToString());
       }
    }
}
