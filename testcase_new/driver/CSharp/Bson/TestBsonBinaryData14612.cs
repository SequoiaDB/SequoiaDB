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
     *                    Equals (final Object obj)
     *                    Equals (BsonBinaryData rhs)
     *                    CompareTo (BsonBinaryData other)
     *                    CompareTo (BsonValue other)
     *                    GetHashCode ()
     *                    1.构造Binary类型的数据，分别覆盖如下情况：相同binary；不同binary值；binary和非bianry类型比较
     *                    2.对2个数据使用equals比较，检查结果；
     *                    3、对2个数据使用CompareTo比较，检查结果
     *                    4.对2个数据使用hascode，检查结果
     * testcase:         14612
     * author:           chensiqin
     * date:             2019/03/11
    */

    [TestClass]
    public class TestBsonBinaryData14612
    {

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
        }

        [TestMethod]
        public void Test14612()
        {
            byte[] bytes1 = System.Text.Encoding.Default.GetBytes("hello sequoiadb!");
            byte[] bytes2 = System.Text.Encoding.Default.GetBytes("hello world!");

            //相同binary
            BsonBinaryData binary1 = new BsonBinaryData(bytes1);
            BsonBinaryData binary2 = new BsonBinaryData(bytes1);
            Assert.AreEqual(true, binary1.Equals(binary2));
            Assert.AreEqual(0, binary1.CompareTo(binary2));
            Assert.AreEqual(binary1.GetHashCode(), binary2.GetHashCode());

            //不同binary值
            binary2 = new BsonBinaryData(bytes2);
            Assert.AreEqual(false, binary1.Equals(binary2));
            Assert.AreEqual(-4, binary1.CompareTo(binary2));
            Assert.AreNotEqual(binary1.GetHashCode(), binary2.GetHashCode());

            //binary和非bianry类型比较
            BsonDocument record = new BsonDocument();
            record.Add("binary1", binary1);
            Assert.AreEqual(false, binary1.Equals(null));
            Assert.AreEqual(0, binary1.CompareTo(record.GetElement("binary1").Value));
        }

        [TestCleanup()]
        public void TearDown()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " end  : " + this.GetType().ToString());
        }
    }
}
