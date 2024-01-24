using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using SequoiaDB;
using CSharp.TestCommon;
using SequoiaDB.Bson;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace CSharp.Crud.Lob
{
    /**
    * description:   seqDB-22582:以SDB_LOB_SHAREREAD模式读取lob 
    * author:        Zhao Xiaoni
    * date:          2020/8/8
    */

    [TestClass]
    public class Lob22582
    {
        private String clName = "cl_22582";
        private Sequoiadb sdb = null;
        private DBCollection cl = null;

        [TestInitialize]
        public void setup() 
        {
            Console.WriteLine( DateTime.Now.ToString( "yyyy-MM-dd hh:mm:ss:fff" ) + " begin: " + this.GetType().ToString() );
            sdb = new Sequoiadb( SdbTestBase.coordUrl );
            sdb.Connect();
            cl = sdb.GetCollecitonSpace(SdbTestBase.csName).CreateCollection( clName );
        }

        [TestMethod]
        public void lob22582()
        {
            int length = 1024;
            byte[] expect = LobUtils.GetRandomBytes( length );
            ObjectId oid = LobUtils.CreateAndWriteLob( cl, expect );

            DBLob lob = cl.OpenLob( oid, DBLob.SDB_LOB_SHAREREAD );
            byte[] actual = new byte[(int)lob.GetSize()];
            lob.Read( actual );
            lob.Close();
            LobUtils.AssertByteArrayEqual( actual, expect );
        }

        [TestCleanup]
        public void tearDown() 
        {
           sdb.GetCollecitonSpace( SdbTestBase.csName ).DropCollection( clName ); 
           sdb.Disconnect();
           Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " end: " + this.GetType().ToString());
        }
    }
}
