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
     * description:    	 createNode (string hostName, int port, string dbPath, Dictionary< string, string > map)； 
     *                   createNode (string hostName, int port, string dbPath, BsonDocument configure)； 
     *                   getNode (string hostName, int port)； 
     *                   start (); 
     *                   stop () 
     *                   removeNode (string hostName, int port, BsonDocument configure)； 
     *                   getNode (string nodeName)；
     *                   getNode（string hostName，int port）；
     *                   getMaster (); 
     *                   getSlave ()；
     *                   GetDetail（）;
     *                   GetNodeNum (SDBConst.NodeStatus status)
     *                   1、创建数据节点，分别验证两个createNode接口： a、指定Dictionary map b、指定configure 
     *                   2、启动节点，获取节点信息，调用getDetail（）查看该组中所有节点信息 
     *                   3、通过节点名获取节点信息 
     *                   4、通过主机名和端口获取节点信息
     *                   5、获取该组内主备节点信息 
     *                   6、停止其中一个节点，检查节点状态 
     *                   7、删除其中一个节点，通过节点名获取节点信息，检查节点是否存在 
     *                   8、再次创建已删除节点 
     * testcase:         14573
     * author:           chensiqin 2019/04/18
     *                   liuli     2020/10/29
     */

    [TestClass]
    public class TestClusterManage14573
    {
        private Sequoiadb sdb = null;
        private string rgName = "group14573";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
        }

        [TestMethod]
        public void Test14573()
        {
            if (Common.IsStandalone(sdb))
            {
                return;
            }
            if (sdb.IsReplicaGroupExist(rgName))
            {
                sdb.RemoveReplicaGroup(rgName);
            }
            //1、创建数据节点，分别验证两个createNode接口： a、指定Dictionary map b、指定configure 
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
            //2、启动节点，获取节点信息，调用getDetail（）查看该组中所有节点信息 
            BsonDocument detail = rg.GetDetail();
            //TODO:check
            //3、通过节点名获取节点信息,通过主机名和端口获取节点信息
            node1 = rg.GetNode(hostName, port1);
            Assert.AreEqual(hostName, node1.HostName);
            Assert.AreEqual(port1, node1.Port);
            Assert.AreEqual(2, rg.GetNodeNum(SDBConst.NodeStatus.SDB_NODE_ALL));

            //5、获取该组内主备节点信息 
            //测试GetDetail内部关闭游标
            while (true)
            {
                try
                {
                    rg.GetDetail();
                    break;
                }
                catch (BaseException e)
                {
                    Assert.AreEqual(-71, e.ErrorCode);
                }
            }

            while (true)
            {
                try
                {
                    Node master = rg.GetMaster();
                    break;
                }
                catch (BaseException e)
                {
                    Assert.AreEqual(-71, e.ErrorCode);
                }
            }


            Node slave = rg.GetSlave();
            //6、停止其中一个节点，检查节点状态 
            slave.Stop();
            try
            {
                slave.Connect();
                Assert.Fail("slave stop failed!");
            }
            catch (BaseException e)
            {
                if (e.ErrorCode != -79 && e.ErrorCode != -13)
                {
                    throw e;
                }
                //SEQUOIADBMAINSTREAM-4394
                // Assert.AreEqual(SDBConst.NodeStatus.SDB_NODE_INACTIVE, slave.GetStatus());//SDB_NODE_INACTIVE
            }

            //7、删除其中一个节点，通过节点名获取节点信息，检查节点是否存在 
            rg.RemoveNode(slave.HostName, slave.Port, new BsonDocument());
            Assert.AreEqual(false, rg.IsNodeExist(slave.HostName, slave.Port));
            //8、再次创建已删除节点 
            rg.CreateNode(slave.HostName, slave.Port, SdbTestBase.reservedDir + "/" + slave.Port, map);

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
