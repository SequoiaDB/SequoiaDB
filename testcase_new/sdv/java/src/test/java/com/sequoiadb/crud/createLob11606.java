package com.sequoiadb.crud;

import org.bson.types.ObjectId;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @FileName seqDB-11606:同cs下多个cl同时插入lob
 * @Author linsuqiang
 * @Date 2017-03-20
 * @Version 1.00
 */

/*
 * 1、在同一cs下创建多个cl； 2、多个cl中同时创建oid哈希值相同的lob。
 */

public class createLob11606 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clNameBase = "cl";

    private String[] sameHashOids = { "590a20e584aeebef8aff716d",
            "590a20e584aeebef8aff724c", "590a20e584aeebef8aff732b",
            "590a20e584aeebef8aff740a", "590a20e584aeebef8aff75e9",
            "590a20e584aeebef8aff76c8", "590a20e584aeebef8aff77a7",
            "590a20e584aeebef8aff7886" };

    @SuppressWarnings("deprecation")
    @BeforeClass
    public void setup() {
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            CollectionSpace cs = sdb.getCollectionSpace( SdbTestBase.csName );
            for ( int i = 0; i < sameHashOids.length; ++i ) {
                String clName = clNameBase + i;
                cs.createCollection( clName );
            }
        } catch ( BaseException e ) {
            e.printStackTrace();
            if ( sdb != null ) {
                sdb.disconnect();
            }
            Assert.fail( e.getMessage() );
        }
    }

    @Test
    public void test() {
        try {
            OperateLob[] oprLobThds = new OperateLob[ sameHashOids.length ];
            for ( int i = 0; i < sameHashOids.length; ++i ) {
                String clName = clNameBase + i;
                String oidStr = sameHashOids[ i ];
                oprLobThds[ i ] = new OperateLob( clName, oidStr );
            }

            for ( int i = 0; i < oprLobThds.length; ++i ) {
                oprLobThds[ i ].start();
            }

            for ( int i = 0; i < oprLobThds.length; ++i ) {
                if ( !oprLobThds[ i ].isSuccess() ) {
                    Assert.fail( oprLobThds[ i ].getErrorMsg() );
                }
            }
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        }

    }

    @SuppressWarnings("deprecation")
    @AfterClass
    public void teardown() {
        try {
            CollectionSpace cs = sdb.getCollectionSpace( SdbTestBase.csName );
            for ( int i = 0; i < sameHashOids.length; ++i ) {
                String clName = clNameBase + i;
                cs.dropCollection( clName );
            }
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        } finally {
            if ( sdb != null ) {
                sdb.disconnect();
            }
        }
    }

    /**
     * put and remove a lob repeatedly
     */
    private class OperateLob extends SdbThreadBase {
        private String clName = null;
        private ObjectId oid = null;

        public OperateLob( String clName, String oidStr ) {
            this.clName = clName;
            oid = new ObjectId( oidStr );
        }

        @SuppressWarnings({ "resource", "deprecation" })
        @Override
        public void exec() {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( coordUrl, "", "" );
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                DBLob lob = cl.createLob( oid );
                lob.write( "a test string".getBytes() );
                lob.close();
                removeLob( cl );
            } catch ( BaseException e ) {
                e.printStackTrace();
                throw e;
            } finally {
                if ( db != null ) {
                    db.disconnect();
                }
            }
        }

        /**
         * try removing lob until success
         * 
         * @param cl
         */
        private void removeLob( DBCollection cl ) {
            while ( true ) {
                try {
                    cl.removeLob( oid );
                    break;
                } catch ( BaseException e ) {
                    if ( -4 != e.getErrorCode() ) {
                        throw e;
                    }
                }
            }
        }
    }
}
