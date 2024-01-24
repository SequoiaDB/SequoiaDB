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
     *               Equals (object obj)
     *               Equals (BsonObjectId rhs)
     *               CompareTo (BsonObjectId other)
     *               CompareTo (BsonValue other)
     *               GetHashCode ()
     * testcase:     14614
     * author:       chensiqin
     * date:         2019/03/11
    */


    [TestClass]
    public class TestBsonObjectId14614
    {
        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
        }

        [TestMethod]
        public void Test14614()
        {
            //相同BsonObjectId
            BsonObjectId oid1 = new BsonObjectId("123456789012345678901234");
            BsonObjectId oid2 = new BsonObjectId("123456789012345678901234");
            Assert.AreEqual(0, oid1.CompareTo(oid2));
            Assert.AreEqual(true, oid1.Equals(oid2));
            Assert.AreEqual(oid1.GetHashCode(), oid2.GetHashCode());

            //不同值
            oid2 = new BsonObjectId("123456789012345678901233");
            Assert.AreEqual(1, oid1.CompareTo(oid2));
            Assert.AreEqual(false, oid1.Equals(oid2));
            Assert.AreNotEqual(oid1.GetHashCode(), oid2.GetHashCode());

            //BsonObjectId和非object类型（数值相同）比较
            object obj = "123456789012345678901234";
            //TODO:SEQUOIADBMAINSTREAM-4280
            //Assert.AreEqual(true, oid1.Equals(obj));
            Assert.AreNotEqual(oid1.GetHashCode(), obj.GetHashCode());

            BsonDocument doc = new BsonDocument();
            doc.Add("oid1", oid1);
            Assert.AreEqual(0, oid1.CompareTo(doc.GetElement("oid1").Value));

        }

        [TestCleanup()]
        public void TearDown()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " end  : " + this.GetType().ToString());
        }
    }
}
