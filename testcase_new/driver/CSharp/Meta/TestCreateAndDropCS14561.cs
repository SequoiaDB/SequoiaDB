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
     *              createCollectionSpace (String csName, BsonDocument options) 
     *              getCollectionSpace (String csName) 
     *              dropCollectionSpace (String csName) 
     *              isCollectionSpaceExist (String csName)
     *              1、创建CS，指定options参数，覆盖pageSize、Domain、LobPageSize，检查创建结果正确性； 
     *              2、查询该CS名（getCS（）），检查查询结果正确性； 
     *              3、删除CS，检查删除结果正确性（判断该CS已不存在）；  
     * testcase:    14561
     * author:      chensiqin
     * date:        2018/05/03
    */

    [TestClass]
    public class TestCreateAndDropCS14561
    {
        private Sequoiadb sdb = null;
        private string csName = "csName14561";
        private string domainName = "domain14561";
        private List<string> dataGroupNames = new List<string>();

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
        }

        [TestMethod]
        public void Test14561()
        {
            if (Common.isStandalone(sdb))
            {
                return;
            }
            dataGroupNames = Common.getDataGroupNames(sdb);
            if (dataGroupNames.Count < 1)
            {
                return;
            }
            BsonDocument option = new BsonDocument();
            BsonArray arr = new BsonArray();
            arr.Add(dataGroupNames[0]);
            option.Add("Groups", arr);
            sdb.CreateDomain(domainName, option);

            option = new BsonDocument();
            option.Add("PageSize", SDBConst.SDB_PAGESIZE_4K);
            option.Add("LobPageSize", 4096);
            option.Add("Domain", domainName);
            sdb.CreateCollectionSpace(csName, option);
            CheckCSInfo(csName, option);

            CollectionSpace cs = sdb.GetCollectionSpace(csName);
            Assert.AreEqual(csName, cs.Name);

            sdb.DropCollectionSpace(csName);
            Assert.IsFalse(sdb.IsCollectionSpaceExist(csName));

            sdb.DropDomain(domainName);
        }

        private void CheckCSInfo(string ckCSName, BsonDocument expected)
        {
            DBQuery query = new DBQuery();
            BsonDocument matcher = new BsonDocument();
            BsonDocument actual = new BsonDocument();
            matcher.Add("Name", ckCSName);
            ReplicaGroup cataRg = sdb.GetReplicaGroup("SYSCatalogGroup");
            Sequoiadb cataDB = cataRg.GetMaster().Connect();
            CollectionSpace sysCS = cataDB.GetCollectionSpace("SYSCAT");
            DBCollection sysCL = sysCS.GetCollection("SYSCOLLECTIONSPACES");
            query.Matcher = matcher;
            DBCursor cur = sysCL.Query(query);
            Assert.IsNotNull(cur.Next());
            actual = cur.Current();
            Assert.AreEqual(expected.GetElement("PageSize").Value.ToString(), actual.GetElement("PageSize").Value.ToString());
            Assert.AreEqual(expected.GetElement("LobPageSize").Value.ToString(), actual.GetElement("LobPageSize").Value.ToString());
            Assert.AreEqual(expected.GetElement("Domain").Value.ToString(), actual.GetElement("Domain").Value.ToString());
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
