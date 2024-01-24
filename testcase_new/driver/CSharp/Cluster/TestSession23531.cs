using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB.Bson;
using SequoiaDB;
using CSharp.TestCommon;

namespace CSharp.Cluster
{
    /**
     * description: set session PreferedInstance对大小写不敏感
     * testcase:    seqDB-23531
     * author:      liuli   
     * date:        2021/02/05
    */
    [TestClass]
    public class TestSession23531
    {

        private Sequoiadb sdb = null;

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);          
            sdb.Connect();
           
        }

        [TestMethod]
        public void Test23531()
        {
            if (Common.isStandalone(sdb))
            {
                return;
            }

            sdb.SetSessionAttr(new BsonDocument("PreferedInstance", "M"));
            Assert.AreEqual(sdb.GetSessionAttr().GetElement("PreferedInstance").ToString(), "PreferedInstance=M");

            sdb.SetSessionAttr(new BsonDocument("PreferedInstance", "S"));
            Assert.AreEqual(sdb.GetSessionAttr().GetElement("PreferedInstance").ToString(), "PreferedInstance=S");

            sdb.SetSessionAttr(new BsonDocument("PreferedInstance", "A"));
            Assert.AreEqual(sdb.GetSessionAttr().GetElement("PreferedInstance").ToString(), "PreferedInstance=A");

            sdb.SetSessionAttr(new BsonDocument("PreferedInstance", "m"));
            Assert.AreEqual(sdb.GetSessionAttr().GetElement("PreferedInstance").ToString(), "PreferedInstance=M");

            sdb.SetSessionAttr(new BsonDocument("PreferedInstance", "s"));
            Assert.AreEqual(sdb.GetSessionAttr().GetElement("PreferedInstance").ToString(), "PreferedInstance=S");

            sdb.SetSessionAttr(new BsonDocument("PreferedInstance", "a"));
            Assert.AreEqual(sdb.GetSessionAttr().GetElement("PreferedInstance").ToString(), "PreferedInstance=A");

            sdb.SetSessionAttr(new BsonDocument("PreferedInstance", "-M"));
            Assert.AreEqual(sdb.GetSessionAttr().GetElement("PreferedInstance").ToString(), "PreferedInstance=-M");

            sdb.SetSessionAttr(new BsonDocument("PreferedInstance", "-S"));
            Assert.AreEqual(sdb.GetSessionAttr().GetElement("PreferedInstance").ToString(), "PreferedInstance=-S");

            sdb.SetSessionAttr(new BsonDocument("PreferedInstance", "-A"));
            Assert.AreEqual(sdb.GetSessionAttr().GetElement("PreferedInstance").ToString(), "PreferedInstance=-A");

            sdb.SetSessionAttr(new BsonDocument("PreferedInstance", "-m"));
            Assert.AreEqual(sdb.GetSessionAttr().GetElement("PreferedInstance").ToString(), "PreferedInstance=-M");

            sdb.SetSessionAttr(new BsonDocument("PreferedInstance", "-s"));
            Assert.AreEqual(sdb.GetSessionAttr().GetElement("PreferedInstance").ToString(), "PreferedInstance=-S");

            sdb.SetSessionAttr(new BsonDocument("PreferedInstance", "-a"));
            Assert.AreEqual(sdb.GetSessionAttr().GetElement("PreferedInstance").ToString(), "PreferedInstance=-A");
        }

        [TestCleanup()]
        public void TearDown()
        {
            if (sdb != null)
            {
                sdb.Disconnect();
            }
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " end  : " + this.GetType().ToString());
        }
    }
}
