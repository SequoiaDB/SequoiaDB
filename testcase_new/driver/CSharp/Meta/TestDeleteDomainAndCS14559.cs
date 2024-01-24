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
     *              delete domain and domain not exist
     *              delete cs and cs not exist
     * testcase:    14559
     * author:      chensiqin
     * date:        2018/05/02
    */
    [TestClass]
    public class TestDeleteDomainAndCS14559
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
        public void Test14559()
        {
            try
            {
                sdb.DropDomain("domain14559");
                Assert.Fail("expected thow BaseException but success!");
            }
            catch (BaseException e)
            {
                Assert.AreEqual(-214, e.ErrorCode);
            }

            try
            {
                sdb.DropCollectionSpace("csName14559");
                Assert.Fail("expected thow BaseException but success!");
            }
            catch (BaseException e)
            {
                Assert.AreEqual(-34, e.ErrorCode);
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
