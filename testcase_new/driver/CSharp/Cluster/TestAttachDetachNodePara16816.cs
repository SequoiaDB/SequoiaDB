using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB;
using CSharp.TestCommon;
using SequoiaDB.Bson;

namespace CSharp.Cluster
{
    /**
     * TestCase : seqDB-16635
     * test interface:   attachNode/detachNode
     * author:  chensiqin
     * date:    2018/12/18
     * version: 1.0
    */

    [TestClass]
    public class TestAttachDetachNodePara16816
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);
        }

        [TestMethod]
        public void Test16816()
        {
            if (Common.isStandalone(sdb))
            {
                return;
            }
            List<string> rgNames = Common.getDataGroupNames(sdb);
            ReplicaGroup rg = sdb.GetReplicaGroup(rgNames[0]);
            Node node = rg.GetMaster();
            try
            {
                rg.DetachNode(node.HostName, node.Port, null);
                Assert.Fail("DetachNode when options is null  expected fail  but success!  ");
            }
            catch (BaseException e)
            {
                Assert.AreEqual(-6, e.ErrorCode);
            }
            try
            {
                rg.AttachNode(node.HostName, node.Port, null);
                Assert.Fail("AttachNode when options is null  expected fail  but success!  ");
            }
            catch (BaseException e)
            {
                Assert.AreEqual(-6, e.ErrorCode);
            }
            try
            {
                rg.DetachNode(node.HostName, node.Port, new BsonDocument());
                Assert.Fail("DetachNode when options is empty  expected fail  but success!  ");
            }
            catch (BaseException e)
            {
                Assert.AreEqual(-6, e.ErrorCode);
            }
            try
            {
                rg.AttachNode(node.HostName, node.Port, new BsonDocument());
                Assert.Fail("AttachNode when options is empty  expected fail  but success!  ");
            }
            catch (BaseException e)
            {
                Assert.AreEqual(-6, e.ErrorCode);
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
