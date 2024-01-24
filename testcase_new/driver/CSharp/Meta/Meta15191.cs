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
     * description: cl.setAttributes() modify cl attribute and check
     *              a. shardingKey、shardingType、partition、EnsureShardingIndex、AutoSplit
     *              b. Compression、compressed
     *              c. AutoIndexId、EnsureShardingIndex
     *              d. Size、Max、OverWrite
     *              e. ReplSize、StrictDataMode
     * testcase:    15191
     * author:      chensiqin
     * date:        2018/04/26
    */
    [TestClass]
    public class Meta15191
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private CollectionSpace localcs = null; 
        private DBCollection cl = null;
        private DBCollection localcl = null; 
        private string localCsName = "cs15191"; 
        private string clName = "cl15191";
        private List<string> dataGroupNames = new List<string>();

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
        }

        [TestMethod]
        public void Test15191() 
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
            //cappedCL
            BsonDocument clop = new BsonDocument();
            clop.Add("Capped", true);
            localcs = sdb.CreateCollectionSpace(localCsName, clop);
            clop = new BsonDocument();
            clop.Add("Size", 1024);
            clop.Add("Max", 1000000);
            clop.Add("AutoIndexId", false);
            clop.Add("Capped", true);
            localcl = localcs.CreateCollection(clName, clop);
            BsonDocument option = new BsonDocument();
            option = new BsonDocument {
                {"Size", 1024},
                {"Max", 10000000},
                {"OverWrite", false}
            };
            localcl.SetAttributes(option);
            checkCLAlter("cappedcl", option);

            //normal cl
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);
            cl = cs.CreateCollection(clName);
            option = new BsonDocument();
            option = new BsonDocument {
                {"ShardingKey", new BsonDocument{{"a",1}}},
                {"ShardingType", "hash"},
                {"Partition", 1024},
                {"AutoSplit", true},
                {"CompressionType", "lzw"},
                {"Compressed", true},
                {"EnsureShardingIndex", false},
                {"ReplSize", 2},
                {"StrictDataMode", true}
            };
            cl.SetAttributes(option);
            checkCLAlter("normal", option);

            sdb.DropCollectionSpace(localCsName);
            cs.DropCollection(clName);
        }

        private void checkCLAlter(string cltype, BsonDocument expected)
        {
            BsonDocument matcher = new BsonDocument();
            BsonDocument actual = new BsonDocument();
            DBCursor cur = null;
            if (cltype.Equals("normal"))
            {
                matcher.Add("Name", SdbTestBase.csName + "." + clName);
                cur = sdb.GetSnapshot(8, matcher, null, null);
                Assert.IsNotNull(cur.Next());
                actual = cur.Current();
                Assert.AreEqual(expected.GetElement("ShardingKey").Value.ToString(), actual.GetElement("ShardingKey").Value.ToString());
                Assert.AreEqual(expected.GetElement("ShardingType").Value.ToString(), actual.GetElement("ShardingType").Value.ToString());
                Assert.AreEqual(expected.GetElement("Partition").Value.ToString(), actual.GetElement("Partition").Value.ToString());
                Assert.AreEqual(expected.GetElement("AutoSplit").Value.ToString(), actual.GetElement("AutoSplit").Value.ToString());
                Assert.AreEqual(expected.GetElement("CompressionType").Value.ToString(), actual.GetElement("CompressionTypeDesc").Value.ToString());
                Assert.AreEqual("Compressed | StrictDataMode", actual.GetElement("AttributeDesc").Value.ToString());
                Assert.AreEqual(expected.GetElement("EnsureShardingIndex").Value.ToString(), actual.GetElement("EnsureShardingIndex").Value.ToString());
                Assert.AreEqual(expected.GetElement("ReplSize").Value.ToString(), actual.GetElement("ReplSize").Value.ToString());
                cur.Close();
            }
            else
            {
                matcher.Add("Name", localCsName+"."+clName);
                cur = sdb.GetSnapshot(8, matcher, null, null);
                Assert.IsNotNull(cur.Next());
                actual = cur.Current();
                long expectSize = 1024 * 1024 * 1024;//固定集合的Size单位默认是M，因此此处需要做单位转换
                Assert.AreEqual(expectSize+"", actual.GetElement("Size").Value.ToString());
                Assert.AreEqual(expected.GetElement("Max").Value.ToString(), actual.GetElement("Max").Value.ToString());
                Assert.AreEqual(expected.GetElement("OverWrite").Value.ToString(), actual.GetElement("OverWrite").Value.ToString());
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
