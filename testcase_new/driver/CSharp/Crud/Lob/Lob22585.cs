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
   * description:   seqDB-22585:以SDB_LOB_SHAREREAD|SDB_LOB_WRITE模式OpenLob后Read、Write、Seek、Lock 
   * author:        Zhao Xiaoni
   * date:          2020/8/8
   */

    [TestClass]
    public class Lob22585
    {
        private String clName = "cl_22585";
        private Sequoiadb sdb = null;
        private DBCollection cl = null;

        [TestInitialize]
        public void setup()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
            cl = sdb.GetCollecitonSpace(SdbTestBase.csName).CreateCollection(clName);
        }

        [TestMethod]
        public void lob22585()
        {
            int length = 1024 * 2;
            byte[] expect = LobUtils.GetRandomBytes( length );
            ObjectId oid = LobUtils.CreateAndWriteLob( cl, expect );

            //Lock、Seek、Write
            DBLob lob = cl.OpenLob( oid, DBLob.SDB_LOB_SHAREREAD | DBLob.SDB_LOB_WRITE );
            lob.Lock( length, length );
            lob.Seek( length, DBLob.SDB_LOB_SEEK_CUR );
            lob.Write( expect );
            lob.Close();

            //Seek、Read
            lob = cl.OpenLob( oid, DBLob.SDB_LOB_SHAREREAD | DBLob.SDB_LOB_WRITE );
            lob.Seek( length, DBLob.SDB_LOB_SEEK_SET );
            byte[] actual = new byte[ (int)lob.GetSize() ];
            lob.Read( actual );
            lob.Close();
        }

        [TestCleanup]
        public void tearDown()
        {
            sdb.GetCollecitonSpace(SdbTestBase.csName).DropCollection(clName);
            sdb.Disconnect();
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " end: " + this.GetType().ToString());
        }
    }
}
