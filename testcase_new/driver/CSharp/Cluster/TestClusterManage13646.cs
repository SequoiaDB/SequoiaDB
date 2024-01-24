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
     * description:    	1、创建数据组，获取数据组的主节点/从节点，分别验证如下场景：
     *                     1）、数据组为空
     *                     2）、数据组无主节点
     *                     3）、数据组有主节点
     *                     4）、catalog元数据中无主节点信息（如手工删除编目表中主节点信息，构造该场景测试）
     * testcase:    13646
     * author:      chensiqin
     * date:        2019/04/15
     */

    [TestClass]
    public class TestClusterManage13646
    {
        private Sequoiadb sdb = null;
        private string rgName = "group13646";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
        }

        [TestMethod]
        public void Test13646()
        {
            if (Common.IsStandalone(sdb))
            {
                return;
            }
            if (sdb.IsReplicaGroupExist(rgName))
            {
                sdb.RemoveReplicaGroup(rgName);
            }
            TestNullGroup();
            TestNotNullGroup();
        }

        private void TestNullGroup()
        {
            try
            {
                ReplicaGroup rg = sdb.CreateReplicaGroup(rgName);
                try
                {
                    rg.GetMaster();
                    Assert.Fail("expected GetMaster faild but success!");
                }
                catch (BaseException e)
                {
                    Assert.AreEqual(-158, e.ErrorCode);
                }
                try
                {
                    rg.GetSlave();
                    Assert.Fail("expected GetSlave faild but success!");
                }
                catch (BaseException e)
                {
                    Assert.AreEqual(-158, e.ErrorCode);
                }
            }
            catch (BaseException e)
            {
                Assert.Fail(e.Message);
            }
            finally
            {
                sdb.RemoveReplicaGroup(rgName);
            }
        }

        private void TestNotNullGroup()
        {
            try
            {
                string hostName = sdb.GetReplicaGroup("SYSCatalogGroup").GetMaster().HostName;
                ReplicaGroup rg = sdb.CreateReplicaGroup(rgName);
                int port1 = SdbTestBase.reservedPortBegin + 10;
                int port2 = SdbTestBase.reservedPortBegin + 20;
                rg.CreateNode(hostName, port1, SdbTestBase.reservedDir + "/" + port1, new BsonDocument());
                rg.CreateNode(hostName, port2, SdbTestBase.reservedDir + "/" + port2, new BsonDocument());
                rg.Start();
                rg.Stop();
                try
                {
                    rg.GetMaster();
                    Assert.Fail("expected GetMaster faild but success!");
                }
                catch (BaseException e)
                {
                    Assert.AreEqual(-71, e.ErrorCode);
                }
                Node node = rg.GetSlave();
                Console.WriteLine(node.NodeName);
                Assert.AreEqual(hostName, node.HostName);
                if (!port1.Equals(node.Port) && !port2.Equals(node.Port))
                {
                    Assert.Fail("node port expected: " + port1 + " or " + port2);
                }
            }
            catch (BaseException e)
            {
                Assert.Fail(e.Message);
            }
            finally
            {
                sdb.RemoveReplicaGroup(rgName);
            }
            //数据组有主节点
            List<string> list =  Common.getDataGroupNames(sdb);
            ReplicaGroup group = sdb.GetReplicaGroup(list[0]);
            Sequoiadb localdb = group.GetMaster().Connect();
            DBCursor cursor = localdb.GetSnapshot(SDBConst.SDB_SNAP_DATABASE,null,null,null);
            BsonDocument doc = cursor.Next();
            Assert.AreEqual(true, doc.GetElement("IsPrimary").Value.ToBoolean());
            group.GetSlave();
            localdb.Disconnect();
            cursor.Close();
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
