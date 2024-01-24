using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB.Bson;
using SequoiaDB;
using CSharp.TestCommon;

namespace CSharp.Monitor
{
    /**
     * description:  
     *                GetList（int listType，BsonDocument matcher，BsonDocument selector，BsonDocument orderBy）；GetList（int listType）
     *                SDBConst.SDB_LIST_CONTEXTS 
     *                SDBConst.SDB_LIST_CONTEXTS_CURRENT 
     *                SDBConst.SDB_LIST_SESSIONS 
     *                SDBConst.SDB_LIST_SESSIONS_CURRENT 
     *                SDBConst.SDB_LIST_COLLECTIONS 
     *                SDBConst.SDB_LIST_COLLECTIONSPACES 
     *                SDBConst.SDB_LIST_STORAGEUNITS 
     *                SDBConst.SDB_LIST_GROUPS 
     *                SDBConst.SDB_LIST_STOREPROCEDURES 
     *                SDBConst.SDB_LIST_DOMAINS 
     *                SDBConst.SDB_LIST_TASKS 
     *                SDBConst.SDB_LIST_TRANSACTIONS 
     *                SDBConst.SDB_LIST_TRANSACTIONS_CURRENT
     *                1.创建cs.cl，并在cl中插入数据
     *                2.使用get_list接口获取信息，分别验证如下场景：
     *                  a、验证GetList（int listType）接口，覆盖所有listType取值
     *                  b、只指定listType，其它参数值为null
     *                  c、指定listType、matcher、selector、order_by等参数，其中listType覆盖所有值
     *                3、检查查询结果
     * testcase:     seqDB-13623
     * author:       chensiqin
     * date:         2019/03/27
    */

    [TestClass]
    public class TestGetList13623
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private string localCSName = "cs13623";
        private string clName = "cl13623";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            
            sdb.Connect();
           

        }
        [TestMethod]
        public void Test13623()
        {
            if (Common.IsStandalone(sdb) == true)
            {
                return;
            }

            DBCursor cursor = null;
            BsonDocument doc = null;
            List<string> actual = new List<string>();
            List<string> dataList = Common.getDataGroupNames(sdb);

            //SDBConst.SDB_LIST_CONTEXTS
            BsonDocument selector = new BsonDocument();
            selector.Add("Contexts", new BsonDocument("$include", 1));
            cursor = sdb.GetList(SDBConst.SDB_LIST_CONTEXTS, null, selector, null);
            while (cursor.Next() != null)
            {
                doc = cursor.Current();
                Assert.AreEqual(true, doc.Contains("Contexts"));
            }
            cursor.Close();

            //SDBConst.SDB_LIST_CONTEXTS_CURRENT
            cursor = sdb.GetList(SDBConst.SDB_LIST_CONTEXTS_CURRENT, null, selector, null);
            while (cursor.Next() != null)
            {
                doc = cursor.Current();
                Assert.AreEqual(true, doc.Contains("Contexts"));
            }
            cursor.Close();

            //SDBConst.SDB_LIST_SESSIONS
            selector = new BsonDocument();
            selector.Add("SessionID", new BsonDocument("$include", 1));
            cursor = sdb.GetList(SDBConst.SDB_LIST_SESSIONS, null, selector, null);
            while (cursor.Next() != null)
            {
                doc = cursor.Current();
                Assert.AreEqual(true, doc.Contains("SessionID"));
            }
            cursor.Close();

            //SDBConst.SDB_LIST_SESSIONS_CURRENT
            cursor = sdb.GetList(SDBConst.SDB_LIST_SESSIONS_CURRENT, null, selector, null);
            while (cursor.Next() != null)
            {
                doc = cursor.Current();
                Assert.AreEqual(true, doc.Contains("SessionID"));
            }
            cursor.Close();

            //SDBConst.SDB_LIST_COLLECTIONS
            cs = sdb.CreateCollectionSpace(localCSName);
            cs.CreateCollection(clName, new BsonDocument("Group", dataList[0]));

            string expectedStr = localCSName + "." + clName;
            BsonDocument matcher = new BsonDocument();
            matcher.Add("Name", expectedStr);
            selector = new BsonDocument();
            selector.Add("Name", new BsonDocument("$include", 1));
            cursor = sdb.GetList(SDBConst.SDB_LIST_COLLECTIONS, matcher, selector, null);
            while (cursor.Next() != null)
            {
                doc = cursor.Current();
            }
            Assert.AreEqual(expectedStr, doc.GetElement("Name").Value.ToString());
            cursor.Close();

            //SDBConst.SDB_LIST_COLLECTIONSPACES
            expectedStr = localCSName;
            matcher = new BsonDocument();
            matcher.Add("Name", expectedStr);
            selector = new BsonDocument();
            selector.Add("Name", new BsonDocument("$include", 1));
            cursor = sdb.GetList(SDBConst.SDB_LIST_COLLECTIONSPACES, matcher, selector, null);
            while (cursor.Next() != null)
            {
                doc = cursor.Current();
            }
            Assert.AreEqual(expectedStr, doc.GetElement("Name").Value.ToString());
            cursor.Close();

            //SDBConst.SDB_LIST_STORAGEUNITS 
            
            ReplicaGroup rg = sdb.GetReplicaGroup(dataList[0]);
            Sequoiadb dataDB = rg.GetMaster().Connect();
            cursor = dataDB.GetList(SDBConst.SDB_LIST_STORAGEUNITS, matcher, null, null);
            int count = 0;
            while (cursor.Next() != null)
            {
                doc = cursor.Current();
                count++;
            }
            Assert.AreEqual(expectedStr, doc.GetElement("Name").Value.ToString());
            Assert.AreEqual(true, doc.Contains("CollectionHWM"));
            Assert.AreEqual(true, doc.Contains("ID"));
            Assert.AreEqual(1, count);
            cursor.Close();
            dataDB.Disconnect();

            //SDBConst.SDB_LIST_GROUPS
            cursor = sdb.GetList(SDBConst.SDB_LIST_GROUPS);
            count = 0;
            while (cursor.Next() != null)
            {
                doc = cursor.Current();
                count++;
                Assert.AreEqual(true, doc.Contains("Group"));
                Assert.AreEqual(true, doc.Contains("GroupName"));
                Assert.AreEqual(true, doc.Contains("GroupID"));

            }
            Assert.AreEqual(1, count.CompareTo(0));
            cursor.Close();

            //SDBConst.SDB_LIST_STOREPROCEDURES  //TODO:SEQUOIADBMAINSTREAM-4303

            //SDBConst.SDB_LIST_DOMAINS
            string domainName = "domain13623";
            if (sdb.IsDomainExist(domainName))
            {
                sdb.DropDomain(domainName);
            }
            sdb.CreateDomain(domainName, new BsonDocument("Groups", new BsonArray().Add(dataList[0])));
            matcher = new BsonDocument();
            matcher.Add("Name", domainName);
            cursor = sdb.GetList(SDBConst.SDB_LIST_DOMAINS, matcher, null, null);
            count = 0;
            while (cursor.Next() != null)
            {
                doc = cursor.Current();
                count++;
            }
            Assert.AreEqual(domainName, doc.GetElement("Name").Value.ToString());
            Assert.AreEqual(1, count);
            cursor.Close();
            sdb.DropDomain(domainName);
            /* 
               SDBConst.SDB_LIST_TASKS 
               SDBConst.SDB_LIST_TRANSACTIONS 
               SDBConst.SDB_LIST_TRANSACTIONS_CURRENT*/

            //SEQUOIADBMAINSTREAM-4998
            //SDB_LIST_USERS
            sdb.CreateUser("user13623", "user13623");
            sdb.CreateUser("user13623_1", "user13623_1");
            sdb.CreateUser("user13623_2", "user13623_2");
            cursor = sdb.GetList(SDBConst.SDB_LIST_USERS, null, null, null, null, 1, 1);
            count = 0;
            while (cursor.Next() != null)
            {
                doc = cursor.Current();
                count++;
                Assert.AreEqual("user13623_1", doc.GetElement("User").Value.ToString());
            }
            cursor.Close();
            Assert.AreEqual(1, count);
            sdb.RemoveUser("user13623", "user13623");
            sdb.RemoveUser("user13623_1", "user13623_1");
            sdb.RemoveUser("user13623_2", "user13623_2");
            //SDB_LIST_SVCTASKS
            cursor = sdb.GetList(SDBConst.SDB_LIST_SVCTASKS, null, null, null, null, 0, 1);
            count = 0;
            while (cursor.Next() != null)
            {
                doc = cursor.Current();
                count++;
                Assert.IsTrue(doc.ToString().Contains("\"TaskID\" : 0"));
                Assert.IsTrue(doc.ToString().Contains("\"TaskName\" : \"Default\""));
            }
            cursor.Close();
            Assert.AreEqual(1, count);

            List<string> dataGroups = Common.getDataGroupNames(sdb);
            string backupName1 = "backup13623";
            BsonDocument option1 = new BsonDocument();
            option1.Add("Name", backupName1);
            option1.Add("GroupName", new BsonArray(dataGroups));
            sdb.Backup(option1);
            cursor = sdb.GetList(SDBConst.SDB_LIST_BACKUPS, null, null, null);
            count = 0;
            while (cursor.Next() != null)
            {
                doc = cursor.Current();
                count++;
                Assert.IsTrue(doc.ToString().Contains("\"Name\" : \"backup13623\""));
            }
            Assert.AreEqual(dataGroups.Count(), count);
            cursor.Close();
        }

        [TestCleanup()]
        public void TearDown()
        {
            BsonDocument removeOption = new BsonDocument();
            removeOption.Add("Name", "backup13623");
            try
            {
                sdb.RemoveBackup(removeOption);
            }
            catch (BaseException e)
            {
                if (e.ErrorCode != -264)
                    Assert.Fail("clear env failed, errMsg:" + e.Message);
            }
            if (sdb.IsCollectionSpaceExist(localCSName))
            {
                sdb.DropCollectionSpace(localCSName);
            }
            if (sdb != null)
            {
                sdb.Disconnect();
            }
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " end  : " + this.GetType().ToString());
        }
    }
}
