/**
 * Copyright (c) 2020, SequoiaDB Ltd.
 * File Name:OprLobTask.java
 *      将实现在各个测试用例类中的内部类抽取出来
 *
 *  @author wangwenjing
 * Date:2020年6月17日上午10:06:19
 *  @version 1.00
 */
package com.sequoiadb.datasync;

import org.bson.BSONObject;
import org.bson.util.JSON;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.datasync.OprLobTask;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.task.OperateTask;

public class CreateCLTask extends OperateTask {
    private int clNum = 500 ;
    private String namePrefix ;
    private int count = 0 ;
    private String csName ;
    private BSONObject option = null ;
    private String url = SdbTestBase.coordUrl ;
    public CreateCLTask(String prefix, String groupName) {
        this.namePrefix = prefix ;
        option = ( BSONObject ) JSON
                .parse( "{ ShardingKey: { a: 1 },"
                        + "ShardingType: 'hash', "
                        + "Partition: 2048, " + "ReplSize: 2, "
                        + "Compressed: true, "
                        + "CompressionType: 'lzw',"
                        + "IsMainCL: false, " + "AutoSplit: false, "
                        + (groupName.equals("") ? "": "Group: '" + groupName + "', ")
                        + "AutoIndexId: true, "
                        + "EnsureShardingIndex: true }" );
    }
    
    public CreateCLTask(String prefix, int clNum)
    {
       this.namePrefix = prefix ;
       this.clNum = clNum ;
       this.csName = SdbTestBase.csName ;
    }

    public CreateCLTask(String prefix, int clNum, String csName )
    {
       this(prefix, clNum) ;
       this.csName = csName ;
    }

    public CreateCLTask(String prefix, String groupName, int clNum) {
        this(prefix, groupName) ;
        this.clNum = clNum ;
        this.csName = SdbTestBase.csName ;
    }

    public void setOption(BSONObject opt) {
        this.option = opt ;
    }
    
    public void setUrl(String url) {
        this.url = url ;
    }
    
    @Override
    public void exec() throws Exception {
        try ( Sequoiadb db = new Sequoiadb(this.url , "", "" )) {
            CollectionSpace commCS = db
                    .getCollectionSpace( this.csName );
            for ( int i = 0; i < clNum; i++ ) {
                String clName = namePrefix + "_" + i;
                if (this.option == null) {
                    commCS.createCollection( clName );
                }else {
                    commCS.createCollection( clName, option ); 
                }
                count++;
            }
        } catch ( BaseException e ) {
            System.out.println( "the create cl error i is =" + count );
        }
    }

    public int getCreateNum() {
        return count;
    }
}
