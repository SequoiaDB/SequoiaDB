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
     *                 BSONTimestamp (int timestamp, int increment)  BsonTimestamp（long value）  ToString ()
     *                 1.插入timestamp类型的数据（使用不同接口插入）
     *                 2.将timestamp类型数据转换为string类型
     * testcase:    14600
     * author:      chensiqin
     * date:        2019/03/06
    */

    [TestClass]
    public class TestBsonTimestamp14600
    {
        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
        }

        [TestMethod]
        public void Test14600()
        {
            BsonDocument doc1 = new BsonDocument();
            BsonDefaults.JsCompatibility = true;
            doc1.Add("timestamp1", new BsonTimestamp(1482396787, 999999));
            Assert.AreEqual("{ \"timestamp1\" : { \"$timestamp\" : \"2016-12-22-16.53.07.999999\" } }", doc1.ToString());

            BsonDocument doc2 = new BsonDocument();
            doc2.Add("timestamp2", new BsonTimestamp(6366845719861477951));
            Assert.AreEqual("{ \"timestamp2\" : { \"$timestamp\" : \"2016-12-22-16.53.07.999999\" } }", doc2.ToString());

            BsonDefaults.JsCompatibility = false;
            Assert.AreEqual("{ \"timestamp1\" : { \"$timestamp\" : 6366845719861477951 } }", doc1.ToString());
            Assert.AreEqual("{ \"timestamp2\" : { \"$timestamp\" : 6366845719861477951 } }", doc2.ToString());
        }

        [TestCleanup()]
        public void TearDown()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " end  : " + this.GetType().ToString());
        }
    }
}
