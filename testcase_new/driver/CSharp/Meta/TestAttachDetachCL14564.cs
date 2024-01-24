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
     * description:  
     *               createCollection (String collectionName, BsonDocument options) 
     *               attachCollection (String subClFullName, BsonDocument options) 
     *               detachCollection (String subClFullName) 
     *               dropCollection (String collectionName)
     *               已创建CS和多个CL
     *               1、创建主表和子表（createcl中指定options覆盖所有参数，可参考sdb中createcl中options参数），检查创建结果正确性 
     *               2、挂载子表到主表，检查挂载结果正确性 
     *               3、去挂载子表，检查去挂载结果正确性 
     *               4、删除主表和子表所在CS，检查删除结果正确性 
     *               5、判断删除的cl是否存在 
     * testcase:    14564
     * author:      chensiqin
     * date:        2018/05/03
    */

    [TestClass]
    public class TestAttachDetachCL14564
    {

        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private string localCSName = "cs14564";
        private string clName1 = "cl14564_1";
        private string clName2 = "cl14564_2";
        private string clName3 = "cl14564_3";
        private List<string> dataGroupNames = new List<string>();

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
        }

        [TestMethod]
        public void Test14564()
        {
            
            if (Common.isStandalone(sdb))
            {
                return;
            }
            dataGroupNames = Common.getDataGroupNames(sdb);
            //创建多个cl
            cs = sdb.CreateCollectionSpace(localCSName);
            BsonDocument option = new BsonDocument();
            //主表
            option = new BsonDocument {
                {"ShardingKey", new BsonDocument{{"a",1}}},
                {"ShardingType", "range"},
                {"ReplSize", 1},
                {"IsMainCL", true}
            };
            DBCollection mainCL = cs.CreateCollection(clName1, option);
            
            //子表
            option = new BsonDocument {
                {"ShardingKey", new BsonDocument{{"a",1}}},
                {"ShardingType", "hash"},
                {"Partition", 1024},
                {"AutoSplit", true},
                {"CompressionType", "lzw"},
                {"Compressed", true},
                {"AutoIndexId", false},
                {"EnsureShardingIndex", false},
                {"ReplSize", 3},
                {"StrictDataMode", true}
            };
            cs.CreateCollection(clName2, option);
            checkCLAlter(clName2, "subCL", option, "");
            cs.CreateCollection(clName3, option);
            checkCLAlter(clName3, "subCL", option, "");

            option = new BsonDocument {
                {"LowBound", new BsonDocument{{"a",0}}},
                {"UpBound", new BsonDocument{{"a",100}}}
            };
            mainCL.AttachCollection(localCSName + "." + clName2, option);
            option = new BsonDocument {
                {"LowBound", new BsonDocument{{"a",100}}},
                {"UpBound", new BsonDocument{{"a",200}}}
            };
            mainCL.AttachCollection(localCSName + "." + clName3, option);
            
            BsonDocument expected = new BsonDocument();
            expected = new BsonDocument {
                {"ShardingKey", new BsonDocument{{"a",1}}},
                {"ShardingType", "range"},
                {"ReplSize", 1},
                {"IsMainCL", true},
                {"LowBound", new BsonDocument{{"a",100}}},
                {"UpBound", new BsonDocument{{"a",200}}}
            };
            checkCLAlter(clName1, "mainCL", expected, "attach");
            mainCL.DetachCollection(localCSName + "." + clName2);
            mainCL.DetachCollection(localCSName + "." + clName3);
            checkCLAlter(clName1, "mainCL", expected, "");


            sdb.DropCollectionSpace(localCSName);
            Assert.IsFalse(sdb.IsCollectionSpaceExist(localCSName));
            DBCursor cur = sdb.ListCollections();
            List<string> actual = new List<string>();
            while (cur.Next() != null)
            {
                actual.Add(cur.Current().GetElement("Name").Value.ToString());
            }
            Assert.IsFalse(actual.Contains(localCSName + "." + clName1));
            Assert.IsFalse(actual.Contains(localCSName + "." + clName2));
            Assert.IsFalse(actual.Contains(localCSName + "." + clName3));
        }

        private void checkCLAlter(string clName, string clType, BsonDocument expected, string attachType)
        {
            BsonDocument matcher = new BsonDocument();
            BsonDocument actual = new BsonDocument();
            DBCursor cur = null;
            matcher.Add("Name", localCSName + "." + clName);
            cur = sdb.GetSnapshot(8, matcher, null, null);
            Assert.IsNotNull(cur.Next());
            actual = cur.Current();
            if (clType.Equals("mainCL"))
            {
                Console.WriteLine(actual.GetElement("CataInfo").Value.ToString().Equals("[]"));
                Assert.AreEqual(expected.GetElement("ShardingKey").Value.ToString(), actual.GetElement("ShardingKey").Value.ToString());
                Assert.AreEqual(expected.GetElement("ShardingType").Value.ToString(), actual.GetElement("ShardingType").Value.ToString());
                Assert.AreEqual(expected.GetElement("ReplSize").Value.ToString(), actual.GetElement("ReplSize").Value.ToString());
                Assert.AreEqual(expected.GetElement("IsMainCL").Value.ToString(), actual.GetElement("IsMainCL").Value.ToString());

                if (attachType.Equals("attach"))
                {
                    BsonArray arr = (BsonArray)actual.GetElement("CataInfo").Value;
                    for (int i = 0; i < arr.Count; i++)
                    {
                        BsonElement element = arr[i].ToBsonDocument().GetElement("SubCLName");
                        string clFullName1 = localCSName + "." + clName2;
                        string clFullName2 = localCSName + "." + clName3;
                        if(clFullName1.Equals(element.Value.ToString()))
                        {
                            //LowBound,UpBound
                            Assert.AreEqual("{ \"a\" : 0 }", arr[i].ToBsonDocument().GetElement("LowBound").Value.ToString());
                            Assert.AreEqual("{ \"a\" : 100 }", arr[i].ToBsonDocument().GetElement("UpBound").Value.ToString());
                        }
                        else if (clFullName2.Equals(element.Value.ToString()))
                        {
                            Assert.AreEqual("{ \"a\" : 100 }", arr[i].ToBsonDocument().GetElement("LowBound").Value.ToString());
                            Assert.AreEqual("{ \"a\" : 200 }", arr[i].ToBsonDocument().GetElement("UpBound").Value.ToString());
                        }
                        else
                        {
                            Assert.Fail(element.Value.ToString() + ", subcl name is error!");
                        }

                    }
                }
                else
                {
                    Assert.IsTrue(actual.GetElement("CataInfo").Value.ToString().Equals("[]"));
                }
            }
            else
            {
                Assert.AreEqual(expected.GetElement("ShardingKey").Value.ToString(), actual.GetElement("ShardingKey").Value.ToString());
                Assert.AreEqual(expected.GetElement("ShardingType").Value.ToString(), actual.GetElement("ShardingType").Value.ToString());
                Assert.AreEqual(expected.GetElement("Partition").Value.ToString(), actual.GetElement("Partition").Value.ToString());
                Assert.AreEqual(expected.GetElement("AutoSplit").Value.ToString(), actual.GetElement("AutoSplit").Value.ToString());
                Assert.AreEqual(expected.GetElement("CompressionType").Value.ToString(), actual.GetElement("CompressionTypeDesc").Value.ToString());
                Assert.AreEqual("Compressed | NoIDIndex | StrictDataMode", actual.GetElement("AttributeDesc").Value.ToString());
                Assert.AreEqual(expected.GetElement("EnsureShardingIndex").Value.ToString(), actual.GetElement("EnsureShardingIndex").Value.ToString());
                Assert.AreEqual(expected.GetElement("ReplSize").Value.ToString(), actual.GetElement("ReplSize").Value.ToString());
          
            }
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
