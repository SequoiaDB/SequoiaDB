using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB;
using CSharp.TestCommon;
using SequoiaDB.Bson;

namespace CSharp.Monitor
{
    [TestClass]
    public class TestSnapshot22813
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private const string clName = "cl22813";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);
            cl = cs.CreateCollection(clName);
        }

        [TestMethod]
        public void Test22813()
        {
            sdb.GetSnapshot(SDBConst.SDB_SNAP_QUERIES, null, null, null);
            Assert.AreEqual(18, SDBConst.SDB_SNAP_QUERIES);

            sdb.GetSnapshot(SDBConst.SDB_SNAP_LATCHWAITS, null, null, null);
            Assert.AreEqual(19, SDBConst.SDB_SNAP_LATCHWAITS);

            sdb.GetSnapshot(SDBConst.SDB_SNAP_LOCKWAITS, null, null, null);
            Assert.AreEqual(20, SDBConst.SDB_SNAP_LOCKWAITS);
        }

        [TestCleanup()]
        public void TearDown()
        {
            try
            {
                cs.DropCollection(clName);
            }
            finally
            {
                if (sdb != null)
                {
                    sdb.Disconnect();
                }
                Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " end  : " + this.GetType().ToString());
            }                   
        }
    }
}
