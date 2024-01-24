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
     *               Equals ( Object obj)
     *               Equals ( ObjectId rhs)
     *               CompareTo (ObjectId other)
     *               GetHashCode ()
     * testcase:     14647
     * author:       chensiqin
     * date:         2019/03/11
    */

    [TestClass]
    public class TestObjectId14647
    {

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
        }

        [TestMethod]
        public void Test14647()
        {
            //相同objectId；
            ObjectId oid1 = new ObjectId("123456789012345678901234");
            ObjectId oid2 = new ObjectId("123456789012345678901234");
            Assert.AreEqual(0, oid1.CompareTo(oid2));
            Assert.AreEqual(true, oid1.Equals(oid2));
            Assert.AreEqual(oid1.GetHashCode(), oid2.GetHashCode());

            //不同值
            oid2 = new ObjectId("123456789012345678901233");
            Assert.AreEqual(1, oid1.CompareTo(oid2));
            Assert.AreEqual(false, oid1.Equals(oid2));
            Assert.AreNotEqual(oid1.GetHashCode(), oid2.GetHashCode());

            //objectID和非object类型（数值相同）比较
            object obj = "123456789012345678901234";
            Assert.AreEqual(false, oid1.Equals(obj));
            Assert.AreNotEqual(oid1.GetHashCode(), obj.GetHashCode());

        }

        [TestCleanup()]
        public void TearDown()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " end  : " + this.GetType().ToString());
        }
    }
}
