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
    * description: GetSnapshot ( int  snapType,
     *                           BsonDocument  matcher,
     *                           BsonDocument selector,
     *                           BsonDocument  orderBy,
     *                           BsonDocument hint,
     *                           long  skipRows,
     *                           long  returnRows)             
    * testcase:    15759
    * author:      csq
    * date:        2018/09/07
    */

    [TestClass]
    public class TestGetSnapshot15759
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
        public void TestSnapshot15759()
        {
            BsonDocument matcher = new BsonDocument();
            matcher.Add("IsPrimary", true);
            matcher.Add("Status", "Normal");
            BsonDocument selector = new BsonDocument();
            selector.Add("ServiceStatus", 1);
            selector.Add("IsPrimary", 1);
            BsonDocument orderBy = new BsonDocument();
            orderBy.Add("NodeName", 1);
            BsonDocument hint = new BsonDocument();
            DBCursor cursor = sdb.GetSnapshot(SDBConst.SDB_SNAP_HEALTH, matcher, selector, orderBy, hint, 0, 1);
            int num = 0;
            while (cursor.Next() != null)
            {
                num++;
                Console.WriteLine();
                Assert.AreEqual("{ \"IsPrimary\" : true, \"ServiceStatus\" : true }", cursor.Current().ToString());
            }
            Assert.AreEqual(1, num);
        }

        [TestCleanup()]
        public void TearDown()
        {
            if (sdb != null)
            {
                sdb.Disconnect();
                Assert.AreEqual(false, sdb.IsValid());
            }
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " end  : " + this.GetType().ToString());
        }

    }
}
