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
     * description: cl.Alter() modify cl attribute and check
     *              a. shardingKey、shardingType、partition、EnsureShardingIndex、AutoSplit
     *              b. Compression、compressed
     *              c. AutoIndexId、EnsureShardingIndex
     *              d. Size、Max、OverWrite
     *              e. ReplSize、StrictDataMode
     *              1. when IgnoreException:false,then cl.alter and check result
     *              2. when IgnoreException:true,then cl.alter and check result
     * testcase:    15190
     * author:      chensiqin
     * date:        2018/04/26
    */
    [TestClass]
    public class Meta15190
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private CollectionSpace localcs = null;
        private DBCollection cl = null;
        private DBCollection localcl = null;
        private string localCsName = "cs15190";
        private string clName = "cl15190";
        private List<string> dataGroupNames = new List<string>();

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
        }

        [TestMethod]
        public void Test15190()
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
            TestCappedCL();
            TestNormalCL();

        }

        public void TestNormalCL()
        {
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);
            cl = cs.CreateCollection(clName);
            BsonArray alterArray = new BsonArray();
            alterArray.Add(new BsonDocument{
                {"Name","enable sharding"},
                {"Args",new BsonDocument{
                    {"ShardingKey", new BsonDocument{{"a",1}}},
                    {"ShardingType", "hash"},
                    {"Partition", 4096},
                    {"EnsureShardingIndex", false},
                    {"AutoSplit", true}}}});
            alterArray.Add(new BsonDocument{
                {"Name","enable compression"},
                {"Args",new BsonDocument{
                    {"CompressionType", "lzw"}}}});
            alterArray.Add(new BsonDocument{
                {"Name","set attributes"},
                {"Args",new BsonDocument{{"ReplSize", 3}}}});
            alterArray.Add(new BsonDocument{
                {"Name","set attributes"},
                {"Args",new BsonDocument{{"StrictDataMode", true}}}});

            BsonDocument options = new BsonDocument();
            options.Add("Alter", alterArray);
            cl.Alter(options);

            BsonDocument expected = new BsonDocument();
            expected = new BsonDocument {
                {"ShardingKey", new BsonDocument{{"a",1}}},
                {"ShardingType", "hash"},
                {"Partition", 4096},
                {"AutoSplit", true},
                {"CompressionType", "lzw"},
                {"Compressed", true},
                {"EnsureShardingIndex", false},
                {"ReplSize", 3},
                {"StrictDataMode", true}
            };
            checkCLAlter("normal", expected);
            cs.DropCollection(clName);

            //IgnoreExceptions true
            cl = cs.CreateCollection(clName);
            alterArray.Add(new BsonDocument{
                {"Name","set attributes"},
                {"Args",new BsonDocument{{"Name", "clname"}}}});
            options = new BsonDocument();
            options.Add("Options", new BsonDocument { { "IgnoreException", true } });
            options.Add("Alter", alterArray);
            cl.Alter(options);
            checkCLAlter("normal", expected);
            cs.DropCollection(clName);
        }

        public void TestCappedCL()
        {
            //cappedCL IgnoreExceptions default
            BsonDocument clop = new BsonDocument();
            clop.Add("Capped", true);
            localcs = sdb.CreateCollectionSpace(localCsName, clop);
            clop = new BsonDocument();
            clop.Add("Size", 1024);
            clop.Add("Max", 1000000);
            clop.Add("AutoIndexId", false);
            clop.Add("Capped", true);
            localcl = localcs.CreateCollection(clName, clop);
            BsonArray alterArray = new BsonArray();
            alterArray.Add(new BsonDocument{
                {"Name","set attributes"},
                {"Args",new BsonDocument{{"Size", 1024}}}});
            alterArray.Add(new BsonDocument{
                {"Name","set attributes"},
                {"Args",new BsonDocument{{"Max", 10000000}}}});
            alterArray.Add(new BsonDocument{
                {"Name","set attributes"},
                {"Args",new BsonDocument{{"OverWrite", false}}}});
            BsonDocument options = new BsonDocument();
            options.Add("Alter", alterArray);
            localcl.Alter(options);
            BsonDocument expected = new BsonDocument();
            expected = new BsonDocument {
                {"Size", 1024},
                {"Max", 10000000},
                {"OverWrite", false}
            };
            checkCLAlter("cappedcl", expected);

            //cappedCL IgnoreExceptions true
            localcs.DropCollection(clName);
            localcl = localcs.CreateCollection(clName, clop);
            alterArray.Add(new BsonDocument{
                {"Name","set attributes"},
                {"Args",new BsonDocument{{"Name", "clname"}}}});
            options = new BsonDocument();
            options.Add("Options", new BsonDocument { { "IgnoreException", true } });
            options.Add("Alter", alterArray);
            localcl.Alter(options);
            checkCLAlter("cappedcl", expected);
            sdb.DropCollectionSpace(localCsName);
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
                matcher.Add("Name", localCsName + "." + clName);
                cur = sdb.GetSnapshot(8, matcher, null, null);
                Assert.IsNotNull(cur.Next());
                actual = cur.Current();
                long expectSize = 1024 * 1024 * 1024;
                Assert.AreEqual(expectSize + "", actual.GetElement("Size").Value.ToString());
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
