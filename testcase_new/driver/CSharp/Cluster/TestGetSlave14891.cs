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
     * description: test GetSlave(int postitions);only test the basic functionality of the interface
     *              1.specify one postition
     *              2.specify two postition
     *              test interface:  GetSlave(int postitions)
     * testcase:    14891/14892
     * author:      wuyan    
     * date:        2018/4/12
    */
    [TestClass]
    public class TestGetSlave14891
    {
        private Sequoiadb sdb = null;
        private string groupName =  "";
        private int masterNodePosition = 0;
        private BsonArray nodeIDs = new BsonArray();
      

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());  
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
            List<string> groupNames = GetGroupNames();
            groupName = groupNames[1]; 
        }
        
        [TestMethod()]
        public void Test14891()
        {
            ReplicaGroup rg = sdb.GetReplicaGroup(groupName);  
            GetNodePositionAndID(rg);
            GetOnePosition(rg);
            GetMultiplePositions(rg);
        }

        [TestCleanup()]
        public void TearDown()
        {
            try
            {
                sdb.Disconnect();              
            }            
            finally
            {
                if (sdb != null)
                {
                    sdb.Disconnect();    
                }
                Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " end  : " + this.GetType().ToString());
            }                   
        }

        private void GetOnePosition(ReplicaGroup group)
        {
            SequoiaDB.Node slave = null;
            int postition = new Random().Next(1, nodeIDs.Count);              
            slave = group.GetSlave(postition);            
            // read the primary node if the primary node position is specified;
            if (postition == masterNodePosition)
            {
                 int getNodeID = slave.NodeID;
                 int masterNodeId = group.GetMaster().NodeID;
                 Assert.AreEqual(getNodeID, masterNodeId, "--the position is " + postition);
             }
             else
             {
                 //get the slave node at the specified location
                 int getNodeID = slave.NodeID;
                 int slaveNodeID = (int)nodeIDs[postition - 1];
                 Assert.AreEqual(getNodeID, slaveNodeID,"--the position is "+postition);
             }
        }

        private void GetMultiplePositions(ReplicaGroup group)
        {            
            BsonArray postititons = new BsonArray();
            BsonArray expNodeID = new BsonArray();
            //get the slave node postititons
            for (int i = 0; i < nodeIDs.Count; i++)
            {
                if ( i != (masterNodePosition -1))
                {
                    postititons.Add(i + 1);
                    expNodeID.Add(nodeIDs[i]);
                } 
            }          
            
            int postition1 = (int)postititons[0];
            int postition2 = (int)postititons[1]; 
            SequoiaDB.Node slave = null;           
            for (int i = 0; i < 20; i++)
            {
                slave = group.GetSlave(postition1, postition2);
                int getNodeID = slave.NodeID;  
               
                if (!expNodeID.Contains(getNodeID))
                {
                    Assert.Fail("---getNodeID is: " + getNodeID+" postititon1 :"+postition1+ " postition2: "+postition2);
                }
            }
        }       

        private void GetNodePositionAndID(ReplicaGroup group)
        {  
            int masterNodeID = group.GetMaster().NodeID;
            BsonDocument detail = group.GetDetail();
            BsonValue groupinfo = detail.GetValue("Group");
            BsonArray nodes = groupinfo.AsBsonArray;
            
            for (int i = 0; i < nodes.Count; i++)
            {
                BsonDocument obj = nodes[i].ToBsonDocument();                
                int nodeId = (int)obj.GetValue("NodeID");
                //get the master node position
                if (nodeId == masterNodeID)
                {
                    masterNodePosition = i + 1;
                }
                //save nodeID by postition
                nodeIDs.Add(nodeId);   
            }            
        }

        private List<string> GetGroupNames()
        {
            List<string> groupNames = new List<string>();
            DBCursor cursor = sdb.ListReplicaGroups();
            while (cursor.Next() != null)
            {
                BsonDocument actObj = cursor.Current();
                string name = (string)actObj.GetValue("GroupName");
                if (!name.Equals("SYSCoord"))
                {
                    groupNames.Add(name);
                }
            }           
            cursor.Close();
            return groupNames;
        }
    }
}
