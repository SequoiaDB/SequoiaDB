using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB;
using CSharp.TestCommon;
using SequoiaDB.Bson;

namespace CSharp.Sdb
{
    /**
     * TestCase : seqDB-16815
     * test interface:   ListReplicaGroups()
     * author:  chensiqin
     * date:    2018/12/17
     * version: 1.0
    */

    [TestClass]
    public class TestListReplicaGroups16815
    {
        private Sequoiadb sdb = null;

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
            
        }

        [TestMethod]
        public void Test16815()
        {
            try
            {
                sdb.ListReplicaGroups();
            }
            catch (BaseException e)
            {
                Assert.AreEqual(-159, e.ErrorCode);
                return ;
            }
            DBCursor cur = sdb.ListReplicaGroups();
            while (cur.Next() != null)
            {
                BsonDocument doc = cur.Current();
                Assert.IsTrue(doc.Contains("Group"));
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
