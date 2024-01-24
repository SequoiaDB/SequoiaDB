/*
 * Copyright (c) 2011 IOMark.CO.CC All rights reserved.
 *
 * The contents of this file are subject to the terms of the IOMARK LICENSE.
 * You may not modify or re-distribute the contents of this file.
 *
 */
package iomark.TPCCRunner ;

import java.io.BufferedReader ;
import java.io.DataOutputStream ;
import java.io.FileInputStream ;
import java.io.FileOutputStream ;
import java.io.IOException ;
import java.io.InputStreamReader ;
import java.io.PrintStream ;
import java.net.ServerSocket ;
import java.net.Socket ;
import java.sql.Timestamp ;
import java.util.HashMap ;
import java.util.Properties ;

/**
 * Master thread to control over slaves
 * 
 * @author "Jarvis Wang"
 * @version 1.00
 */
public class Master implements Config, Runnable {

    private PrintStream printStreamLogs = null ;

    private ServerSocket master = null ;

    // Configuration

    private HashMap< String, Socket > slaves = null ;

    private int userCount = 0 ;
    private int warehousesCount = 0 ;

    private int runMinutes = 0 ;
    private int warmupMinutes = 0 ;
    private long startTimestamp = 0 ;
    private long warmupTimestamp = 0 ;

    private int newOrderPercent = 45 ;
    private int paymentPercent = 43 ;
    private int orderStatusPercent = 4 ;
    private int deliveryPercent = 4 ;
    private int stockLevelPercent = 4 ;

    private int newOrderThinkSecond = 18 ;
    private int paymentThinkSecond = 3 ;
    private int orderStatusThinkSecond = 2 ;
    private int deliveryThinkSecond = 2 ;
    private int stockLevelThinkSecond = 2 ;

    // Payment Counters

    private int payment_num_total = 0 ;
    private int payment_num_warmup = 0 ;
    private int payment_num_total_last = 0 ;

    private long payment_time_total = 0 ;
    private long payment_time_warmup = 0 ;
    private long payment_time_max = 0 ;
    private long payment_time_total_last = 0 ;
    private long payment_time_max_last = 0 ;

    private long payment_dbtime_total = 0 ;
    private long payment_dbtime_warmup = 0 ;
    private long payment_dbtime_max = 0 ;
    private long payment_dbtime_total_last = 0 ;
    private long payment_dbtime_max_last = 0 ;

    // Stock-Level Counters

    private int stock_level_num_total = 0 ;
    private int stock_level_num_warmup = 0 ;
    private int stock_level_num_total_last = 0 ;

    private long stock_level_time_total = 0 ;
    private long stock_level_time_warmup = 0 ;
    private long stock_level_time_max = 0 ;
    private long stock_level_time_total_last = 0 ;
    private long stock_level_time_max_last = 0 ;

    private long stock_level_dbtime_total = 0 ;
    private long stock_level_dbtime_warmup = 0 ;
    private long stock_level_dbtime_max = 0 ;
    private long stock_level_dbtime_total_last = 0 ;
    private long stock_level_dbtime_max_last = 0 ;

    // Order-Status Counters

    private int order_status_num_total = 0 ;
    private int order_status_num_warmup = 0 ;
    private int order_status_num_total_last = 0 ;

    private long order_status_time_total = 0 ;
    private long order_status_time_warmup = 0 ;
    private long order_status_time_max = 0 ;
    private long order_status_time_total_last = 0 ;
    private long order_status_time_max_last = 0 ;

    private long order_status_dbtime_total = 0 ;
    private long order_status_dbtime_warmup = 0 ;
    private long order_status_dbtime_max = 0 ;
    private long order_status_dbtime_total_last = 0 ;
    private long order_status_dbtime_max_last = 0 ;

    // Delivery Counters

    private int delivery_num_total = 0 ;
    private int delivery_num_warmup = 0 ;
    private int delivery_num_total_last = 0 ;

    private long delivery_time_total = 0 ;
    private long delivery_time_warmup = 0 ;
    private long delivery_time_max = 0 ;
    private long delivery_time_total_last = 0 ;
    private long delivery_time_max_last = 0 ;

    private long delivery_dbtime_total = 0 ;
    private long delivery_dbtime_warmup = 0 ;
    private long delivery_dbtime_max = 0 ;
    private long delivery_dbtime_total_last = 0 ;
    private long delivery_dbtime_max_last = 0 ;

    // New-Order Counters

    private int new_order_num_total = 0 ;
    private int new_order_num_warmup = 0 ;
    private int new_order_num_total_last = 0 ;

    private long new_order_time_total = 0 ;
    private long new_order_time_warmup = 0 ;
    private long new_order_time_max = 0 ;
    private long new_order_time_total_last = 0 ;
    private long new_order_time_max_last = 0 ;

    private long new_order_dbtime_total = 0 ;
    private long new_order_dbtime_warmup = 0 ;
    private long new_order_dbtime_max = 0 ;
    private long new_order_dbtime_total_last = 0 ;
    private long new_order_dbtime_max_last = 0 ;

    private void logMessage( String message ) {
        printStreamLogs.println( message ) ;
        System.out.println( message ) ;
    }

    public Master( String propertiesFile ) throws IOException {

        this.printStreamLogs = new PrintStream( new FileOutputStream(
                "log/log_master_" + Util.getFileNameSuffix() + ".txt" ) ) ;

        // Create properties
        Properties properties = new Properties() ;
        properties.load( new FileInputStream( propertiesFile ) ) ;

        // Create Listen Socket
        logMessage( "Creating listen socket" ) ;
        int listenPort = Integer.parseInt( properties
                .getProperty( "listenPort" ) ) ;
        this.master = new ServerSocket( listenPort ) ;
        logMessage( "Listening on " + listenPort ) ;

        // Connect to Slaves
        logMessage( "Waiting slaves to be connected." ) ;
        String[] slaveNames = properties.getProperty( "slaves" ).split( "," ) ;
        this.slaves = new HashMap< String, Socket >() ;
        while ( this.slaves.size() != slaveNames.length ) {
            Socket slave = master.accept() ;
            BufferedReader response = new BufferedReader(
                    new InputStreamReader( slave.getInputStream() ) ) ;
            DataOutputStream request = new DataOutputStream(
                    slave.getOutputStream() ) ;
            request.writeBytes( SOCK_GET_SLAVE_NAME + "\n" ) ;
            String name = response.readLine() ;
            boolean rightName = false ;
            for ( String slaveName : slaveNames ) {
                if ( slaveName.equalsIgnoreCase( name ) ) {
                    rightName = true ;
                    break ;
                }
            }
            if ( rightName == true && !slaves.containsKey( name ) ) {
                slaves.put( name, slave ) ;
                request.writeBytes( SOCK_GET_SLAVE_NAME_OK + "\n" ) ;
                logMessage( name + " connected." ) ;
            } else {
                request.writeBytes( SOCK_GET_SLAVE_NAME_ERROR + "\n" ) ;
                response.close() ;
                request.close() ;
                slave.close() ;
            }
        }
        logMessage( "All slaves are connected" ) ;

        // Get user and warehouse properties
        logMessage( "Sync properties with slaves" ) ;
        for ( Socket slave : this.slaves.values() ) {
            BufferedReader response = new BufferedReader(
                    new InputStreamReader( slave.getInputStream() ) ) ;
            DataOutputStream request = new DataOutputStream(
                    slave.getOutputStream() ) ;
            request.writeBytes( SOCK_GET_USER_AND_WAREHOUSE + "\n" ) ;
            String[] counts = response.readLine().split( "," ) ;
            if ( counts.length == 2 ) {
                this.userCount += Integer.parseInt( counts[0] ) ;
                this.warehousesCount += Integer.parseInt( counts[1] ) ;
                request.writeBytes( SOCK_GET_USER_AND_WAREHOUSE_OK + "\n" ) ;
            } else {
                request.writeBytes( SOCK_GET_USER_AND_WAREHOUSE_ERROR + "\n" ) ;
            }
        }

        // get run time properties
        this.runMinutes = Integer.parseInt( properties
                .getProperty( "runMinutes" ) ) ;
        this.warmupMinutes = Integer.parseInt( properties
                .getProperty( "warmupMinutes" ) ) ;

        // get transaction properties and sync with slaves
        this.newOrderPercent = Integer.parseInt( properties
                .getProperty( "newOrderPercent" ) ) ;
        this.paymentPercent = Integer.parseInt( properties
                .getProperty( "paymentPercent" ) ) ;
        this.orderStatusPercent = Integer.parseInt( properties
                .getProperty( "orderStatusPercent" ) ) ;
        this.deliveryPercent = Integer.parseInt( properties
                .getProperty( "deliveryPercent" ) ) ;
        this.stockLevelPercent = Integer.parseInt( properties
                .getProperty( "stockLevelPercent" ) ) ;
        this.newOrderThinkSecond = Integer.parseInt( properties
                .getProperty( "newOrderThinkSecond" ) ) ;
        this.paymentThinkSecond = Integer.parseInt( properties
                .getProperty( "paymentThinkSecond" ) ) ;
        this.orderStatusThinkSecond = Integer.parseInt( properties
                .getProperty( "orderStatusThinkSecond" ) ) ;
        this.deliveryThinkSecond = Integer.parseInt( properties
                .getProperty( "deliveryThinkSecond" ) ) ;
        this.stockLevelThinkSecond = Integer.parseInt( properties
                .getProperty( "stockLevelThinkSecond" ) ) ;
        String transactionConfig = String.format(
                "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d", this.newOrderPercent,
                this.paymentPercent, this.orderStatusPercent,
                this.deliveryPercent, this.stockLevelPercent,
                this.newOrderThinkSecond, this.paymentThinkSecond,
                this.orderStatusThinkSecond, this.deliveryThinkSecond,
                this.stockLevelThinkSecond ) ;
        for ( Socket slave : this.slaves.values() ) {
            DataOutputStream request = new DataOutputStream(
                    slave.getOutputStream() ) ;
            request.writeBytes( SOCK_SEND_TRANSACTION_CONFIGS + ","
                    + transactionConfig + "\n" ) ;
        }

        // Start Transaction
        logMessage( "Start transactions" ) ;
        for ( Socket slave : this.slaves.values() ) {
            DataOutputStream request = new DataOutputStream(
                    slave.getOutputStream() ) ;
            request.writeBytes( SOCK_START_TRANSACTION + "\n" ) ;
        }

        this.startTimestamp = System.currentTimeMillis() ;
        logMessage( "Transactions started at " + new Timestamp( startTimestamp ) ) ;
    }

    public void run() {
        long currentTimestamp = 0 ;
        for ( int min = 1; min < Integer.MAX_VALUE; min++ ) {
            try {
                Thread.sleep( 60 * 1000 ) ;
                currentTimestamp = System.currentTimeMillis() ;

                payment_num_total_last = payment_num_total ;
                payment_dbtime_total_last = payment_dbtime_total ;
                payment_time_total_last = payment_time_total ;
                payment_num_total = 0 ;
                payment_dbtime_total = 0 ;
                payment_time_total = 0 ;
                payment_dbtime_max_last = 0 ;
                payment_time_max_last = 0 ;

                stock_level_num_total_last = stock_level_num_total ;
                stock_level_dbtime_total_last = stock_level_dbtime_total ;
                stock_level_time_total_last = stock_level_time_total ;
                stock_level_num_total = 0 ;
                stock_level_dbtime_total = 0 ;
                stock_level_time_total = 0 ;
                stock_level_dbtime_max_last = 0 ;
                stock_level_time_max_last = 0 ;

                order_status_num_total_last = order_status_num_total ;
                order_status_dbtime_total_last = order_status_dbtime_total ;
                order_status_time_total_last = order_status_time_total ;
                order_status_num_total = 0 ;
                order_status_dbtime_total = 0 ;
                order_status_time_total = 0 ;
                order_status_dbtime_max_last = 0 ;
                order_status_time_max_last = 0 ;

                delivery_num_total_last = delivery_num_total ;
                delivery_dbtime_total_last = delivery_dbtime_total ;
                delivery_time_total_last = delivery_time_total ;
                delivery_num_total = 0 ;
                delivery_dbtime_total = 0 ;
                delivery_time_total = 0 ;
                delivery_dbtime_max_last = 0 ;
                delivery_time_max_last = 0 ;

                new_order_num_total_last = new_order_num_total ;
                new_order_dbtime_total_last = new_order_dbtime_total ;
                new_order_time_total_last = new_order_time_total ;
                new_order_num_total = 0 ;
                new_order_dbtime_total = 0 ;
                new_order_time_total = 0 ;
                new_order_dbtime_max_last = 0 ;
                new_order_time_max_last = 0 ;

                for ( Socket slave : this.slaves.values() ) {
                    BufferedReader response = new BufferedReader(
                            new InputStreamReader( slave.getInputStream() ) ) ;
                    DataOutputStream request = new DataOutputStream(
                            slave.getOutputStream() ) ;
                    request.writeBytes( SOCK_GET_TRANSACTION_COUNTERS + "\n" ) ;
                    String[] contersGroups = response.readLine().split( " " ) ;

                    String[] counters = contersGroups[0].split( "," ) ;
                    payment_num_total += Integer.parseInt( counters[0] ) ;
                    payment_time_total += Long.parseLong( counters[1] ) ;
                    if ( payment_time_max_last < Long.parseLong( counters[2] ) ) {
                        payment_time_max_last = Long.parseLong( counters[2] ) ;
                    }
                    payment_dbtime_total += Long.parseLong( counters[3] ) ;
                    if ( payment_dbtime_max_last < Long.parseLong( counters[4] ) ) {
                        payment_dbtime_max_last = Long.parseLong( counters[4] ) ;
                    }

                    counters = contersGroups[1].split( "," ) ;
                    stock_level_num_total += Integer.parseInt( counters[0] ) ;
                    stock_level_time_total += Long.parseLong( counters[1] ) ;
                    if ( stock_level_time_max_last < Long
                            .parseLong( counters[2] ) ) {
                        stock_level_time_max_last = Long
                                .parseLong( counters[2] ) ;
                    }
                    stock_level_dbtime_total += Long.parseLong( counters[3] ) ;
                    if ( stock_level_dbtime_max_last < Long
                            .parseLong( counters[4] ) ) {
                        stock_level_dbtime_max_last = Long
                                .parseLong( counters[4] ) ;
                    }

                    counters = contersGroups[2].split( "," ) ;
                    order_status_num_total += Integer.parseInt( counters[0] ) ;
                    order_status_time_total += Long.parseLong( counters[1] ) ;
                    if ( order_status_time_max_last < Long
                            .parseLong( counters[2] ) ) {
                        order_status_time_max_last = Long
                                .parseLong( counters[2] ) ;
                    }
                    order_status_dbtime_total += Long.parseLong( counters[3] ) ;
                    if ( order_status_dbtime_max_last < Long
                            .parseLong( counters[4] ) ) {
                        order_status_dbtime_max_last = Long
                                .parseLong( counters[4] ) ;
                    }

                    counters = contersGroups[3].split( "," ) ;
                    delivery_num_total += Integer.parseInt( counters[0] ) ;
                    delivery_time_total += Long.parseLong( counters[1] ) ;
                    if ( delivery_time_max_last < Long.parseLong( counters[2] ) ) {
                        delivery_time_max_last = Long.parseLong( counters[2] ) ;
                    }
                    delivery_dbtime_total += Long.parseLong( counters[3] ) ;
                    if ( delivery_dbtime_max_last < Long
                            .parseLong( counters[4] ) ) {
                        delivery_dbtime_max_last = Long.parseLong( counters[4] ) ;
                    }

                    counters = contersGroups[4].split( "," ) ;
                    new_order_num_total += Integer.parseInt( counters[0] ) ;
                    new_order_time_total += Long.parseLong( counters[1] ) ;
                    if ( new_order_time_max_last < Long.parseLong( counters[2] ) ) {
                        new_order_time_max_last = Long.parseLong( counters[2] ) ;
                    }
                    new_order_dbtime_total += Long.parseLong( counters[3] ) ;
                    if ( new_order_dbtime_max_last < Long
                            .parseLong( counters[4] ) ) {
                        new_order_dbtime_max_last = Long
                                .parseLong( counters[4] ) ;
                    }
                }
                if ( payment_dbtime_max < payment_dbtime_max_last ) {
                    payment_dbtime_max = payment_dbtime_max_last ;
                }
                if ( payment_time_max < payment_time_max_last ) {
                    payment_time_max = payment_time_max_last ;
                }
                if ( stock_level_dbtime_max < stock_level_dbtime_max_last ) {
                    stock_level_dbtime_max = stock_level_dbtime_max_last ;
                }
                if ( stock_level_time_max < stock_level_time_max_last ) {
                    stock_level_time_max = stock_level_time_max_last ;
                }
                if ( order_status_dbtime_max < order_status_dbtime_max_last ) {
                    order_status_dbtime_max = order_status_dbtime_max_last ;
                }
                if ( order_status_time_max < order_status_time_max_last ) {
                    order_status_time_max = order_status_time_max_last ;
                }
                if ( delivery_dbtime_max < delivery_dbtime_max_last ) {
                    delivery_dbtime_max = delivery_dbtime_max_last ;
                }
                if ( delivery_time_max < delivery_time_max_last ) {
                    delivery_time_max = delivery_time_max_last ;
                }
                if ( new_order_dbtime_max < new_order_dbtime_max_last ) {
                    new_order_dbtime_max = new_order_dbtime_max_last ;
                }
                if ( new_order_time_max < new_order_time_max_last ) {
                    new_order_time_max = new_order_time_max_last ;
                }

                String type = null ;
                double tpm = 0 ;
                double avg_rt = 0 ;
                long max_rt = 0 ;
                double avg_db_rt = 0 ;
                long max_db_rt = 0 ;
                StringBuilder reportString = null ;

                reportString = new StringBuilder( REPORT_HEADER ) ;
                type = "payment" ;
                tpm = ( double ) ( payment_num_total - payment_num_total_last ) ;
                if ( payment_num_total - payment_num_total_last == 0 ) {
                    avg_rt = 0 ;
                    avg_db_rt = 0 ;
                } else {
                    avg_rt = ( double ) ( payment_time_total - payment_time_total_last )
                            / ( payment_num_total - payment_num_total_last ) ;
                    avg_db_rt = ( double ) ( payment_dbtime_total - payment_dbtime_total_last )
                            / ( payment_num_total - payment_num_total_last ) ;
                }
                max_rt = payment_time_max_last ;
                max_db_rt = payment_dbtime_max_last ;
                reportString.append( String.format( REPORT_VALUE,
                        new Timestamp( currentTimestamp ), type, tpm, avg_rt,
                        max_rt, avg_db_rt, max_db_rt ) ) ;

                type = "stock_level" ;
                tpm = ( double ) ( stock_level_num_total - stock_level_num_total_last ) ;
                if ( stock_level_num_total - stock_level_num_total_last == 0 ) {
                    avg_rt = 0 ;
                    avg_db_rt = 0 ;
                } else {
                    avg_rt = ( double ) ( stock_level_time_total - stock_level_time_total_last )
                            / ( stock_level_num_total - stock_level_num_total_last ) ;
                    avg_db_rt = ( double ) ( stock_level_dbtime_total - stock_level_dbtime_total_last )
                            / ( stock_level_num_total - stock_level_num_total_last ) ;
                }
                max_rt = stock_level_time_max_last ;
                max_db_rt = stock_level_dbtime_max_last ;
                reportString.append( String.format( REPORT_VALUE,
                        new Timestamp( currentTimestamp ), type, tpm, avg_rt,
                        max_rt, avg_db_rt, max_db_rt ) ) ;

                type = "order_status" ;
                tpm = ( double ) ( order_status_num_total - order_status_num_total_last ) ;
                if ( order_status_num_total - order_status_num_total_last == 0 ) {
                    avg_rt = 0 ;
                    avg_db_rt = 0 ;
                } else {
                    avg_rt = ( double ) ( order_status_time_total - order_status_time_total_last )
                            / ( order_status_num_total - order_status_num_total_last ) ;
                    avg_db_rt = ( double ) ( order_status_dbtime_total - order_status_dbtime_total_last )
                            / ( order_status_num_total - order_status_num_total_last ) ;
                }
                max_rt = order_status_time_max_last ;
                max_db_rt = order_status_dbtime_max_last ;
                reportString.append( String.format( REPORT_VALUE,
                        new Timestamp( currentTimestamp ), type, tpm, avg_rt,
                        max_rt, avg_db_rt, max_db_rt ) ) ;

                type = "delivery" ;
                tpm = ( double ) ( delivery_num_total - delivery_num_total_last ) ;
                if ( delivery_num_total - delivery_num_total_last == 0 ) {
                    avg_rt = 0 ;
                    avg_db_rt = 0 ;
                } else {
                    avg_rt = ( double ) ( delivery_time_total - delivery_time_total_last )
                            / ( delivery_num_total - delivery_num_total_last ) ;
                    avg_db_rt = ( double ) ( delivery_dbtime_total - delivery_dbtime_total_last )
                            / ( delivery_num_total - delivery_num_total_last ) ;
                }
                max_rt = delivery_time_max_last ;
                max_db_rt = delivery_dbtime_max_last ;
                reportString.append( String.format( REPORT_VALUE,
                        new Timestamp( currentTimestamp ), type, tpm, avg_rt,
                        max_rt, avg_db_rt, max_db_rt ) ) ;

                type = "new_order" ;
                tpm = ( double ) ( new_order_num_total - new_order_num_total_last ) ;
                if ( new_order_num_total - new_order_num_total_last == 0 ) {
                    avg_rt = 0 ;
                    avg_db_rt = 0 ;
                } else {
                    avg_rt = ( double ) ( new_order_time_total - new_order_time_total_last )
                            / ( new_order_num_total - new_order_num_total_last ) ;
                    avg_db_rt = ( double ) ( new_order_dbtime_total - new_order_dbtime_total_last )
                            / ( new_order_num_total - new_order_num_total_last ) ;
                }
                max_rt = new_order_time_max_last ;
                max_db_rt = new_order_dbtime_max_last ;
                reportString.append( String.format( REPORT_VALUE,
                        new Timestamp( currentTimestamp ), type, tpm, avg_rt,
                        max_rt, avg_db_rt, max_db_rt ) ) ;

                logMessage( reportString.toString() ) ;

                // warmup end:
                if ( min == this.warmupMinutes ) {
                    this.warmupTimestamp = System.currentTimeMillis() ;
                    this.payment_dbtime_warmup = this.payment_dbtime_total ;
                    this.payment_num_warmup = this.payment_num_total ;
                    this.payment_time_warmup = this.payment_time_total ;
                    this.payment_dbtime_max = 0 ;
                    this.payment_time_max = 0 ;

                    this.stock_level_dbtime_warmup = this.stock_level_dbtime_total ;
                    this.stock_level_num_warmup = this.stock_level_num_total ;
                    this.stock_level_time_warmup = this.stock_level_time_total ;
                    this.stock_level_dbtime_max = 0 ;
                    this.stock_level_time_max = 0 ;

                    this.order_status_dbtime_warmup = this.order_status_dbtime_total ;
                    this.order_status_num_warmup = this.order_status_num_total ;
                    this.order_status_time_warmup = this.order_status_time_total ;
                    this.order_status_dbtime_max = 0 ;
                    this.order_status_time_max = 0 ;

                    this.delivery_dbtime_warmup = this.delivery_dbtime_total ;
                    this.delivery_num_warmup = this.delivery_num_total ;
                    this.delivery_time_warmup = this.delivery_time_total ;
                    this.delivery_dbtime_max = 0 ;
                    this.delivery_time_max = 0 ;

                    this.new_order_dbtime_warmup = this.new_order_dbtime_total ;
                    this.new_order_num_warmup = this.new_order_num_total ;
                    this.new_order_time_warmup = this.new_order_time_total ;
                    this.new_order_dbtime_max = 0 ;
                    this.new_order_time_max = 0 ;
                    logMessage( "Warmup phase end.\n" ) ;
                    for ( Socket slave : this.slaves.values() ) {
                        DataOutputStream request = new DataOutputStream(
                                slave.getOutputStream() ) ;
                        request.writeBytes( SOCK_SIGNAL_WARMUP_PHASE_END + "\n" ) ;
                    }
                }

                // run end:
                if ( min == this.runMinutes ) {
                    logMessage( "Run End.\n" ) ;
                    reportString = new StringBuilder( REPORT_HEADER ) ;
                    double executeMinutes = ( ( ( double ) ( currentTimestamp - this.warmupTimestamp ) / 1000 ) / 60 ) ;

                    type = "payment" ;
                    tpm = ( double ) ( payment_num_total - payment_num_warmup )
                            / executeMinutes ;
                    if ( ( payment_num_total - payment_num_warmup ) == 0 ) {
                        avg_rt = 0 ;
                        avg_db_rt = 0 ;
                    } else {
                        avg_rt = ( double ) ( payment_time_total - payment_time_warmup )
                                / ( payment_num_total - payment_num_warmup ) ;
                        avg_db_rt = ( double ) ( payment_dbtime_total - payment_dbtime_warmup )
                                / ( payment_num_total - payment_num_warmup ) ;
                    }
                    max_rt = payment_time_max ;
                    max_db_rt = payment_dbtime_max ;
                    reportString.append( String.format( REPORT_VALUE,
                            "average", type, tpm, avg_rt, max_rt, avg_db_rt,
                            max_db_rt ) ) ;

                    type = "stock_level" ;
                    tpm = ( double ) ( stock_level_num_total - stock_level_num_warmup )
                            / executeMinutes ;
                    if ( ( stock_level_num_total - stock_level_num_warmup ) == 0 ) {
                        avg_rt = 0 ;
                        avg_db_rt = 0 ;
                    } else {
                        avg_rt = ( double ) ( stock_level_time_total - stock_level_time_warmup )
                                / ( stock_level_num_total - stock_level_num_warmup ) ;
                        avg_db_rt = ( double ) ( stock_level_dbtime_total - stock_level_dbtime_warmup )
                                / ( stock_level_num_total - stock_level_num_warmup ) ;
                    }
                    max_rt = stock_level_time_max ;
                    max_db_rt = stock_level_dbtime_max ;
                    reportString.append( String.format( REPORT_VALUE,
                            "average", type, tpm, avg_rt, max_rt, avg_db_rt,
                            max_db_rt ) ) ;

                    type = "order_status" ;
                    tpm = ( double ) ( order_status_num_total - order_status_num_warmup )
                            / executeMinutes ;
                    if ( ( order_status_num_total - order_status_num_warmup ) == 0 ) {
                        avg_rt = 0 ;
                        avg_db_rt = 0 ;
                    } else {
                        avg_rt = ( double ) ( order_status_time_total - order_status_time_warmup )
                                / ( order_status_num_total - order_status_num_warmup ) ;
                        avg_db_rt = ( double ) ( order_status_dbtime_total - order_status_dbtime_warmup )
                                / ( order_status_num_total - order_status_num_warmup ) ;
                    }
                    max_rt = order_status_time_max ;
                    max_db_rt = order_status_dbtime_max ;
                    reportString.append( String.format( REPORT_VALUE,
                            "average", type, tpm, avg_rt, max_rt, avg_db_rt,
                            max_db_rt ) ) ;

                    type = "delivery" ;
                    tpm = ( double ) ( delivery_num_total - delivery_num_warmup )
                            / executeMinutes ;
                    if ( ( delivery_num_total - delivery_num_warmup ) == 0 ) {
                        avg_rt = 0 ;
                        avg_db_rt = 0 ;
                    } else {
                        avg_rt = ( double ) ( delivery_time_total - delivery_time_warmup )
                                / ( delivery_num_total - delivery_num_warmup ) ;
                        avg_db_rt = ( double ) ( delivery_dbtime_total - delivery_dbtime_warmup )
                                / ( delivery_num_total - delivery_num_warmup ) ;
                    }
                    max_rt = delivery_time_max ;
                    max_db_rt = delivery_dbtime_max ;
                    reportString.append( String.format( REPORT_VALUE,
                            "average", type, tpm, avg_rt, max_rt, avg_db_rt,
                            max_db_rt ) ) ;

                    type = "new_order" ;
                    tpm = ( double ) ( new_order_num_total - new_order_num_warmup )
                            / executeMinutes ;
                    if ( ( new_order_num_total - new_order_num_warmup ) == 0 ) {
                        avg_rt = 0 ;
                        avg_db_rt = 0 ;
                    } else {
                        avg_rt = ( double ) ( new_order_time_total - new_order_time_warmup )
                                / ( new_order_num_total - new_order_num_warmup ) ;
                        avg_db_rt = ( double ) ( new_order_dbtime_total - new_order_dbtime_warmup )
                                / ( new_order_num_total - new_order_num_warmup ) ;
                    }
                    max_rt = new_order_time_max ;
                    max_db_rt = new_order_dbtime_max ;
                    reportString.append( String.format( REPORT_VALUE,
                            "average", type, tpm, avg_rt, max_rt, avg_db_rt,
                            max_db_rt ) ) ;

                    logMessage( reportString.toString() ) ;

                    for ( Socket slave : this.slaves.values() ) {
                        DataOutputStream request = new DataOutputStream(
                                slave.getOutputStream() ) ;
                        request.writeBytes( SOCK_SIGNAL_RUN_END + "\n" ) ;
                    }
                    logMessage( "Waiting slaves to terminate users." ) ;
                    for ( Socket slave : this.slaves.values() ) {
                        while ( true ) {
                            Thread.sleep( 100 ) ;
                            try {
                                slave.sendUrgentData( 0xFF ) ;
                            } catch ( IOException e ) {
                                break ;
                            }
                        }
                    }
                    logMessage( "All slaves disconnected." ) ;
                    this.printStreamLogs.close() ;
                    break ;
                }
            } catch ( InterruptedException e ) {
                e.printStackTrace() ;
            } catch ( IOException e ) {
                e.printStackTrace() ;
            }
        }
    }

    public static void main( String[] args ) {
        if ( args.length != 1 ) {
            System.err
                    .println( "USAGE: java TPCCRunner.Master [properties file]" ) ;
            System.exit( -1 ) ;
        }
        try {
            new Thread( new Master( args[0] ) ).start() ;
        } catch ( IOException e ) {
            e.printStackTrace() ;
        }
    }

}
