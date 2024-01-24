using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB.Bson;
using SequoiaDB;
using CSharp.TestCommon;


namespace CSharp.Sdb
{
    /**
     * description:   1.调用setSessionAttr (BSONObject options)设置会话属性
     *                2.通过快照或者get检查set后的参数是否生效
     * testcase:    20304
     * author:      chensiqin
     * date:        2019/12/09
     */

    [TestClass]
    public class TestSetSessionAttr20304
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
        public void Test20304()
        {
            if (Common.IsStandalone(sdb) == true)
            {
                return;
            }
            BsonDocument options = new BsonDocument()
                                     .Add("Timeout", 15000);
            sdb.SetSessionAttr(options);
            BsonDocument doc = sdb.GetSessionAttr();
            Assert.AreEqual("15000", doc.GetElement("Timeout").Value.ToString());
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
