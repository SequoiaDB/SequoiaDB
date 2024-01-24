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
     * description:  seqDB-16642 seqDB-16643 renamCL
     * testcase:     16642 16643
     * author:       chensiqin
     * date:         2018/11/20
    */
    [TestClass]
    public class RenameCS16642
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);
        }

        [TestMethod()]
        public void Test16642_16643()
        {

            Test16642();
            Test16643();
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

        public void Test16642()
        {
            string localCLName = "cl16642";
            string newCLName = "newcl16642";

            if (cs.IsCollectionExist(localCLName))
            {
                cs.DropCollection(localCLName);
            }
            if (cs.IsCollectionExist(newCLName))
            {
                cs.DropCollection(newCLName);
            }
            try
            {
                cs.RenameCollection(localCLName, newCLName);
                Assert.Fail("expected rename fail !");
            }
            catch (BaseException e)
            {
                Assert.AreEqual(-23, e.ErrorCode);
            }
            Assert.AreEqual(false, cs.IsCollectionExist(localCLName));
            Assert.AreEqual(false, cs.IsCollectionExist(newCLName));
        }

        public void Test16643()
        {
            string localCLName = "cl16643";
            string newCLName = "newcl16643";
            if (cs.IsCollectionExist(localCLName))
            {
                cs.DropCollection(localCLName);
            }
            if (cs.IsCollectionExist(newCLName))
            {
                cs.DropCollection(newCLName);
            }
            cs.CreateCollection(localCLName);
            cs.CreateCollection(newCLName);
            try
            {
                cs.RenameCollection(localCLName, newCLName);
                Assert.Fail("expected rename fail !");
            }
            catch (BaseException e)
            {
                Assert.AreEqual(-22, e.ErrorCode);
            }
            Assert.AreEqual(true, cs.IsCollectionExist(localCLName));
            Assert.AreEqual(true, cs.IsCollectionExist(newCLName));
            //rename  为原名
            try
            {
                cs.RenameCollection(localCLName, localCLName);
                Assert.Fail("expected rename fail !");
            }
            catch (BaseException e)
            {
                Assert.AreEqual(-22, e.ErrorCode);
            }
        }
    }
}
