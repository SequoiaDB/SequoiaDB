package com.sequoiadb.snapshot;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.testcommon.SdbTestBase;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;

import java.util.*;

public class SnapshotUtil extends SdbTestBase {

    // 插入记录数，快照用例公用此变量
    public static final int INSERT_NUMS = 1000;
    private static ArrayList< String > lobStats = new ArrayList<>();
    private static ArrayList< String > checkRangeKeys = new ArrayList<>();
    public static double lobdSize = 128 * 1024 * 1024;
    public static double lobmSize = ( 80 * 1024 + 96 ) * 1024;

    static {
        // lob相关指标
        lobStats.add( "TotalLobGet" );
        lobStats.add( "TotalLobPut" );
        lobStats.add( "TotalLobDelete" );
        lobStats.add( "TotalLobReadSize" );
        lobStats.add( "TotalLobWriteSize" );
        lobStats.add( "LobCapacity" );
        lobStats.add( "MaxLobCapacity" );
        lobStats.add( "MaxLobCapSize" );
        lobStats.add( "TotalLobs" );
        lobStats.add( "TotalLobPages" );
        lobStats.add( "TotalUsedLobSpace" );
        lobStats.add( "UsedLobSpaceRatio" );
        lobStats.add( "FreeLobSpace" );
        lobStats.add( "FreeLobSize" );
        lobStats.add( "TotalLobSize" );
        lobStats.add( "TotalValidLobSize" );
        lobStats.add( "LobUsageRate" );
        lobStats.add( "AvgLobSize" );
        lobStats.add( "TotalLobRead" );
        lobStats.add( "TotalLobWrite" );
        lobStats.add( "TotalLobTruncate" );
        lobStats.add( "TotalLobAddressing" );
        lobStats.add( "NodeName" );

        // 校验结果时允许存在误差的指标
        checkRangeKeys.add( "LobCapacity" );
        checkRangeKeys.add( "LobUsageRate" );
        checkRangeKeys.add( "LobMetaCapacity" );
        checkRangeKeys.add( "TotalLobRead" );
        checkRangeKeys.add( "TotalLobWrite" );
        checkRangeKeys.add( "MaxLobCapacity" );
        checkRangeKeys.add( "MaxLobCapSize" );
        checkRangeKeys.add( "UsedLobSpaceRatio" );
        checkRangeKeys.add( "TotalLobReadSize" );
        checkRangeKeys.add( "TotalLobWriteSize" );
    }

    public static void insertData( DBCollection cl ) {
        insertData( cl, INSERT_NUMS );
    }

    public static void insertData( DBCollection cl, int recordNum ) {

        if ( recordNum < 1 ) {
            recordNum = 1;
        }
        for ( int i = 0; i < recordNum; i++ ) {
            BSONObject record = new BasicBSONObject();
            record.put( "a", i );
            cl.insert( record );
        }
    }

    public static ArrayList< BSONObject > getSnapshotLobStat(
            DBCursor cursor ) {
        ArrayList< BSONObject > actLobStats = new ArrayList<>();
        while ( cursor.hasNext() ) {
            BSONObject obj = cursor.getNext();
            BasicBSONObject actLobStat = new BasicBSONObject();
            for ( String lobStat : lobStats ) {
                actLobStat.put( lobStat, obj.get( lobStat ) );
            }
            actLobStats.add( actLobStat );
        }
        cursor.close();
        Collections.sort( actLobStats, new SortBy( "NodeName" ) );
        return actLobStats;
    }

    public static void checkSnapshot( DBCursor cursor,
            ArrayList< BSONObject > expResults, int range ) {
        int i = 0;
        while ( cursor.hasNext() ) {
            BSONObject obj = cursor.getNext();
            BSONObject expResult = expResults.get( i );
            for ( String key : expResult.keySet() ) {
                // TotalLobAddressing指标允许存在一个范围误差
                if ( key.equals( "TotalLobAddressing" ) ) {
                    if ( ( double ) obj.get( key ) < ( double ) expResult
                            .get( key )
                            || ( double ) obj.get(
                                    key ) > ( ( double ) expResult.get( key )
                                            + range ) ) {
                        Assert.fail( "actual: " + obj.get( key )
                                + "\nexpected: " + expResult.get( key ) + "\n"
                                + key + "\nnode name:"
                                + expResult.get( "NodeName" ) );
                    }
                    // checkRangeKeys中的指标允许存在5%的误差
                } else if ( checkRangeKeys.contains( key ) ) {
                    Object value = obj.get( key );
                    if ( value != null ) {
                        double absolute = ( double ) value
                                - ( double ) expResult.get( key );
                        if ( absolute > range
                                && absolute > ( absolute * 0.05 ) ) {
                            Assert.fail( "actual: " + value + "\nexpected: "
                                    + expResult.get( key ) + "\n" + key
                                    + "\nnode name:"
                                    + expResult.get( "NodeName" ) );
                        }
                    }
                } else {
                    Object value = obj.get( key );
                    if ( value != null ) {
                        Assert.assertEquals( value, expResult.get( key ),
                                "act: " + value + "exp: " + expResult.get( key )
                                        + key + "node name:"
                                        + expResult.get( "NodeName" ) );
                    }
                }
            }
            i++;
        }
        cursor.close();
    }

    public static ArrayList< BSONObject > getSnapshotLobStatCL( DBCursor cursor,
            boolean isRawData ) {
        ArrayList< BSONObject > actLobStats = new ArrayList<>();
        String Details = "Details";
        String Group = "Group";
        while ( cursor.hasNext() ) {
            BSONObject obj = cursor.getNext();
            if ( isRawData ) {
                BasicBSONObject actLobStat = new BasicBSONObject();
                for ( String lobStat : lobStats ) {
                    ArrayList< BSONObject > clDetails = ( ArrayList< BSONObject > ) obj
                            .get( Details );
                    actLobStat.put( lobStat,
                            clDetails.get( 0 ).get( lobStat ) );
                }
                actLobStats.add( actLobStat );
            } else {
                ArrayList< BSONObject > clDetails = ( ArrayList< BSONObject > ) obj
                        .get( Details );
                ArrayList< BSONObject > clInfos = ( ArrayList< BSONObject > ) clDetails
                        .get( 0 ).get( Group );
                for ( BSONObject clInfo : clInfos ) {
                    BasicBSONObject actLobStat = new BasicBSONObject();
                    for ( String lobStat : lobStats ) {
                        actLobStat.put( lobStat, clInfo.get( lobStat ) );
                    }
                    actLobStats.add( actLobStat );
                }
            }
        }
        cursor.close();
        Collections.sort( actLobStats, new SortBy( "NodeName" ) );
        return actLobStats;
    }

    public static void checkSnapshotCL( DBCursor cursor,
            ArrayList< BSONObject > expResults, boolean isRawData, int range ) {
        String Details = "Details";
        String Group = "Group";
        int i = 0;
        while ( cursor.hasNext() ) {
            BSONObject obj = cursor.getNext();
            BSONObject expResult = expResults.get( i );
            if ( isRawData ) {
                ArrayList< BSONObject > clDetails = ( ArrayList< BSONObject > ) obj
                        .get( Details );
                for ( String key : expResult.keySet() ) {
                    // TotalLobAddressing指标允许存在一个范围误差
                    if ( key.equals( "TotalLobAddressing" ) ) {
                        if ( ( double ) clDetails.get( 0 )
                                .get( key ) < ( double ) expResult.get( key )
                                || ( double ) clDetails.get( 0 )
                                        .get( key ) > ( ( double ) clDetails
                                                .get( 0 ).get( key )
                                                + range ) ) {
                            Assert.fail( "actual: "
                                    + clDetails.get( 0 ).get( key )
                                    + "\nexpected: " + expResult.get( key )
                                    + "\n" + key + "npde name:"
                                    + expResult.get( "NodeName" ) );
                        }
                        // checkRangeKeys中的指标允许存在5%的误差
                    } else if ( checkRangeKeys.contains( key ) ) {
                        double absolute = ( double ) clDetails.get( 0 )
                                .get( key ) - ( double ) expResult.get( key );
                        if ( absolute > range
                                && absolute > ( absolute * 0.05 ) ) {
                            Assert.fail( "actual: "
                                    + clDetails.get( 0 ).get( key )
                                    + "\nexpected: " + expResult.get( key )
                                    + "\n" + key + "npde name:"
                                    + expResult.get( "NodeName" ) );
                        }
                    } else {
                        Assert.assertEquals( clDetails.get( 0 ).get( key ),
                                expResult.get( key ),
                                "act: " + clDetails.get( 0 ).get( key )
                                        + "\nexp: " + expResult.get( key )
                                        + "\n" + key + "npde name:"
                                        + expResult.get( "NodeName" ) );
                    }
                }
            } else {
                ArrayList< BSONObject > clDetails = ( ArrayList< BSONObject > ) obj
                        .get( Details );
                ArrayList< BSONObject > clInfos = ( ArrayList< BSONObject > ) clDetails
                        .get( 0 ).get( Group );
                Collections.sort( clInfos, new SortBy( "NodeName" ) );
                for ( int j = 0; j < clInfos.size(); j++ ) {
                    BSONObject clInfo = clInfos.get( j );
                    expResult = expResults.get( j );
                    for ( String key : expResult.keySet() ) {
                        if ( key.equals( "TotalLobAddressing" ) ) {
                            Object value = clInfo.get( key );
                            if ( value != null ) {
                                if ( ( long ) clInfo.get(
                                        key ) < ( long ) expResult.get( key )
                                        || ( long ) clInfo.get(
                                                key ) > ( ( long ) expResult
                                                        .get( key )
                                                        + range ) ) {
                                    Assert.fail( "actual: " + clInfo.get( key )
                                            + "\nexpected: "
                                            + expResult.get( key ) + "\n" + key
                                            + "\nnode name:"
                                            + expResult.get( "NodeName" ) );
                                }
                            }
                        } else if ( checkRangeKeys.contains( key ) ) {
                            Object value = clInfo.get( key );
                            if ( value != null ) {
                                if ( clInfo.get( key ).getClass().toString()
                                        .equals( "class java.lang.Double" ) ) {
                                    double absolute = ( double ) clInfo
                                            .get( key )
                                            - ( double ) expResult.get( key );
                                    if ( absolute > range
                                            && absolute > ( ( double ) clInfo
                                                    .get( key ) * 0.05 ) ) {
                                        Assert.fail( "actual: "
                                                + clInfo.get( key )
                                                + "\nexpected: "
                                                + expResult.get( key ) + "\n"
                                                + key + "npde name:"
                                                + expResult.get( "NodeName" ) );
                                    }
                                } else {
                                    long absolute = ( long ) clInfo.get( key )
                                            - ( long ) expResult.get( key );
                                    if ( absolute > range
                                            && absolute > ( ( long ) clInfo
                                                    .get( key ) * 0.05 ) ) {
                                        Assert.fail( "actual: "
                                                + clInfo.get( key )
                                                + "\nexpected: "
                                                + expResult.get( key ) + "\n"
                                                + key + "\nnode name:"
                                                + expResult.get( "NodeName" ) );
                                    }
                                }
                            }
                        } else {
                            Object value = clInfo.get( key );
                            if ( value != null ) {
                                Assert.assertEquals( clInfo.get( key ),
                                        expResult.get( key ),
                                        "act: " + clInfo.get( key ) + "\nexp: "
                                                + expResult.get( key ) + "\n"
                                                + key + "\nact node name:"
                                                + clInfo.get( "NodeName" )
                                                + "\nexp node name:"
                                                + expResult.get( "NodeName" ) );
                            }
                        }
                    }
                }
            }
            i++;
        }
        cursor.close();
    }

    public static class SortBy implements Comparator< BSONObject > {
        public SortBy( String key ) {
            this.key = key;
        }

        private final String key;

        @Override
        public int compare( BSONObject t1, BSONObject t2 ) {
            int flag = 0;
            String no1 = ( String ) t1.get( key );
            String no2 = ( String ) t2.get( key );

            if ( no1.compareTo( no2 ) < 0 ) {
                flag = 1;
            } else if ( no1.compareTo( no2 ) > 0 ) {
                flag = -1;
            }
            return flag;
        }
    }
}
