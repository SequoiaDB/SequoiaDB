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
     *                 	1、插入少量数据；
     *                 	2、直连数据主节点查询所有记录；
     *                 	3、db.resetSnapshot({Type: "sessions"})清空会话快照信息；
     *                 	4、检查结果；
     *                 	5、设置options为null,执行重置快照；
     *                 	6、检查结果；
     * testcase:     seqDB-14578
     * author:       chensiqin
     * date:         2019/03/28
    */

    [TestClass]
    public class TestResetSnapshot14578
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private string localCSName = "cs14578";
        private string clName = "cl14578";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
        }

        [TestMethod]
        public void TestMethod1()
        {
            if (Common.IsStandalone(sdb) == true)
            {
                return;
            }
            List<string> dataList = Common.getDataGroupNames(sdb);
            if (sdb.IsCollectionSpaceExist(localCSName))
            {
                sdb.DropCollectionSpace(localCSName);
            }
            cs = sdb.CreateCollectionSpace(localCSName);
            cl = cs.CreateCollection(clName, new BsonDocument("Group", dataList[0]));
            cl.Insert(new BsonDocument("name14578",1));
            ReplicaGroup rg = sdb.GetReplicaGroup(dataList[0]);
            Sequoiadb datadb = rg.GetMaster().Connect();
            cs = datadb.GetCollectionSpace(localCSName);
            cl = cs.GetCollection(clName);
            DBCursor cur = cl.Query();
            while (cur.Next() != null)
            {
                cur.Current();
            }
            cur.Close();
            BsonDocument option = new BsonDocument("Type", "sessions");
            sdb.ResetSnapshot(option);
            Assert.IsFalse(IsSnapClean(datadb, SDBConst.SDB_SNAP_DATABASE, null));
            Assert.IsTrue(IsSnapClean(datadb, SDBConst.SDB_SNAP_SESSIONS, new BsonDocument("Type", "Agent")));
            datadb.Disconnect();

            sdb.ResetSnapshot(null);
        }

        private bool IsSnapClean(Sequoiadb dataDB, int type, BsonDocument matcher)
        {
            DBCursor cur = dataDB.GetSnapshot(type, matcher, null, null);
            BsonDocument rec = cur.Next();
            long totalRead = (long)rec.GetElement("TotalRead").Value;
            cur.Close();
            return (totalRead == 0);
        }

        [TestCleanup()]
        public void TearDown()
        {
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
