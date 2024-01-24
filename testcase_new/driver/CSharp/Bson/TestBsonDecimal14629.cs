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
     *                 compareTo (BSONDecimal obj)
     *                 CompareTo (BsonDecimal other)
     *                 equals (Object obj)
     *                 Equals (BsonDecimal rhs)
     *                 hashCode ()
     *                 1.构造decimal类型的数据，分别包括带精度及不带精度两种格式，覆盖如下情况：
     *                     值相等但精度不相等；值相等且精度相等；值相等且类型相同；值相同但类型不同；值不相等但精度相同，值相同且精度相同
     *                 2.对2个数据进行compareTo比较，检查结果
     *                 3.对2个数据使用equals比较，检查结果；
     *                 4.对2个数据使用hascode，检查结果
     * testcase:     seqDB-14629
     * author:       chensiqin
     * date:         2019/03/14
    */

    [TestClass]
    public class TestBsonDecimal14629
    {

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
        }

        [TestMethod]
        public void Test14629()
        {
            //1.构造decimal类型的数据，分别包括带精度及不带精度两种格式，覆盖如下情况：
            //值相等但精度不相等；值相等且精度相等；值相等且类型相同；值相同但类型不同；值不相等但精度相同，值相同且精度相同

            //带精度
            Test1();
            //不带精度
            Test2();
        }

        private void Test1()
        {
            BsonDecimal decimal1 = new BsonDecimal("123.456", 6, 2);
            Assert.AreEqual(0, decimal1.CompareTo(decimal1));
            Assert.AreEqual(true, decimal1.Equals(decimal1));
            Assert.AreEqual(decimal1.GetHashCode(), decimal1.GetHashCode());
           
            BsonDecimal decimal2 = new BsonDecimal("123.456", 6, 3);
            Assert.AreEqual(1, decimal1.CompareTo(decimal2));
            Assert.AreEqual(false, decimal1.Equals(decimal2));
            Assert.AreNotEqual(decimal1.GetHashCode(), decimal2.GetHashCode());

            //值相等且精度相等 值相等且类型相同 值相同且精度相同
            BsonDecimal decimal3 = new BsonDecimal("123.456", 6, 2);
            Assert.AreEqual(0, decimal1.CompareTo(decimal3));
            Assert.AreEqual(true, decimal1.Equals(decimal3));
            Assert.AreEqual(decimal1.GetHashCode(), decimal3.GetHashCode());

            //值相等但精度不相等
            BsonDecimal decimal4 = new BsonDecimal("123.456", 5, 2);
            Assert.AreEqual(0, decimal1.CompareTo(decimal4));
            Assert.AreEqual(true, decimal1.Equals(decimal4));
            Assert.AreEqual(decimal1.GetHashCode(), decimal4.GetHashCode());

            //值相同但类型不同
            BsonDocument doc = new BsonDocument();
            doc.Add("bsondecimal", new BsonDecimal("123.4567", 7, 4));
            BsonDecimal decimal5 = new BsonDecimal("123.4567", 7, 4);
            Assert.AreEqual(0, decimal5.CompareTo(doc.GetElement("bsondecimal").Value));
            //TODO:SEQUOIADBMAINSTREAM-4280
            //Assert.AreEqual(true, decimal5.Equals(double.Parse("123.4567")));

            //值不相等但精度相同
            BsonDecimal decimal6 = new BsonDecimal("123.1234", 6, 2);
            Assert.AreEqual(1, decimal1.CompareTo(decimal6));
            Assert.AreEqual(false, decimal1.Equals(decimal6));
            Assert.AreNotEqual(decimal1.GetHashCode(), decimal6.GetHashCode());
        }

        private void Test2()
        {
            //值相等且类型相同；
            BsonDecimal decimal1 = new BsonDecimal("1231234");
            BsonDecimal decimal2 = new BsonDecimal("1231234");
            Assert.AreEqual(0, decimal1.CompareTo(decimal1));
            Assert.AreEqual(true, decimal1.Equals(decimal1));
            Assert.AreEqual(decimal1.GetHashCode(), decimal1.GetHashCode());

            Assert.AreEqual(0, decimal1.CompareTo(decimal2));
            Assert.AreEqual(true, decimal1.Equals(decimal2));
            Assert.AreEqual(decimal1.GetHashCode(), decimal2.GetHashCode());

            //值相同但类型不同
            Assert.AreEqual(0, decimal1.CompareTo(int.Parse("1231234")));
            Assert.AreEqual(true, decimal1.Equals(int.Parse("1231234")));
            
        }

        [TestCleanup()]
        public void TearDown()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " end  : " + this.GetType().ToString());
        }
    }
}
