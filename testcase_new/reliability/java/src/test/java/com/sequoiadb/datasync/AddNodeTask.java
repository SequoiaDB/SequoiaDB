/**
 * Copyright (c) 2020, SequoiaDB Ltd.
 * File Name: AddNodeTask.java
 *      将实现在各个测试用例类中的内部类抽取出来
 *
 *  @author wangwenjing
 * Date:2020年6月17日上午10:06:19
 *  @version 1.00
 */
package com.sequoiadb.datasync;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;

import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Node;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.task.OperateTask;

public class AddNodeTask extends OperateTask{
    private Sequoiadb db = null ;
    private String groupName ;
    private String nodeName ;
    private int nodePort ;
    private String hostName ;
    private ReplicaGroup randomGroup ;
    
    public AddNodeTask( String groupName, String hostName, int nodePort ) {
        this.groupName = groupName ;
        this.hostName = hostName ;
        this.nodePort = nodePort ;
    }
    
    @Override
    public void init() {
        db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        randomGroup = db.getReplicaGroup( groupName );
        String nodePath = SdbTestBase.reservedDir + "/data/" + nodePort;
        Node newNode = randomGroup.createNode( hostName, nodePort,
                nodePath, ( BSONObject ) null );
        newNode.start();
        nodeName = newNode.getNodeName() ;
    }

    @Override
    public void exec() throws Exception {
        boolean isFullSync = false ;
        int totalTimeLen = 600 ;
        int alreadySleepLen = 0 ;
  
        do 
        {
            final String sRawdata = "rawdata" ;
            final String sNodeName = "NodeName" ;
            final String sStatus = "Status" ;
            final String sFullSync = "FullSync" ;
            BSONObject cond = new BasicBSONObject() ;
            cond.put( sRawdata, true ) ;
            cond.put( sNodeName, nodeName ) ;
        
            BSONObject selector = new BasicBSONObject() ;
            selector.put( sStatus, "" ) ;
            DBCursor cursor = db.getSnapshot( Sequoiadb.SDB_SNAP_DATABASE, cond, selector, null ) ;
            while( cursor.hasNext() ) 
            {
               BasicBSONObject ret = ( BasicBSONObject ) cursor.getNext() ;
               String status = ret.getString( sStatus ) ;
               if ( status.equals(sFullSync))
               {
                   isFullSync = true ;
                   Thread.sleep(1000);
                   alreadySleepLen += 1;
               }
               else
               {
                   isFullSync = false;
                   
               }
            }
        }while( isFullSync && alreadySleepLen < totalTimeLen) ;
        
    }
    public void removeNode() {
       
        Node master = randomGroup.getMaster() ;
        if ( master.getNodeName().equals( this.hostName + ":" + this.nodePort)){
            randomGroup.reelect(null);
        }
            
        randomGroup.removeNode( this.hostName, this.nodePort, null );
        
    }
}
