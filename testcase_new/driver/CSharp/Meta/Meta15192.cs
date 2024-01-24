using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB.Bson;
using SequoiaDB;
using CSharp.TestCommon;

namespace CSharp.Meta
{
    /**
     * description: cl.enableSharding()
     *              a. cl.enableSharding() modify shardingKey、shardingType、partition、EnsureShardingIndex and check result
     *              b. disableSharding and check result
     * testcase:    15192
     * author:      chensiqin
     * date:        2018/04/26
    */
    [TestClass]
    public class Meta15192
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private string clName = "cl15192";
        private List<string> dataGroupNames = new List<string>();

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();            
        }

        [TestMethod]
        public void Test15192() 
        {
            if (Common.isStandalone(sdb))
            {
                return;
            }
            dataGroupNames = Common.getDataGroupNames(sdb);
            if (dataGroupNames.Count < 3)
            {
                return;
            }
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);
            cl = cs.CreateCollection(clName);
            BsonDocument option = new BsonDocument();
            option = new BsonDocument {
                {"ShardingKey", new BsonDocument{{"a",1}}},
                {"ShardingType", "hash"},
                {"Partition", 2048},
                {"EnsureShardingIndex", true}
            };
            cl.EnableSharding(option);
            checkCLAlter(true, option);

            cl.DisableSharding();
            checkCLAlter(false, new BsonDocument());

            cs.DropCollection(clName);
        }

        private void checkCLAlter(bool enableSharding, BsonDocument expected)
        {
            BsonDocument matcher = new BsonDocument();
            BsonDocument actual = new BsonDocument();
            DBCursor cur = null;
            if (enableSharding)
            {
                matcher.Add("Name", SdbTestBase.csName + "." + clName);
                cur = sdb.GetSnapshot(8, matcher, null, null);
                Assert.IsNotNull(cur.Next());
                actual = cur.Current();
                Assert.AreEqual(expected.GetElement("ShardingKey").Value.ToString(), actual.GetElement("ShardingKey").Value.ToString());
                Assert.AreEqual(expected.GetElement("ShardingType").Value.ToString(), actual.GetElement("ShardingType").Value.ToString());
                Assert.AreEqual(expected.GetElement("Partition").Value.ToString(), actual.GetElement("Partition").Value.ToString());
                Assert.AreEqual(expected.GetElement("EnsureShardingIndex").Value.ToString(), actual.GetElement("EnsureShardingIndex").Value.ToString());
                cur.Close();
            }
            else
            {
                matcher.Add("Name", SdbTestBase.csName + "." + clName);
                matcher.Add("EnsureShardingIndex", true);
                cur = sdb.GetSnapshot(8, matcher, null, null);
                Assert.IsNull(cur.Next());
                cur.Close();
            }
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
