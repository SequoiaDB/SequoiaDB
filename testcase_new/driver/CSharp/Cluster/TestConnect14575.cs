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
     * description:   connect () connect (string username, string password) disconnect () IsValid（）    
     *  	          1、connect连接coord节点（指定用户名和密码），并创建集合，插入数据 
     *  	          2、connect集合所在节点，检查集合信息正确性
     *  	          3、disconnect节点，检查节点是否断连 
     * testcase:    14575
     * author:      chensiqin
     * date:        2019/04/12
     */

    [TestClass]
    public class TestConnect14575
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private string clName = "cl14575";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
        }

        [TestMethod]
        public void Test14576()
        {
            if (Common.IsStandalone(sdb))
            {
                return;
            }
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);
            if (cs.IsCollectionExist(clName))
            {
                cs.DropCollection(clName);
            }
            List<string> rgNames = Common.getDataGroupNames(sdb);
            BsonDocument option = new BsonDocument();
            option.Add("Group", rgNames[0]);
            cl = cs.CreateCollection(clName, option);
            cl.Insert(new BsonDocument("name", 14576));
            ReplicaGroup rg = sdb.GetReplicaGroup(rgNames[0]);
            Node node = rg.GetMaster();
            Sequoiadb dataDB = node.Connect("", "");
            cs = dataDB.GetCollectionSpace(SdbTestBase.csName);
            cl = cs.GetCollection(clName);
            Assert.AreEqual(1, cl.GetCount(null));
            dataDB.Disconnect();
            Assert.AreEqual(false, dataDB.IsValid());
            Assert.AreEqual(true, sdb.IsValid());
               
            string username = "name14576";
            string passwd = "passwd14576";
            sdb.CreateUser(username, passwd);
            rg = sdb.GetReplicaGroup(rgNames[0]);
            dataDB = rg.GetMaster().Connect(username, passwd);
            Assert.AreEqual(true, sdb.IsValid());
            sdb.RemoveUser(username, passwd);
            dataDB.Disconnect();
            Assert.AreEqual(false, dataDB.IsValid());
            
        }

        [TestCleanup()]
        public void TearDown()
        {
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);
            if (cs.IsCollectionExist(clName))
            {
                cs.DropCollection(clName);
            }
            if (sdb != null)
            {
                sdb.Disconnect();
            }
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " end  : " + this.GetType().ToString());
        }
    }
}
