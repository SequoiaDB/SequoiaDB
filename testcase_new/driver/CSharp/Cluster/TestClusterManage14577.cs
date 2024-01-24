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
     * description:   1、重复创建相同数据节点，检查创建结果正确性 
     * testcase:    14577
     * author:      chensiqin
     * date:        2019/04/15
     */

    [TestClass]
    public class TestClusterManage14577
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
        public void Test14577()
        {
            if (Common.IsStandalone(sdb))
            {
                return;
            }
            List<string> rgNames = Common.getDataGroupNames(sdb);
            ReplicaGroup rg = sdb.GetReplicaGroup(rgNames[0]);
            Node master = rg.GetMaster();
            try
            {
                rg.CreateNode(master.HostName, master.Port, SdbTestBase.reservedDir + "/" + master.Port, new BsonDocument());
                Assert.Fail("expected faild and throw exception but success!");
            }
            catch (BaseException e)
            {
                Assert.AreEqual(-145, e.ErrorCode);
            }
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
