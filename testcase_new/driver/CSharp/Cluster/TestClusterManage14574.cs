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
     * description:    	 attachNode (string hostName, int port, BsonDocument options) detachNode (string hostName, int port, BsonDocument options)
     *                   1、创建数据组 
     *                   2、指定options参数去挂载数据组节点，检查去挂载结果正确性  
     *                   3、再次去挂载该节点到数据组（不指定opitons参数），并启停节点，检查节点信息正确性
     * testcase:         14574
     * author:           chensiqin
     * date:             2019/04/18
     */

    [TestClass]
    public class TestClusterManage14574
    {
        private Sequoiadb sdb = null;
        private string rgName = "group14574";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
        }

        [TestMethod]
        public void Test14574()
        {
            if (Common.IsStandalone(sdb))
            {
                return;
            }
            if (sdb.IsReplicaGroupExist(rgName))
            {
                sdb.RemoveReplicaGroup(rgName);
            }
            //1、创建数据组 
            string hostName = sdb.GetReplicaGroup("SYSCatalogGroup").GetMaster().HostName;
            int port1 = SdbTestBase.reservedPortBegin + 10;
            int port2 = SdbTestBase.reservedPortBegin + 20;
            ReplicaGroup rg = sdb.CreateReplicaGroup(rgName);
            rg.Start();

            Node node1 = rg.CreateNode(hostName, port1, SdbTestBase.reservedDir + "/" + port1, new BsonDocument("instanceid", 100));
            Dictionary<string, string> map = new Dictionary<string, string>();
            map.Add("instanceid", "110");
            Node node2 = rg.CreateNode(hostName, port2, SdbTestBase.reservedDir + "/" + port2, map);
            node1.Start();
            node2.Start();
            //2、指定options参数去挂载数据组节点，检查去挂载结果正确性 
            rg.DetachNode(node1.HostName, node1.Port, new BsonDocument().Add("KeepData",true));
            Assert.AreEqual(1, rg.GetNodeNum(SDBConst.NodeStatus.SDB_NODE_ALL));
            //3、再次去挂载该节点到数据组（不指定opitons参数），并启停节点，检查节点信息正确性
            rg.AttachNode(node1.HostName, node1.Port, new BsonDocument().Add("KeepData", false));
            Assert.AreEqual(2, rg.GetNodeNum(SDBConst.NodeStatus.SDB_NODE_ALL));
            node1.Stop();
            //SEQUOIADBMAINSTREAM-4394
            //Assert.AreEqual(1, rg.GetNodeNum(SDBConst.NodeStatus.SDB_NODE_ACTIVE));
            node1.Start();
            Assert.AreEqual(2, rg.GetNodeNum(SDBConst.NodeStatus.SDB_NODE_ACTIVE));
        }

        [TestCleanup()]
        public void TearDown()
        {
            if (sdb.IsReplicaGroupExist(rgName))
            {
                sdb.RemoveReplicaGroup(rgName);
            }
            if (sdb != null)
            {
                sdb.Disconnect();
            }
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " end  : " + this.GetType().ToString());
        } 
    }
}
