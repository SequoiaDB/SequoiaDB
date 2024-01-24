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
     * description: 
     *              1.IsReplicaGroupExist指定存在/不存在的组id/name
     *              2.IsNodeExist (string nodeName)指定存在/不存在的nodename
     *              3.IsNodeExist (string  hostName,int  port )指定存在/不存在的nodename
     * testcase:    15935
     * author:      chensiqin
     * date:        2018/10/08
    */
    [TestClass]
    public class TestReplicaGroup15935
    {

        private Sequoiadb sdb = null;

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
           
        }

        //SEQUOIADBMAINSTREAM-6488
        //[TestMethod]
        [Ignore]
        public void Test15935()
        {
            Assert.AreEqual(false, sdb.IsReplicaGroupExist("csq"));
            Assert.AreEqual(false, sdb.IsReplicaGroupExist(-100));
            DBCursor cur = sdb.ListReplicaGroups();
            while (cur.Next() != null){
                BsonDocument doc = cur.Current();
                BsonElement element = doc.GetElement("GroupID");
                Assert.AreEqual(true, sdb.IsReplicaGroupExist(element.Value.ToInt32()));
                element = doc.GetElement("GroupName");
                Assert.AreEqual(true, sdb.IsReplicaGroupExist(element.Value.ToString()));
            }
            cur.Close();

            string groupName = "SYSCatalogGroup";
            ReplicaGroup rg = sdb.GetReplicaGroup(groupName);
            Node node = rg.GetMaster();

            Assert.AreEqual(false, rg.IsNodeExist("nodename:111"));
            Assert.AreEqual(false, rg.IsNodeExist("nodename",111));
            Assert.AreEqual(true, rg.IsNodeExist(node.NodeName));
            Assert.AreEqual(true, rg.IsNodeExist(node.HostName, node.Port));

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
