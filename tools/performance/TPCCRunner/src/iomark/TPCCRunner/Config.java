/*
 * Copyright (c) 2011 IOMark.CO.CC All rights reserved.
 *
 * The contents of this file are subject to the terms of the IOMARK LICENSE.
 * You may not modify or re-distribute the contents of this file.
 *
 */
package iomark.TPCCRunner ;

import java.text.SimpleDateFormat ;

/**
 * Constant variables defination
 * 
 * @author "Jarvis Wang"
 * @version 1.00
 */
public interface Config {
    public final static String VERSION = "1.0.0" ;
    public final static int NEW_ORDER = 1, PAYMENT = 2, ORDER_STATUS = 3,
            DELIVERY = 4, STOCK_LEVEL = 5 ;

    public final static String[] nameTokens = { "BAR", "OUGHT", "ABLE", "PRI",
            "PRES", "ESE", "ANTI", "CALLY", "ATION", "EING" } ;

    public final static String userPrefix = "User-" ;
    public final static String reportFilePrefix = "reports/TPCCRunner_" ;

    public final static SimpleDateFormat dateFormat = new SimpleDateFormat(
            "yyyy-MM-dd HH:mm:ss" ) ;

    public final static int configCommitCount = 1000 ;
    public final static int configWhseCount = 1 ;
    public final static int configItemCount = 100000 ;
    public final static int configDistPerWhse = 10 ;
    public final static int configCustPerDist = 3000 ;

    /* Interactive commands between the master and the slaves */
    public final static String SOCK_GET_SLAVE_NAME = "SOCK_GET_SLAVE_NAME" ;
    public final static String SOCK_GET_SLAVE_NAME_OK = "SOCK_GET_SLAVE_NAME_OK" ;
    public final static String SOCK_GET_SLAVE_NAME_ERROR = "SOCK_GET_SLAVE_NAME_ERROR" ;

    public final static String SOCK_GET_USER_AND_WAREHOUSE = "SOCK_GET_USER_AND_WAREHOUSE" ;
    public final static String SOCK_GET_USER_AND_WAREHOUSE_OK = "SOCK_GET_USER_AND_WAREHOUSE_OK" ;
    public final static String SOCK_GET_USER_AND_WAREHOUSE_ERROR = "SOCK_GET_USER_AND_WAREHOUSE_ERROR" ;

    public final static String SOCK_SEND_TRANSACTION_CONFIGS = "SOCK_SEND_TRANSACTION_CONFIGS" ;
    public final static String SOCK_START_TRANSACTION = "SOCK_START_TRANSACTION" ;
    public final static String SOCK_GET_TRANSACTION_COUNTERS = "SOCK_GET_TRANSACTION_COUNTERS" ;
    public final static String SOCK_SIGNAL_WARMUP_PHASE_END = "SOCK_SIGNAL_WARMUP_PHASE_END" ;
    public final static String SOCK_SIGNAL_RUN_END = "SOCK_SIGNAL_RUN_END" ;

    public final static String REPORT_HEADER = "              timestamp          type         tpm      avg_rt      max_rt   avg_db_rt   max_db_rt\n" ;
    public final static String REPORT_VALUE = "%23s  %12s  %10.2f  %10.2f  %10d  %10.2f  %10d\n" ;
}
