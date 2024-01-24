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
     * description:  createCollection (String collectionName, BsonDocument options) alterCollection (BsonDocument options) dropCollection (String collectionName)
     *              1、创建普通表 
     *              2、修改cl，指定分区属性，其中options覆盖所有参数 
     *              3、检查修改后cl信息 
     *              4、删除cl，检查删除结果正确性 
     * testcase:    14563
     * author:      chensiqin
     * date:        2018/05/03
    */

    [TestClass]
    public class TestCL14563
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private string clName = "cl14563";
        private List<string> dataGroupNames = new List<string>();

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
        }

        [TestMethod]
        public void Test14563()
        {
            
            if (Common.isStandalone(sdb))
            {
                return;
            }
            dataGroupNames = Common.getDataGroupNames(sdb);
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
                {"Args",new BsonDocument{{"AutoIndexId", false}}}});
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
                {"AutoIndexId", false},
                {"EnsureShardingIndex", false},
                {"ReplSize", 3},
                {"StrictDataMode", true}
            };
            checkCLAlter(expected);

            cs.DropCollection(clName);

            try
            {
                cs.GetCollection(clName);
            }
            catch (BaseException e)
            {
                Assert.AreEqual(-23, e.ErrorCode);
            }
        }

        private void checkCLAlter(BsonDocument expected)
        {
            BsonDocument matcher = new BsonDocument();
            BsonDocument actual = new BsonDocument();
            DBCursor cur = null;
            matcher.Add("Name", SdbTestBase.csName + "." + clName);
            cur = sdb.GetSnapshot(8, matcher, null, null);
            Assert.IsNotNull(cur.Next());
            actual = cur.Current();
            Assert.AreEqual(expected.GetElement("ShardingKey").Value.ToString(), actual.GetElement("ShardingKey").Value.ToString());
            Assert.AreEqual(expected.GetElement("ShardingType").Value.ToString(), actual.GetElement("ShardingType").Value.ToString());
            Assert.AreEqual(expected.GetElement("Partition").Value.ToString(), actual.GetElement("Partition").Value.ToString());
            Assert.AreEqual(expected.GetElement("AutoSplit").Value.ToString(), actual.GetElement("AutoSplit").Value.ToString());
            Assert.AreEqual(expected.GetElement("CompressionType").Value.ToString(), actual.GetElement("CompressionTypeDesc").Value.ToString());
            Assert.AreEqual("Compressed | NoIDIndex | StrictDataMode", actual.GetElement("AttributeDesc").Value.ToString());
            Assert.AreEqual(expected.GetElement("EnsureShardingIndex").Value.ToString(), actual.GetElement("EnsureShardingIndex").Value.ToString());
            Assert.AreEqual(expected.GetElement("ReplSize").Value.ToString(), actual.GetElement("ReplSize").Value.ToString());
            cur.Close();
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
