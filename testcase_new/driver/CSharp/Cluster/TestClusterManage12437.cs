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
     * description:    	  	1.创建一个数据组，并新增多个节点，并在该组上创建cs.cl,并插入记录
     *                      2.使用detach_node指定KeepData参数卸载节点，检查结果
     *                      3.创建第二个数据组
     *                      4.使用attach_node指定KeepData参数将步骤2中detach的节点新增到第二个数据组组，检查结果
     *                      5.连接第二个数据组中的节点，查询cs.cl中数据，检查结果
     * testcase:         12437
     * author:           chensiqin 2019/04/18
     *                   liuli     2020/11/12
     */

    [TestClass]
    public class TestClusterManage12437
    {
        private Sequoiadb sdb = null;
        private string rgName1 = "group12437_1";
        private string rgName2 = "group12437_2";
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private string clName = "cl12437";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
        }

        [TestMethod]
        public void Test12437()
        {
            if (Common.IsStandalone(sdb))
            {
                return;
            }
            if (sdb.IsReplicaGroupExist(rgName1))
            {
                sdb.RemoveReplicaGroup(rgName1);
            }
            string hostName = sdb.GetReplicaGroup("SYSCatalogGroup").GetMaster().HostName;
            int port1 = SdbTestBase.reservedPortBegin + 10;
            int port2 = SdbTestBase.reservedPortBegin + 20;
            ReplicaGroup rg = sdb.CreateReplicaGroup(rgName1);

            Node node1 = rg.CreateNode(hostName, port1, SdbTestBase.reservedDir + "/" + port1, new BsonDocument("instanceid", 100));
            Dictionary<string, string> map = new Dictionary<string, string>();
            map.Add("instanceid", "110");
            Node node2 = rg.CreateNode(hostName, port2, SdbTestBase.reservedDir + "/" + port2, map);
            rg.Start();
            //向新组上插入记录
            Node master = null;
            while (true)
            {
                try
                {
                    master = rg.GetMaster();
                    break;
                }
                catch (BaseException e)
                {
                    Assert.AreEqual(-71, e.ErrorCode);
                }
            }
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);
            cl = cs.CreateCollection(clName, new BsonDocument("Group", rgName1).Add("ReplSize", 0));
            List<BsonDocument> datas = new List<BsonDocument>();
            for (int i = 0; i < 100; i++)
            {
                BsonDocument doc = new BsonDocument();
                doc.Add("_id", i);
                doc.Add("number", i);
                datas.Add(doc);
            }
            cl.Insert(datas, 0);
            rg.DetachNode(node1.HostName, node1.Port, new BsonDocument("KeepData", true));
            Assert.AreEqual(false, rg.IsNodeExist(node1.NodeName));

            //创建新组 attachnode
            if (sdb.IsReplicaGroupExist(rgName2))
            {
                sdb.RemoveReplicaGroup(rgName2);
            }
            rg = sdb.CreateReplicaGroup(rgName2);
            rg.Start();
            rg.AttachNode(hostName, port1, new BsonDocument("KeepData", true));
            while (true)
            {
                try
                {
                    master = rg.GetMaster();
                    break;
                }
                catch (BaseException e)
                {
                    Assert.AreEqual(-71, e.ErrorCode);
                }
            }
            Sequoiadb localdb = master.Connect("", "");
            cs = localdb.GetCollectionSpace(SdbTestBase.csName);
            cl = cs.GetCollection(clName);
            Assert.AreEqual(100, cl.GetCount(new BsonDocument()));
        }

        [TestCleanup()]
        public void TearDown()
        {
            try
            {
                cs = sdb.GetCollectionSpace(SdbTestBase.csName);
                if (cs.IsCollectionExist(clName))
                {
                    cs.DropCollection(clName);
                }
            }
            catch (BaseException e)
            {
                Assert.Fail(e.Message);
            }
            finally
            {
                if (sdb.IsReplicaGroupExist(rgName1))
                {
                    sdb.RemoveReplicaGroup(rgName1);
                }
                if (sdb.IsReplicaGroupExist(rgName2))
                {
                    sdb.RemoveReplicaGroup(rgName2);
                }
                if (sdb != null)
                {
                    sdb.Disconnect();
                }
            }
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " end  : " + this.GetType().ToString());
        } 
    }
}
