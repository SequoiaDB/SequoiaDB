package com.sequoiadb.transaction.common;

import java.util.Random;

import com.sequoiadb.base.ConfigOptions;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.task.OperateTask;

public class TransferTh extends OperateTask {
    private String csName;
    private String clName;
    private String coordUrl;
    private boolean faultCoordFlag;
    private Random random = new Random();

    public TransferTh( String csName, String clName ) {
        this.csName = csName;
        this.clName = clName;
        coordUrl = SdbTestBase.coordUrl;
        faultCoordFlag = false;
    }

    private TransferTh( String csName, String clName, String coordUrl ) {
        this.csName = csName;
        this.clName = clName;
        this.coordUrl = coordUrl;
    }

    public TransferTh( String csName, String clName, String coordUrl,
            boolean faultCoordFlag ) {
        this( csName, clName, coordUrl );
        this.faultCoordFlag = faultCoordFlag;
    }

    @Override
    public void exec() throws Exception {
        if ( faultCoordFlag ) {
            runCoordFault();
            return;
        }
        runNormalFault();
    }

    /**
     * 随机提交或者回滚
     * 
     * @param sdb
     */
    private void commitRollback( Sequoiadb sdb ) {
        boolean flag = random.nextBoolean();
        if ( flag ) {
            sdb.commit();
        } else {
            sdb.rollback();
        }
    }

    private void runNormalFault() {
        Sequoiadb db = null;
        try {
            ConfigOptions options = new ConfigOptions();
            options.setSocketTimeout( 600 * 1000 );
            db = new Sequoiadb( coordUrl, "", "", options );
            DBCollection cl = db.getCollectionSpace( csName )
                    .getCollection( clName );
            int count = 0;

            // 模拟转账操作：开启事务，随机取一个账户转出value；随机取另一个账户转入value
            while ( count++ < 1800 ) {
                if ( !TransUtil.runFlag ) {
                    break;
                }

                int accountA = ( int ) ( Math.random() * 10000 );
                int accountB = ( int ) ( Math.random() * 10000 );
                int transAmount = ( int ) ( Math.random() * 200 );

                db.beginTransaction();
                cl.update( "{'account':" + accountA + "}",
                        "{$inc:{'balance':" + ( -transAmount ) + "}}",
                        "{'':'$shard'}" );
                cl.update( "{'account':" + accountB + "}",
                        "{$inc:{'balance':" + transAmount + "}}",
                        "{'':'$shard'}" );
                commitRollback( db );
            }
        } catch ( BaseException e ) {
            if ( "rcauto".equals( SdbTestBase.testGroupOfCurrent ) ) {
                if ( db != null && !db.isClosed() ) {
                    db.rollback();
                }
            }
        } finally {
            if ( db != null && !db.isClosed() ) {
                db.close();
            }
        }
    }

    private void runCoordFault() {
        Sequoiadb db = null;
        try {
            ConfigOptions options = new ConfigOptions();
            options.setSocketTimeout( 600 * 1000 );
            db = new Sequoiadb( coordUrl, "", "", options );
            DBCollection cl = db.getCollectionSpace( csName )
                    .getCollection( clName );
            int count = 0;

            // 模拟转账操作：开启事务，随机取一个账户转出value；随机取另一个账户转入value
            while ( count++ < 1800 ) {
                if ( !TransUtil.runFlag ) {
                    break;
                }

                int accountA = ( int ) ( Math.random() * 10000 );
                int accountB = ( int ) ( Math.random() * 10000 );
                int transAmount = 100;

                db.beginTransaction();
                cl.update( "{'account':" + accountA + "}",
                        "{$inc:{'balance':" + ( -transAmount ) + "}}",
                        "{'':'$shard'}" );
                cl.update( "{'account':" + accountB + "}",
                        "{$inc:{'balance':" + transAmount + "}}",
                        "{'':'$shard'}" );
                commitRollback( db );
            }
        } catch ( BaseException e ) {
        } finally {
            if ( db != null && !db.isClosed() ) {
                db.close();
            }
        }
    }
}
