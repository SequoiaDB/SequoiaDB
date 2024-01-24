package com.sequoias3.utils;

import com.sequoias3.common.DataShardingType;

import java.text.SimpleDateFormat;
import java.util.Date;

public class ShardingTypeUtils {
    public static final String QUARTER_1      = "Q1";
    public static final String QUARTER_2      = "Q2";
    public static final String QUARTER_3      = "Q3";
    public static final String QUARTER_4      = "Q4";

    public static String getShardingTypeStr(DataShardingType shardingType, Date date){
        switch (shardingType){
            case YEAR:
                SimpleDateFormat yearFormat   = new SimpleDateFormat("yyyy");
                return yearFormat.format(date);
            case MONTH:
                SimpleDateFormat monthFormat = new SimpleDateFormat("MM");
                return monthFormat.format(date);
            case QUARTER:
                SimpleDateFormat quarterFormat = new SimpleDateFormat("MM");
                return getQuarter(Integer.parseInt(quarterFormat.format(date)));
            default:
                return null;
        }
    }

    private static String getQuarter(int month){
        switch (month){
            case 1:
            case 2:
            case 3:
                return QUARTER_1;
            case 4:
            case 5:
            case 6:
                return QUARTER_2;
            case 7:
            case 8:
            case 9:
                return QUARTER_3;
            case 10:
            case 11:
            case 12:
                return QUARTER_4;
            default:
                return null;
        }
    }
}
