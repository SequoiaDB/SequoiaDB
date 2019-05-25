using SequoiaDB;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using System;
using SequoiaDB.Bson;
using System.Collections.Generic;

namespace DriverTest
{
    [TestClass()]
    public class ReplicaGroupTest
    {
        private TestContext testContextInstance;
        private static Config config = null;

        private static Sequoiadb sdb = null;
        private static ReplicaGroup group = null;
        private static SequoiaDB.Node node = null;
        private static int groupID = -1;
        private static String groupName;
        private static Boolean isCluster = true;
        

        public TestContext TestContext
        {
            get
            {
                return testContextInstance;
            }
            set
            {
                testContextInstance = value;
            }
        }

        #region 附加测试特性
        //使用 SequoiadbInitialize 在运行类中的第一个测试前先运行代码
        [ClassInitialize()]
        public static void SequoiadbInitialize(TestContext testContext)
        {
            if (config == null)
            {
                config = new Config();
            }
            // check whether it is in the cluster environment or not
            sdb = new Sequoiadb(config.conf.Coord.Address);
            sdb.Connect(config.conf.UserName, config.conf.Password);
            if (!Constants.isClusterEnv(sdb))
            {
                isCluster = false;
                return;
            }
        }
        //使用 SequoiadbCleamUp 在运行完类中的所有测试后再运行代码
        [ClassCleanup()]
        public static void SequoiadbCleamUp()
        {
            sdb.Disconnect();
        }
        //使用 TestInitialize 在运行每个测试前先运行代码
        [TestInitialize()]
        public void MyTestInitialize()
        {
            if (!isCluster)
            {
                return;
            }
            group = sdb.GetReplicaGroup(1000);
            groupName = group.GroupName;
            groupID = 1000;
        }
        //使用 TestCleanup 在运行完每个测试后运行代码
        [TestCleanup()]
        public void MyTestCleanup()
        {
            if (!isCluster)
            {
                return;
            }
        }
        #endregion

        [TestMethod]
        public void getGroupTest() 
        {
            if (!isCluster) 
            {
                return;
            }
            try
            {
                groupName = "SYSCatalogGroup12345";
                group = sdb.GetReplicaGroup(groupName);
                Assert.Fail("should get SDB_CLS_GRP_NOT_EXIST(-154) error");
            } 
            catch (BaseException e) 
            {
                Assert.AreEqual("SDB_CLS_GRP_NOT_EXIST", e.ErrorType);
            }
            try 
            {
                group = sdb.GetReplicaGroup(0);
                Assert.Fail("should get SDB_CLS_GRP_NOT_EXIST(-154) error");
            } 
            catch (BaseException e) 
            {
                Assert.AreEqual("SDB_CLS_GRP_NOT_EXIST", e.ErrorType);
            }
        }

        [TestMethod]
        public void getNodeTest() 
        {
            if (!isCluster)
            {
                return;
            }
            // case 1: normal case
            groupName = "SYSCatalogGroup";
            group = sdb.GetReplicaGroup(groupName);
            SequoiaDB.Node master = group.GetMaster();
            Console.WriteLine(string.Format("group is: {0}, master is: {1}", groupName, master.NodeName));
            String hostName = master.HostName;
            int hostPort = master.Port;
            SequoiaDB.Node node1 = group.GetNode(hostName, hostPort);
            Console.WriteLine(string.Format("group is: {0}, node1 is: {1}", groupName, node1.NodeName));
            SequoiaDB.Node node2 = group.GetNode(hostName + ":" + hostPort);
            Console.WriteLine(string.Format("group is: {0}, node2 is: {1}", groupName, node2.NodeName));
            // case 2: get a node which is not exist
            SequoiaDB.Node node3 = null;
            try 
            {
                node3 = group.GetNode("ubuntu", 30000);
                Assert.Fail("should get SDB_SYS(-10) error");
            } 
            catch (BaseException e) 
            {
                Assert.AreEqual("SDB_CLS_NODE_NOT_EXIST", e.ErrorType);
            }
            try 
            {
                node3 = group.GetNode(hostName, 0);
                Assert.Fail("should get SDB_CLS_NODE_NOT_EXIST(-155) error");
            } 
            catch (BaseException e) 
            {
                Assert.AreEqual("SDB_CLS_NODE_NOT_EXIST", e.ErrorType);
            }
            // case 3: get a node from empty group
            groupName = "groupNoteExist";
            group = sdb.CreateReplicaGroup(groupName);
            try 
            {
                node3 = group.GetNode(hostName, 0);
                Assert.Fail("should get SDB_CLS_NODE_NOT_EXIST(-155) error");
            } 
            catch (BaseException e) 
            {
                Assert.AreEqual("SDB_CLS_NODE_NOT_EXIST", e.ErrorType);
            } 
            finally 
            {
                sdb.RemoveReplicaGroup(groupName);
            }
        }


        [TestMethod()]
        [Ignore]
        public void RgTempTest()
        {
            if (!isCluster)
            {
                return;
            }
            groupName = "SYSCoord";
            SequoiaDB.Node master = null;
            SequoiaDB.Node slave = null;

            // case 1
            group = sdb.GetReplicaGroup(groupName);
            //master = group.GetMaster();
            slave = group.GetSlave();
            Console.WriteLine(String.Format("case1: group is: {0}, master is: {1}, slave is: {2}", groupName,
                    master == null ? null : master.NodeName,
                    slave == null ? null : slave.NodeName));
            // case 2
            int counter1 = 0;
            int counter2 = 0;
            string str1 = "susetzb:40000";
            string str2 = "susetzb:42000";
            for (int i = 0; i < 100; i++)
            {
                slave = group.GetSlave(1,2,3,4,5,6,7);
                if (slave.NodeName.Equals(str1))
                {
                    counter1++;
                }
                else if (slave.NodeName.Equals(str2))
                {
                    counter2++;
                }
            }
            Console.WriteLine("counter1 is: {0}, counter2 is: {1}", counter1, counter2);
        }

        private int _GetMasterPosition(ReplicaGroup group)
        {
            int primaryId = -1;
            int primaryNodePosition = 0;
            bool hasPrimary = true;
            BsonDocument detail = group.GetDetail();
            BsonValue groupValue = detail.Contains(SequoiadbConstants.FIELD_GROUP) ? detail[SequoiadbConstants.FIELD_GROUP] : null;
            if (groupValue == null || !groupValue.IsBsonArray)
            {
                throw new BaseException("SDB_SYS");
            }
            BsonArray nodes = groupValue.AsBsonArray;
            if (nodes.Count == 0)
            {
                throw new BaseException((int)Errors.errors.SDB_CLS_EMPTY_GROUP);
            }
            BsonValue primaryIdValue = detail.Contains(SequoiadbConstants.FIELD_PRIMARYNODE) ? detail[SequoiadbConstants.FIELD_PRIMARYNODE] : null;
            if (primaryIdValue == null)
            {
                hasPrimary = false;
            }
            else if (!primaryIdValue.IsInt32)
            {
                throw new BaseException((int)Errors.errors.SDB_SYS);
            }
            else if ((primaryId = primaryIdValue.AsInt32) == -1)
            {
                hasPrimary = false;
            }
            // try to mark the position of primary node in the nodes list,
            // the value of position is [1, 7]
            int counter = 0;
            foreach (BsonDocument node in nodes)
            {
                counter++;
                BsonValue nodeIdValue = node.Contains(SequoiadbConstants.FIELD_NODEID) ? node[SequoiadbConstants.FIELD_NODEID] : null;
                if (nodeIdValue == null || !nodeIdValue.IsInt32)
                {
                    throw new BaseException((int)Errors.errors.SDB_SYS, "invalid node id in node list");
                }
                int nodeId = nodeIdValue.AsInt32;
                if (hasPrimary && primaryId == nodeId)
                {
                    primaryNodePosition = counter;
                }
            }
            return primaryNodePosition;
        }

        [TestMethod()]
        public void GetMasterAndSlaveNodeTest()
        {
            if (!isCluster)
            {
                return;
            }
            groupName = "SYSCatalogGroup";
            group = sdb.GetReplicaGroup(groupName);
            BsonDocument detail = group.GetDetail();
            BsonArray nodeList = (BsonArray)detail["Group"];
            int primaryNodePosition = _GetMasterPosition(group);
            int nodeCount = nodeList.Count;
            Assert.IsTrue(nodeCount != 0);

            SequoiaDB.Node master = null;
            SequoiaDB.Node slave = null;

            // case 1
            master = group.GetMaster();
            slave = group.GetSlave();
            Console.WriteLine(String.Format("case1: group is: {0}, master is: {1}, slave is: {2}", groupName,
                    master == null ? null : master.NodeName,
                    slave == null ? null : slave.NodeName));
            if (nodeCount == 1) {
                Assert.AreEqual(master.NodeName, slave.NodeName);
            } else {
                Assert.AreNotEqual(master.NodeName, slave.NodeName);
            }

            // case 2
            slave = group.GetSlave(1,2,3,4,5,6,7);
            Console.WriteLine(String.Format("case2: group is: {0}, master is: {1}, slave is: {2}", groupName,
                    master == null ? null : master.NodeName,
                    slave == null ? null : slave.NodeName));
            if (nodeCount == 1) {
                Assert.AreEqual(master.NodeName, slave.NodeName);
            } else {
                Assert.AreNotEqual(master.NodeName, slave.NodeName);
            }

            // case 3
            Random random = new Random();
            int pos1 = random.Next(7) + 1;
            int pos2 = 0;
            while(true) {
                pos2 = random.Next(7) + 1;
                if (pos2 != pos1) {
                    break;
                }
            }
            //pos1 = 4; pos2 = 7;
            slave = group.GetSlave(pos1, pos2);
            Console.WriteLine(String.Format("case3: group is: {0}, master is: {1}, slave is: {2}", groupName,
                    master == null ? null : master.NodeName,
                    slave == null ? null : slave.NodeName));
            if (nodeCount == 1) {
                Assert.AreEqual(master.NodeName, slave.NodeName);
            } else {
                if ((pos1 % nodeCount == pos2 % nodeCount) && 
                    (primaryNodePosition == (pos1 - 1) % nodeCount + 1))
                {
                    Assert.AreEqual(master.NodeName, slave.NodeName);
                }
                else
                {
                    Assert.AreNotEqual(master.NodeName, slave.NodeName);
                }
            }
        }

        [TestMethod()]
        [Ignore]
        public void RGTest()
        {
            String hostName = "192.168.20.166";
            int port = 45000;
            // check whether it is in the cluster environment or not
            if (!Constants.isClusterEnv(sdb))
            {
                Console.WriteLine("It's not a cluster environment.");
                return;
            }
            group = sdb.GetReplicaGroup(groupName);
            if (group == null)
                group = sdb.CreateReplicaGroup(groupName);
            ReplicaGroup group1 = sdb.GetReplicaGroup(group.GroupID);
            Assert.AreEqual(group.GroupName, group1.GroupName);
            ReplicaGroup group2 = sdb.GetReplicaGroup(1);
            Assert.IsNotNull(group2);
            node = group.GetNode(hostName, port);
            if (node == null)
            {
                string dbpath = config.conf.Groups[0].Nodes[0].DBPath;
                Dictionary<string, string> map = new Dictionary<string, string>();
                map.Add("diaglevel", config.conf.Groups[0].Nodes[0].DiagLevel);
                node = group.CreateNode(hostName, port, dbpath, map);
            }
            group.Start();
            int num = group.GetNodeNum(SDBConst.NodeStatus.SDB_NODE_ALL);
            Assert.IsTrue(num > 0);
            BsonDocument detail = group.GetDetail();
            string gn = detail["GroupName"].AsString;
            Assert.IsTrue(groupName.Equals(gn));
            SequoiaDB.Node master = group.GetMaster();
            Assert.IsNotNull(master);
            SequoiaDB.Node slave = group.GetSlave();
            Assert.IsNotNull(slave);
            Assert.IsTrue(node.Stop());
            Assert.IsTrue(node.Start());
            SDBConst.NodeStatus status = node.GetStatus();
            Assert.IsTrue(status == SDBConst.NodeStatus.SDB_NODE_ACTIVE);

            Sequoiadb db = node.Connect(config.conf.UserName, config.conf.Password);
            db.Disconnect();
            node.Stop();
            group.Stop();
        }

        [TestMethod()]
        [Ignore]
        public void removeRG()
        {
            // check whether it is in the cluster environment or not
            if (!Constants.isClusterEnv(sdb))
            {
                Console.WriteLine("It's not a cluster environment.");
                return;
            }
            // get rg
            group = sdb.GetReplicaGroup(groupName);
            if (group == null)
                group = sdb.CreateReplicaGroup(groupName);
            Assert.IsNotNull(group);
            // create node1
            string hostName1 = config.conf.Groups[1].Nodes[1].HostName;
            int port1 = config.conf.Groups[1].Nodes[1].Port;
            string dbPath1 = config.conf.Groups[1].Nodes[1].DBPath;
            Dictionary<string, string> map1 = new Dictionary<string, string>();
            map1.Add("diaglevel", config.conf.Groups[1].Nodes[1].DiagLevel);
            SequoiaDB.Node node1 = group.CreateNode(hostName1, port1, dbPath1, map1);
            Assert.IsNotNull(node1);
            // start node1
            Assert.IsTrue(node1.Start());

            // remove the newly build node
            try
            {
                group.RemoveNode(hostName1, port1, null);
            }
            catch (BsonException e)
            {
                string errInfo = e.Message;
                Console.WriteLine("Error code is: " + errInfo);
            }
            group.Stop();
        }

        [TestMethod()]
        [Ignore]
        public void attach_and_detach_node()
        {
            String hostName = "192.168.20.166";
            int port = 46000;
            SequoiaDB.Node data_node = null;

            // check whether it is in the cluster environment or not
            if (!Constants.isClusterEnv(sdb))
            {
                Console.WriteLine("It's not a cluster environment.");
                return;
            }
            // detach node
            group.DetachNode(hostName, port, null);

            // check
            data_node = group.GetNode(hostName, port);
            Assert.IsNull(data_node);

            //attach node 
            group.AttachNode(hostName, port, null);

            // check
            data_node = group.GetNode(hostName, port);
            Assert.IsNotNull(data_node);
        }

        [TestMethod()]
        [Ignore]
        public void createRG()
        {
            // 1. prepare a empty coord by manually

            // 2. get connection
            Sequoiadb db = new Sequoiadb("192.168.20.165", 11810);
            db.Connect();
            //db.ListCollections();

            // 3. create catalog group
            Dictionary<string, string> map = new Dictionary<String, String>();
            map.Add("businessname", "abc");
            map.Add("diaglevel", "5");
            map.Add("omaddr", "susetzb:11830");
            //    	db.createReplicaCataGroup("192.168.20.165", 11820, "/opt/sequoiadb/database/cata/11820", map);
            BsonDocument obj = new BsonDocument();
            obj.Add("businessname", "abc");
            obj.Add("diaglevel", 5);
            obj.Add("omaddr", "susetzb:12345");
            db.CreateReplicaCataGroup("192.168.20.165", 11820, "/opt/sequoiadb/database/cata/11820", obj);
            ReplicaGroup rg = db.GetReplicaGroup("SYSCatalogGroup");
            SequoiaDB.Node node = rg.CreateNode("192.168.20.165", 11830, "/opt/sequoiadb/database/cata/11830", map);
            node.Start();
        }

    }
}
