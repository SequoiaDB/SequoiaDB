using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB;

namespace CSharp.TestCommon
{
    /// <summary>
    /// Test 的摘要说明
    /// </summary>
    /// 
    [TestClass()]
    public class SdbTestBase
    {
        public static String coordUrl;
        public static String csName;
        public static String reservedDir = "/opt/sequoiadb/database";
        public static String workDir = "/tmp/csharptest";
        public static int reservedPortBegin = 26000;
        public static int reservedPortEnd = 27000;
        
        public static TestContext testContextInstance;
        public static Config config = null;

        public TestContext TestContext
        {
            get
            {
                return testContextInstance;
            }
            set
            {
                testContextInstance = value;
            }
        }

        #region 附加测试特性
        //
        // 编写测试时，可以使用以下附加特性:
        //
        // 在运行类中的第一个测试之前使用 ClassInitialize 运行代码
        // [ClassInitialize()]
        // public static void MyClassInitialize(TestContext testContext) { }
        //
        // 在类中的所有测试都已运行之后使用 ClassCleanup 运行代码
        // [ClassCleanup()]
        // public static void MyClassCleanup() { }
        //
        // 在运行每个测试之前，使用 TestInitialize 来运行代码
        // [TestInitialize()]
        // public void MyTestInitialize() { }
        //
        // 在每个测试运行完之后，使用 TestCleanup 来运行代码
        // [TestCleanup()]
        // public void MyTestCleanup() { }
        //
        [ClassInitialize()]
        public static void SequoiadbInitialize(TestContext testContext)
        {
             testContextInstance = testContext;
            
             //Console.WriteLine()            
        }
        
        #endregion


        
        [AssemblyInitialize()]
        public static void AssemblyInit(TestContext testContext)
        {
          /*  if (config == null)
                config = new Config();*/

            if (config == null)
                config = new Config();
            csName = config.conf.CHANGEDPREFIX;
            coordUrl = config.conf.Coord;     
            
            Sequoiadb sdb = new Sequoiadb(coordUrl);
            sdb.Connect(config.conf.UserName, config.conf.Password);
            if (sdb.IsCollectionSpaceExist(csName))
                sdb.DropCollectionSpace(csName);
            sdb.CreateCollectionSpace(csName);
            sdb.Disconnect();
        }

        [AssemblyCleanup()]
        public static void AssemblyClean()
        {
            Sequoiadb sdb = new Sequoiadb(coordUrl);
            sdb.Connect(config.conf.UserName, config.conf.Password);
            if (sdb.IsCollectionSpaceExist(csName))
               // sdb.DropCollectionSpace(csName);
            sdb.Disconnect();

        }
    }
}

