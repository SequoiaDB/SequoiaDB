using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB.Bson;
using SequoiaDB;
using CSharp.TestCommon;

namespace CSharp.Transaction
{
    /**
     * description:    	  	
     *                       	1.开启1个连接，使用getSessionAttr，获取该会话上的事务属性
     *                       	2.使用setSessionAttr，设置该会话上事务的隔离级别为RS
     *                       	3.使用getSessionAttr，获取该会话上的事务属性
     *                       	4.使用updateConf，设置事务配置的隔离级别为RU，事务超时时间为120s
     *                       	5.使用setSessionAttr,清空缓存
     *                       	6.使用getSessionAttr，获取该会话上的事务属性的缓存
     *                       	7.使用getSessionAttr(false)，获取该会话上的事务属性
     * testcase:         19217
     * author:           chensiqin
     * date:             2019/08/26
     */
    [TestClass]
    public class TestTransaction19217
    {
        private Sequoiadb sdb = null;

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());

        }

        [TestMethod]
        public void Test19217()
        {
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
            BsonDocument sessionDoc = sdb.GetSessionAttr();

            //设置隔离级别为RS
            BsonDocument option = new BsonDocument();
            option.Add("TransIsolation", 2);
            sdb.SetSessionAttr(option);
            sessionDoc = sdb.GetSessionAttr();
            Assert.AreEqual("2", sessionDoc.GetElement("TransIsolation").Value.ToString());

            //设置config和事务隔离级别为RU，超时时间为120秒
            BsonDocument config = new BsonDocument();
            option = new BsonDocument();
            config.Add("transisolation", 0);
            config.Add("transactiontimeout", 120);
            option.Add("Global", true);
            sdb.UpdateConfig(config, option);
            sessionDoc = sdb.GetSessionAttr();
            Assert.AreEqual("2", sessionDoc.GetElement("TransIsolation").Value.ToString());
            Assert.AreEqual("60", sessionDoc.GetElement("TransTimeout").Value.ToString());
            Assert.AreEqual("M", sessionDoc.GetElement("PreferedInstance").Value.ToString());

            //使用setSessionAttr,清空缓存
            sdb.SetSessionAttr(new BsonDocument());

            sessionDoc = sdb.GetSessionAttr();
            Assert.AreEqual("2", sessionDoc.GetElement("TransIsolation").Value.ToString());
            Assert.AreEqual("120", sessionDoc.GetElement("TransTimeout").Value.ToString());
            Assert.AreEqual("M", sessionDoc.GetElement("PreferedInstance").Value.ToString());

            sessionDoc = sdb.GetSessionAttr(false);
            Assert.AreEqual("2", sessionDoc.GetElement("TransIsolation").Value.ToString());
            Assert.AreEqual("120", sessionDoc.GetElement("TransTimeout").Value.ToString());
            Assert.AreEqual("M", sessionDoc.GetElement("PreferedInstance").Value.ToString());
        }


        [TestCleanup()]
        public void TearDown()
        {
            //还原配置项
            BsonDocument config = new BsonDocument();
            BsonDocument option = new BsonDocument();
            config.Add("transisolation", 1);
            config.Add("transactiontimeout", 60);
            sdb.DeleteConfig(config, option);
            if (sdb != null)
            {
                sdb.Disconnect();
            }
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " end  : " + this.GetType().ToString());
        } 
    }
}
