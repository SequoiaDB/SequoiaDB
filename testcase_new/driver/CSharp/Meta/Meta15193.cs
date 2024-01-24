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
     *              a. cl.enableCompression() modify CompressionType=lzw and check result
     *              b. disableCompression and check result
     * testcase:    15193
     * author:      chensiqin
     * date:        2018/04/26
    */
    [TestClass]
    public class Meta15193
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private string clName = "cl15193";
        private List<string> dataGroupNames = new List<string>();

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();            
        }

        [TestMethod]
        public void Test15193() 
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
                {"CompressionType", "lzw"}
            };

            cl.EnableCompression(option);
            checkCLAlter(true, option);
            cl.DisableCompression();
            checkCLAlter(false, option);

            cs.DropCollection(clName);
        }

        private void checkCLAlter(bool enableCompression, BsonDocument expected)
        {
            BsonDocument matcher = new BsonDocument();
            BsonDocument actual = new BsonDocument();
            DBCursor cur = null;
            if (enableCompression)
            {
                matcher.Add("Name", SdbTestBase.csName + "." + clName);
                cur = sdb.GetSnapshot(8, matcher, null, null);
                Assert.IsNotNull(cur.Next());
                actual = cur.Current();
                Assert.AreEqual(expected.GetElement("CompressionType").Value.ToString(), actual.GetElement("CompressionTypeDesc").Value.ToString());
                cur.Close();
            }
            else
            {
                matcher.Add("Name", SdbTestBase.csName + "." + clName);
                matcher.Add("CompressionTypeDesc", "lzw");
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
