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
     * description: cs.Alter() modify cs's Name、LobPageSize、PageSize attribute, and chek result
     *              sunch as:
     *                a. alter（{PageSize:4096，LobPageSize：2048}）
     *                b. db.newcs.alter({Alter:[ {Name:"set attributes", Args:{PageSize:4096}}, {Name:"set attributes", Args: {Name:“cs”}}, {Name:"set attributes", Args: {PageSize:8192}}], Options:{IgnoreException:true}} )
     * testcase:    15188
     * author:      chensiqin
     * date:        2018/04/26
    */
    [TestClass]
    public class Meta15188
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private string localCsName = "cs15188";
        private string domainName = "domain15188";
        private List<string> dataGroupNames = new List<string>();

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
            
        }

        [TestMethod]
        public void Test15188() 
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
            cs = sdb.CreateCollectionSpace(localCsName);
            BsonArray alterArray = new BsonArray();
            alterArray.Add(new BsonDocument{
                {"Name","set attributes"},
                {"Args",new BsonDocument{{"PageSize", 4096}}}});
            alterArray.Add(new BsonDocument{
                {"Name","set attributes"},
                {"Args",new BsonDocument{{"LobPageSize", 4096}}}});
            BsonDocument options = new BsonDocument();
            options.Add("Alter", alterArray);
            cs.Alter(options);
            checkCSCataInfo(false);

            BsonDocument option = new BsonDocument();
            BsonArray arr = new BsonArray();
            arr.Add(dataGroupNames[0]);
            arr.Add(dataGroupNames[1]);
            option.Add("Groups", arr);
            sdb.CreateDomain(domainName, option);
            alterArray = new BsonArray();
            alterArray.Add(new BsonDocument{
                {"Name","set attributes"},
                {"Args",new BsonDocument{{"PageSize", 8192}}}});
            alterArray.Add(new BsonDocument{
                {"Name","set attributes"},
                {"Args",new BsonDocument{{"LobPageSize", 8192}}}});
            alterArray.Add(new BsonDocument{
                {"Name","set attributes"},
                {"Args",new BsonDocument{{"Name", "cs"}}}});
            alterArray.Add(new BsonDocument{
                {"Name","set attributes"},
                {"Args",new BsonDocument{{"Domain", domainName}}}});
            options = new BsonDocument();
            options.Add("Alter", alterArray);
            options.Add("Options", new BsonDocument { { "IgnoreException", true } });
            cs.Alter(options);
            checkCSCataInfo(true);

            sdb.DropCollectionSpace(localCsName);
            sdb.DropDomain(domainName);
        }

        public void checkCSCataInfo(bool ignoreException)
        {
            DBQuery query = new DBQuery();
            BsonDocument matcher = new BsonDocument();
            BsonDocument actual = new BsonDocument();
            matcher.Add("Name", localCsName);
            ReplicaGroup cataRg = sdb.GetReplicaGroup("SYSCatalogGroup");
            Sequoiadb cataDB = cataRg.GetMaster().Connect();
            CollectionSpace sysCS = cataDB.GetCollectionSpace("SYSCAT");
            DBCollection sysCL = sysCS.GetCollection("SYSCOLLECTIONSPACES");
            query.Matcher = matcher;
            DBCursor cur = sysCL.Query(query);
            if (ignoreException)
            {
                Assert.IsNotNull(cur.Next());
                actual = cur.Current();
                Assert.AreEqual(8192 + "", actual.GetElement("PageSize").Value.ToString());
                Assert.AreEqual(8192 + "", actual.GetElement("LobPageSize").Value.ToString());
                Assert.AreEqual(domainName, actual.GetElement("Domain").Value.ToString());
            }
            else
            {
                Assert.IsNotNull(cur.Next());
                actual = cur.Current();
                Assert.AreEqual(4096+"", actual.GetElement("PageSize").Value.ToString());
                Assert.AreEqual(4096 + "", actual.GetElement("LobPageSize").Value.ToString());
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
