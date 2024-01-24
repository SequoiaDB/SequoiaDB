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
     * description: UpdateConfig（ BsonDocument configs, BsonDocument options）
     *              DeleteConfig（ BsonDocument configs, BsonDocument options）
     * testcase:    15242
     * author:      chensiqin
     * date:        2018/06/01
    */

    [TestClass]
    public class ClusterManager15242
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
        public void Test15242()
        {
            List<string> dataGroupNames = Common.getDataGroupNames(sdb);
            if (Common.isStandalone(sdb))
            {
                return;
            }
            ReplicaGroup rg = sdb.GetReplicaGroup(dataGroupNames[0]);
            Node node = rg.GetMaster();
            BsonDocument configs = new BsonDocument();
            BsonDocument options = new BsonDocument();
            configs.Add("weight", 20);
            options.Add("GroupName", dataGroupNames[0]);
            options.Add("HostName", node.HostName);
            options.Add("svcname", node.Port + "");
            BsonDocument matcher = new BsonDocument();
            matcher.Add("GroupName", dataGroupNames[0]);
            matcher.Add("SvcName", node.Port);
            matcher.Add("HostName", node.HostName);
            BsonDocument selector = new BsonDocument();
            selector.Add("weight", 1);
            DBCursor cursor = sdb.GetSnapshot(SDBConst.SDB_SNAP_CONFIGS, matcher, selector, null);
            BsonDocument defValue = new BsonDocument();
            while (cursor.Next() != null)
            {
                defValue = cursor.Current();
            }
            cursor.Close();
            sdb.UpdateConfig(configs, options);
            cursor = sdb.GetSnapshot(SDBConst.SDB_SNAP_CONFIGS, matcher, selector, null);
            BsonDocument expected = new BsonDocument();
            expected.Add("weight", 20);
            while (cursor.Next() != null) 
            {
                BsonDocument actual = cursor.Current();
                Assert.AreEqual(expected, actual);
            }
            cursor.Close();

            //deleteconfig
            sdb.DeleteConfig(configs, options);
            cursor = sdb.GetSnapshot(SDBConst.SDB_SNAP_CONFIGS, matcher, selector, null);
            while (cursor.Next() != null)
            {
                BsonDocument actual = cursor.Current();
                Assert.AreEqual(defValue, actual);
            }
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
