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
    * description:   seqDB-22692:lob mode参数校验 
    * author:        Zhao Xiaoni
    * date:          2020/8/25
    */

    [TestClass]
    public class Lob22692
    {
        private String clName = "cl_22692";
        private Sequoiadb sdb = null;
        private DBCollection cl = null;

        [TestInitialize]
        public void setup()
        {
            Console.WriteLine(DateTime.Now.ToString( "yyyy-MM-dd hh:mm:ss:fff" ) + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb( SdbTestBase.coordUrl );
            sdb.Connect();
            cl = sdb.GetCollecitonSpace( SdbTestBase.csName ).CreateCollection( clName );
        }

        [TestMethod]
        public void lob22692()
        {
            int length = 1024;
            byte[] expect = LobUtils.GetRandomBytes( length );
            ObjectId oid = LobUtils.CreateAndWriteLob( cl, expect );

            DBLob lob = cl.OpenLob( oid, DBLob.SDB_LOB_WRITE );
            byte[] actual = new byte[ (int)lob.GetSize() ];
            try
            {
                lob.Read(actual);
                Assert.Fail("Can't read in write mode.");
            }
            catch (BaseException e)
            {
                if (e.ErrorCode != -6)
                {
                    Assert.Fail("Lob22692 read fail in write mode: " + e);
                }
            }
            finally
            {
                lob.Close();
            }

            lob = cl.OpenLob( oid, DBLob.SDB_LOB_READ );
            try
            {
                lob.Write( expect );
                Assert.Fail("Can't write in read mode.");
            }
            catch (BaseException e)
            {
                if ( e.ErrorCode != -6 )
                {
                    Assert.Fail("Lob22692 write fail in read mode: " + e);
                }
            }
            finally
            {
                lob.Close();
            }

            lob = cl.OpenLob( oid, DBLob.SDB_LOB_SHAREREAD );
            try
            {
                lob.Write( expect );
                Assert.Fail("Can't write in shareread mode.");
            }
            catch (BaseException e)
            {
                if (e.ErrorCode != -6)
                {
                    Assert.Fail("Lob22692 write fail in shareread mode: " + e);
                }
            }
            finally
            {
                lob.Close();
            }
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
