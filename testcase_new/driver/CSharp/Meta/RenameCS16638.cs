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
     * description:  seqDB-16638 seqDB-16639 renamCS
     * testcase:     16638 16639
     * author:       chensiqin
     * date:         2018/11/20
    */
    [TestClass]
    public class RenameCS16638
    {
        private Sequoiadb sdb = null;

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
        }

        [TestMethod()]
        public void Test16638_16639()
        {

            Test16638();
            Test16639();
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

        public void Test16638()
        {
            string localCSName = "cs16638";
            string newCSName = "newcs16638";
            
            if (sdb.IsCollectionSpaceExist(localCSName))
            {
                sdb.DropCollectionSpace(localCSName);
            }
            if (sdb.IsCollectionSpaceExist(newCSName))
            {
                sdb.DropCollectionSpace(newCSName);
            }
            try
            {
                sdb.RenameCollectionSpace(localCSName, newCSName);
                Assert.Fail("expected rename fail !");
            }
            catch (BaseException e)
            {
                Assert.AreEqual(-34, e.ErrorCode);
            }
            Assert.AreEqual(false, sdb.IsCollectionSpaceExist(localCSName));
            Assert.AreEqual(false, sdb.IsCollectionSpaceExist(newCSName));
        }

        public void Test16639()
        {
            string localCSName = "cs16639";
            string newCSName = "newcs16639";
            if (sdb.IsCollectionSpaceExist(localCSName))
            {
                sdb.DropCollectionSpace(localCSName);
            }
            if (sdb.IsCollectionSpaceExist(newCSName))
            {
                sdb.DropCollectionSpace(newCSName);
            }
            sdb.CreateCollectionSpace(localCSName);
            sdb.CreateCollectionSpace(newCSName);
            try
            {
                sdb.RenameCollectionSpace(localCSName, newCSName);
                Assert.Fail("expected rename fail !");
            }
            catch (BaseException e)
            {
                Assert.AreEqual(-33, e.ErrorCode);
            }
            Assert.AreEqual(true, sdb.IsCollectionSpaceExist(localCSName));
            Assert.AreEqual(true, sdb.IsCollectionSpaceExist(newCSName));
            //rename  为原名
            try
            {
                sdb.RenameCollectionSpace(localCSName, localCSName);
                Assert.Fail("expected rename fail !");
            }
            catch (BaseException e)
            {
                Assert.AreEqual(-33, e.ErrorCode);
            }
        }
    }
}
