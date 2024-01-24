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
     * description:    	createReplicaGroup (string rgName) 
     *                  activateReplicaGroup (string rgName) 
     *                  getReplicaGroup (int groupID)
     *                  1、创建数据组1并激活，检查该组信息正确性 
     *                  2、启动该组，检查组是否启动并运行正常 
     *                  3、IsCatalog判断该组角色 
     *                  4、再次创建一个数据组，启动数据组 
     *                  5、listReplicaGroups查看所有复制组信息，检查返回结果 
     *                  6、停止数据组1，检查组是否停止 7、删除组1，检查删除结果正确性(通过组ID查询) 
     * testcase:    14572
     * author:      chensiqin
     * date:        2019/04/16
     */

    [TestClass]
    public class TestClusterManage14572
    {
        private Sequoiadb sdb = null;
        private string rgName1 = "group14572_1";
        private string rgName2 = "group14572_2";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
        }

        [TestMethod]
        public void Test14572()
        {
            if (Common.IsStandalone(sdb))
            {
                return;
            }
            if (sdb.IsReplicaGroupExist(rgName1))
            {
                sdb.RemoveReplicaGroup(rgName1);
            }
            //1、创建数据组1并激活，检查该组信息正确性 
            string hostName = sdb.GetReplicaGroup("SYSCatalogGroup").GetMaster().HostName;
            ReplicaGroup rg = sdb.CreateReplicaGroup(rgName1);
            int port1 = SdbTestBase.reservedPortBegin + 10;
            rg.CreateNode(hostName, port1, SdbTestBase.reservedDir + "/" + port1, new BsonDocument());
            rg = sdb.ActivateReplicaGroup(rgName1);
            BsonDocument detail = rg.GetDetail();
            string GID = detail.GetElement("GroupID").Value.ToString(); 
            Assert.AreEqual(rgName1, detail.GetElement("GroupName").Value.ToString());
            Assert.AreEqual(1, detail.GetElement("Status").Value.ToInt32());
            BsonArray groupArr = (BsonArray)detail.GetElement("Group").Value;
            detail = (BsonDocument)groupArr[0];
            Assert.AreEqual(hostName, detail.GetElement("HostName").Value.ToString());
            Assert.AreEqual(SdbTestBase.reservedDir + "/" + port1+"/", detail.GetElement("dbpath").Value.ToString());
            //2、启动该组，检查组是否启动并运行正常 
            rg.Start();
            Sequoiadb localdb = new Sequoiadb(hostName, port1);
            localdb.Connect();
            localdb.Disconnect();
            //3、IsCatalog判断该组角色 
            Assert.AreEqual(false, rg.IsCatalog);
            //4、再次创建一个数据组，启动数据组 
            if (sdb.IsReplicaGroupExist(rgName2))
            {
                sdb.RemoveReplicaGroup(rgName2);
            }
            rg = sdb.CreateReplicaGroup(rgName2);
            rg.Start();
            //5、listReplicaGroups查看所有复制组信息，检查返回结果 
            DBCursor cur = sdb.ListReplicaGroups();
            List<string> list = new List<string>();
            while (cur.Next() != null)
            {
                BsonDocument doc = cur.Current();
                list.Add(doc.GetElement("GroupName").Value.ToString());
            }
            cur.Close();
            Assert.IsTrue(list.Contains(rgName1));
            Assert.IsTrue(list.Contains(rgName2));
            //6、停止数据组1，检查组是否停止 7、删除组1，检查删除结果正确性(通过组ID查询) 
            rg = sdb.GetReplicaGroup(rgName1);
            rg.Stop();
            sdb.RemoveReplicaGroup(rgName1);
            Assert.AreEqual(false, sdb.IsReplicaGroupExist(rgName1));
            try
            {
                sdb.GetReplicaGroup(GID);
                Assert.Fail("expected throw exception, but success!");
            }
            catch (BaseException e)
            {
                Assert.AreEqual(-154, e.ErrorCode);
            }
        }
        
        [TestCleanup()]
        public void TearDown()
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
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " end  : " + this.GetType().ToString());
        } 
    }
}
