using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB.Bson;
using SequoiaDB;
using CSharp.TestCommon;

namespace CSharp.TestBaseException
{
    /**
     * description:  get不存在的cl，查看getLastErrObj返回的错误对象
     * testcase:     seqDB-16533
     * author:       chensiqin
     * date:         2018/11/13
    */
    [TestClass]
    public class ExceptionTest16533
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
        public void Test16533()
        {
            try
            {
                if (!sdb.IsCollectionSpaceExist(SdbTestBase.csName))
                {
                    sdb.CreateCollectionSpace(SdbTestBase.csName);
                }
            }
            catch (BaseException e)
            {
                Assert.AreEqual(-33, e.ErrorCode);
            }
            CollectionSpace cs = sdb.GetCollectionSpace(SdbTestBase.csName);
            try
            {
                cs.GetCollection("bar"); 
            }
            catch (BaseException e)
            {
                Assert.AreEqual(-23, e.ErrorCode);
                Assert.AreEqual("{ \"errno\" : -23, \"description\" : \"Collection does not exist\", \"detail\" : \"\" }", e.ErrorObject.ToString());
            }

            //get 不存在的cs
            try
            {
                sdb.GetCollectionSpace("notexistcs");
            }
            catch (BaseException e)
            {
                Assert.AreEqual(-34, e.ErrorCode);
                Assert.AreEqual("{ \"errno\" : -34, \"description\" : \"Collection space does not exist\", \"detail\" : \"\" }", e.ErrorObject.ToString());
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
