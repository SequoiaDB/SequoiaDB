package com.sequoiadb.fulltext.utils;

import java.util.ArrayList;
import java.util.List;

/**
 * ES的公共类，涉及ES内部操作的方法放于此类
 */
public class FullTextESUtils {

    /**
     * 获取elasticsearch端全文索引的总记录数
     * 
     * @param esIndexName
     * @return long 返回记录总数
     * @throws Exception
     * @Author liuxiaoxuan
     * @Date 2018-11-15
     */
    public static int getCountFromES( String esIndexName ) throws Exception {
        return new FullTextRest().getCount( esIndexName );
    }

    /**
     * 获取elasticsearch端的SDBCOMMIT记录下的逻辑ID值
     * 
     * @param esIndexName
     * @return int 返回SDBCOMMIT._lid值
     * @throws Exception
     * @Author liuxiaoxuan
     * @Date 2018-11-15
     */
    public static int getCommitIDFromES( String esIndexName ) throws Exception {
        return new FullTextRest().getCommitID( esIndexName );
    }

    /**
     * 获取elasticsearch端的SDBCOMMIT记录下的原始集合逻辑ID值
     * 
     * @param esIndexName
     * @return int 返回SDBCOMMIT._cllid值
     * @throws Exception
     * @Author liuxiaoxuan
     * @Date 2019-05-16
     */
    public static int getCommitCLLIDFromES( String esIndexName )
            throws Exception {
        return new FullTextRest().getCommitCLLID( esIndexName );
    }

    /**
     * 获取多个elasticsearch端的SDBCOMMIT记录下的原始集合逻辑ID值
     * 
     * @param esIndexNames
     * @return List < Integer > 返回每个全文索引的SDBCOMMIT._cllid值
     * @throws Exception
     * @Author liuxiaoxuan
     * @Date 2019-05-17
     */
    public static List< Integer > getCommitCLLIDFromES(
            List< String > esIndexNames ) throws Exception {
        List< Integer > commitCLLIDs = new ArrayList<>();

        for ( String esIndexName : esIndexNames ) {
            commitCLLIDs.add( getCommitCLLIDFromES( esIndexName ) );
        }

        return commitCLLIDs;
    }

    /**
     * 获取elasticsearch端的SDBCOMMIT记录下的全文索引的逻辑ID值
     * 
     * @param esIndexName
     * @return int 返回SDBCOMMIT._idxlid值
     * @throws Exception
     * @Author liuxiaoxuan
     * @Date 2019-08-14
     */
    public static int getCommitIDXLIDFromES( String esIndexName )
            throws Exception {
        return new FullTextRest().getCommitIDXLID( esIndexName );
    }

    /**
     * 获取多个elasticsearch端的SDBCOMMIT记录下的全文索引的逻辑ID值
     * 
     * @param esIndexNames
     * @return List < Integer > 返回每个全文索引的SDBCOMMIT._idxlid值
     * @throws Exception
     * @Author liuxiaoxuan
     * @Date 2019-08-14
     */
    public static List< Integer > getCommitIDXLIDFromES(
            List< String > esIndexNames ) throws Exception {
        List< Integer > commitIDXLIDs = new ArrayList<>();

        for ( String esIndexName : esIndexNames ) {
            commitIDXLIDs.add( getCommitIDXLIDFromES( esIndexName ) );
        }

        return commitIDXLIDs;
    }

    /**
     * 判断elasticsearch端的全文索引名是否存在，用于检查在创建阶段索引名是否映射到elasticsearch端
     * 
     * @param esIndexName
     * @return boolean 索引已在ES创建成功则返回true,否则抛出索引不存在异常
     * @throws Exception
     * @Author liuxiaoxuan
     * @Date 2018-11-15
     */
    public static boolean isIndexCreatedInES( String esIndexName )
            throws Exception {
        int timeout = 600; // 超时时间600s
        int doTimes = 0;

        while ( doTimes < timeout ) {
            if ( isExistIndexInES( esIndexName ) ) {
                return true;
            }
            doTimes++;
            // 每次循环间隔1s
            try {
                Thread.sleep( 1000 );
            } catch ( InterruptedException e ) {
                e.printStackTrace();
            }
        }
        throw new Exception( "es client no such index: " + esIndexName );
    }

    /**
     * 判断elasticsearch端的全文索引名是否被删除，用于清理阶段作环境检查
     * 
     * @param esIndexName
     * @return boolean 索引已在ES创建删除则返回true,否则抛出索引仍然存在异常
     * @throws Exception
     * @Author liuxiaoxuan
     * @Date 2018-11-15
     */
    public boolean isIndexDeletedInES( String esIndexName ) throws Exception {
        int timeout = 600; // 超时时间600s
        int doTimes = 0;

        while ( doTimes < timeout ) {
            if ( !isExistIndexInES( esIndexName ) ) {
                return true;
            }
            doTimes++;
            // 每次循环间隔1s
            try {
                Thread.sleep( 1000 );
            } catch ( InterruptedException e ) {
                e.printStackTrace();
            }
        }

        throw new Exception( "index is still in the es: " + esIndexName );
    }

    /**
     * 判断elasticsearch端的全文索引名是否被删除，用于清理阶段作环境检查
     * 
     * @param esIndexNames
     * @return boolean 索引已在ES创建删除则返回true,否则抛出索引仍然存在异常
     * @throws Exception
     * @Author liuxiaoxuan
     * @Date 2018-11-15
     */
    public boolean isIndexDeletedInES( List< String > esIndexNames )
            throws Exception {
        boolean isDelete = false;
        for ( String esIndexName : esIndexNames ) {
            isDelete = isIndexDeletedInES( esIndexName );
            if ( !isDelete ) {
                break;
            }
        }
        return isDelete;
    }

    /**
     * 判断elasticsearch端的全文索引名是否存在
     * 
     * @param esIndexName
     * @return boolean 存在返回true, 否则返回false
     * @throws Exception
     * @Author liuxiaoxuan
     * @Date 2018-11-15
     */
    private static boolean isExistIndexInES( String esIndexName )
            throws Exception {
        return new FullTextRest().isExist( esIndexName );
    }

}
