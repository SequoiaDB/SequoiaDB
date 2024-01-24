using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB.Bson;
using SequoiaDB;
using CSharp.TestCommon;

namespace CSharp.Crud.Analyze
{
    /**
     * description: analyze inexistent cl
     * testcase:    14261
     * author:      linsuqiang
     * date:        2018/07/31
     */

    [TestClass]
    public class AnalyzeInexistCl14261
    {
        private Sequoiadb sdb = null;
        private const string csName = "analyze14261";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
            sdb.CreateCollectionSpace(csName);
        }

        [TestMethod]
        public void Test14261()
        {
            try
            {
                const string clFullName = csName + ".inexistent_cl_name_14261";
                sdb.Analyze(new BsonDocument { { "Collection", clFullName } });
            }
            catch (BaseException e)
            {
                Assert.AreEqual(-23, e.ErrorCode);
            }
        }

        [TestCleanup()]
        public void TearDown()
        {
            if (sdb.IsCollectionSpaceExist(csName))
                sdb.DropCollectionSpace(csName);
            if (sdb != null)
            {
                sdb.Disconnect();
            }
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " end  : " + this.GetType().ToString());
        }
    }
}
