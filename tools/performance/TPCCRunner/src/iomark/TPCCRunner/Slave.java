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
import java.net.Socket ;
import java.sql.SQLException ;
import java.sql.Timestamp ;
import java.util.Properties ;
import java.util.Random ;
import java.util.concurrent.CountDownLatch ;

/**
 * Slave thread
 * 
 * @author "Jarvis Wang"
 * 
 */
public class Slave implements Config, Runnable {
    private long usersStarted = 0 ;
    private Random random = new Random( Util.genRandomSeed( Thread
            .currentThread().getId() ) ) ;

    private CountDownLatch countDownLatch = null ;
    private PrintStream printStreamErrors = null ;
    private PrintStream printStreamLogs = null ;

    private String name = null ;
    private Socket master = null ;
    private ConnectionPool connectionPool = null ;
    private String databaseType ;
    private int userCount ;
    private int warehouseCount = 0 ;
    private int startWarehouseID = 0 ;

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

    private User[] users ;

    private long startTimestamp ;
    private long warmupTimestamp ;

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

    public Slave( String propertiesFile ) throws IOException,
            ClassNotFoundException {

        // Get properties
        Properties properties = new Properties() ;
        properties.load( new FileInputStream( propertiesFile ) ) ;

        // Get my name
        this.name = properties.getProperty( "name" ) ;

        // create print stream
        this.printStreamErrors = new PrintStream( new FileOutputStream(
                "log/error_" + name + "_" + Util.getFileNameSuffix() + ".txt" ) ) ;

        this.printStreamLogs = new PrintStream( new FileOutputStream(
                "log/log_" + name + "_" + Util.getFileNameSuffix() + ".txt" ) ) ;

        // Create socket, send my name to master
        logMessage( "Connect to master..." ) ;
        String masterAddress = properties.getProperty( "masterAddress" ) ;
        int masterPort = Integer.parseInt( properties
                .getProperty( "masterPort" ) ) ;
        this.master = new Socket( masterAddress, masterPort ) ;
        BufferedReader request = new BufferedReader( new InputStreamReader(
                master.getInputStream() ) ) ;
        DataOutputStream response = new DataOutputStream(
                master.getOutputStream() ) ;
        try {
            if ( !request.readLine().equalsIgnoreCase( SOCK_GET_SLAVE_NAME ) ) {
                throw new InteractiveException(
                        "Master sends a wrong command when get slave name" ) ;
            }
            response.writeBytes( this.name + "\n" ) ;
            if ( !request.readLine().equalsIgnoreCase( SOCK_GET_SLAVE_NAME_OK ) ) {
                throw new InteractiveException(
                        "Master response error when get slave name" ) ;
            }

            // Create connection pool
            logMessage( "Create connection pool" ) ;
            String driver = properties.getProperty( "driver" ) ;
            String url = properties.getProperty( "url" ) ;
            String username = properties.getProperty( "user" ) ;
            String password = properties.getProperty( "password" ) ;
            int poolSize = Integer.parseInt( properties
                    .getProperty( "poolSize" ) ) ;
            if ( driver.endsWith( "SQLServerDriver" ) ) {
                databaseType = "SQLServer" ;
            } else if ( driver.endsWith( "IfxDriver" ) ) {
                databaseType = "Informix" ;
            } else {
                databaseType = "Compatible" ;
            }
            connectionPool = new ConnectionPool( driver, url, username,
                    password, poolSize ) ;

            // Get User and Warehouse properties and sync with master
            this.userCount = Integer.parseInt( properties
                    .getProperty( "userCount" ) ) ;
            this.warehouseCount = Integer.parseInt( properties
                    .getProperty( "warehouseCount" ) ) ;
            this.startWarehouseID = Integer.parseInt( properties
                    .getProperty( "startWarehouseID" ) ) ;
            if ( !request.readLine().equalsIgnoreCase(
                    SOCK_GET_USER_AND_WAREHOUSE ) ) {
                throw new InteractiveException(
                        "Master sends a wrong command when get user and warehouse properties" ) ;
            }
            response.writeBytes( String.format( "%d,%d\n", userCount,
                    warehouseCount ) ) ;
            if ( !request.readLine().equalsIgnoreCase(
                    SOCK_GET_USER_AND_WAREHOUSE_OK ) ) {
                throw new InteractiveException(
                        "Master response error when get user and warehouse properties" ) ;
            }

            // Get transaction properties from master
            String[] transactionConfig = request.readLine().split( "," ) ;
            if ( !transactionConfig[0]
                    .equalsIgnoreCase( SOCK_SEND_TRANSACTION_CONFIGS )
                    || transactionConfig.length != 11 ) {
                throw new InteractiveException(
                        "Master send wrong transaction properties" ) ;
            }
            this.newOrderPercent = Integer.parseInt( transactionConfig[1] ) ;
            this.paymentPercent = Integer.parseInt( transactionConfig[2] ) ;
            this.orderStatusPercent = Integer.parseInt( transactionConfig[3] ) ;
            this.deliveryPercent = Integer.parseInt( transactionConfig[4] ) ;
            this.stockLevelPercent = Integer.parseInt( transactionConfig[5] ) ;
            this.newOrderThinkSecond = Integer.parseInt( transactionConfig[6] ) ;
            this.paymentThinkSecond = Integer.parseInt( transactionConfig[7] ) ;
            this.orderStatusThinkSecond = Integer
                    .parseInt( transactionConfig[8] ) ;
            this.deliveryThinkSecond = Integer.parseInt( transactionConfig[9] ) ;
            this.stockLevelThinkSecond = Integer
                    .parseInt( transactionConfig[10] ) ;

            if ( warehouseCount <= 0 || startWarehouseID <= 0 ) {
                throw new InteractiveException(
                        "Wrong warehouseCount or startWarehouseID" ) ;
            }
            if ( userCount <= 0 || userCount > 10 * warehouseCount ) {
                throw new InteractiveException( "Wrong userCount" ) ;
            }
            if ( newOrderPercent < 0
                    || paymentPercent < 0
                    || orderStatusPercent < 0
                    || deliveryPercent < 0
                    || stockLevelPercent < 0
                    || 100 != newOrderPercent + paymentPercent
                            + orderStatusPercent + deliveryPercent
                            + stockLevelPercent ) {
                throw new InteractiveException(
                        "Wrong transaction percent values" ) ;
            }

            // Create Users
            logMessage( "Creating " + userCount + " user(s)" ) ;
            logMessage( "Transaction Weights: " + newOrderPercent
                    + "% New-Order, " + paymentPercent + "% Payment, "
                    + orderStatusPercent + "% Order-Status, " + deliveryPercent
                    + "% Delivery, " + stockLevelPercent + "% Stock-Level" ) ;
            users = new User[ userCount ] ;
            usersStarted = userCount ;
            int[][] usedUsers = new int[ warehouseCount ][ 10 ] ;
            for ( int i = 0; i < warehouseCount; i++ )
                for ( int j = 0; j < 10; j++ )
                    usedUsers[i][j] = 0 ;
            for ( int i = 0; i < userCount; i++ ) {
                int userWarehouseID ;
                int userDistrictID ;
                do {
                    userWarehouseID = ( int ) randomNumber( 1, warehouseCount ) ;
                    userDistrictID = ( int ) randomNumber( 1, 10 ) ;
                } while ( usedUsers[userWarehouseID - 1][userDistrictID - 1] == 1 ) ;
                usedUsers[userWarehouseID - 1][userDistrictID - 1] = 1 ;
                userWarehouseID += startWarehouseID ;
                String userName = userPrefix + i ;
                User user = new User( userName, userWarehouseID,
                        userDistrictID, connectionPool, paymentPercent,
                        orderStatusPercent, deliveryPercent, stockLevelPercent,
                        warehouseCount, this, newOrderThinkSecond,
                        paymentThinkSecond, orderStatusThinkSecond,
                        deliveryThinkSecond, stockLevelThinkSecond,
                        databaseType, printStreamErrors ) ;

                users[i] = user ;
            }
            logMessage( "Created " + userCount + " user(s) successfully!" ) ;

            // Start Transaction
            if ( !request.readLine().equalsIgnoreCase( SOCK_START_TRANSACTION ) ) {
                throw new InteractiveException(
                        "Master response error when get user and warehouse properties" ) ;
            }
            startTimestamp = System.currentTimeMillis() ;
            countDownLatch = new CountDownLatch( users.length ) ;
            synchronized ( users ) {
                logMessage( "Starting all users at "
                        + new Timestamp( startTimestamp ) ) ;
                for ( int i = 0; i < users.length; i++ )
                    ( new Thread( users[i] ) ).start() ;
            }
            logMessage( "All users started executing at "
                    + new Timestamp( startTimestamp ) ) ;
        } catch ( InteractiveException e ) {
            errorMessage( e.getMessage() ) ;
            this.master.close() ;
            System.exit( -1 ) ;
        } catch ( SQLException e ) {
            errorMessage( e.getMessage() ) ;
        }
    }

    public void signalUserEndedTransaction( String userName,
            String transactionType, long connTime, long dbTime ) {
        synchronized ( master ) {
            if ( transactionType.equalsIgnoreCase( "Payment" ) ) {
                this.payment_num_total += 1 ;
                this.payment_time_total += connTime ;
                if ( this.payment_time_max_last < connTime ) {
                    this.payment_time_max_last = connTime ;
                }
                this.payment_dbtime_total += dbTime ;
                if ( this.payment_dbtime_max_last < dbTime ) {
                    this.payment_dbtime_max_last = dbTime ;
                }
            } else if ( transactionType.equalsIgnoreCase( "Stock-Level" ) ) {
                this.stock_level_num_total += 1 ;
                this.stock_level_time_total += connTime ;
                if ( this.stock_level_time_max_last < connTime ) {
                    this.stock_level_time_max_last = connTime ;
                }
                this.stock_level_dbtime_total += dbTime ;
                if ( this.stock_level_dbtime_max_last < dbTime ) {
                    this.stock_level_dbtime_max_last = dbTime ;
                }
            } else if ( transactionType.equalsIgnoreCase( "Order-Status" ) ) {
                this.order_status_num_total += 1 ;
                this.order_status_time_total += connTime ;
                if ( this.order_status_time_max_last < connTime ) {
                    this.order_status_time_max_last = connTime ;
                }
                this.order_status_dbtime_total += dbTime ;
                if ( this.order_status_dbtime_max_last < dbTime ) {
                    this.order_status_dbtime_max_last = dbTime ;
                }
            } else if ( transactionType.equalsIgnoreCase( "Delivery" ) ) {
                this.delivery_num_total += 1 ;
                this.delivery_time_total += connTime ;
                if ( this.delivery_time_max_last < connTime ) {
                    this.delivery_time_max_last = connTime ;
                }
                this.delivery_dbtime_total += dbTime ;
                if ( this.delivery_dbtime_max_last < dbTime ) {
                    this.delivery_dbtime_max_last = dbTime ;
                }
            } else {
                this.new_order_num_total += 1 ;
                this.new_order_time_total += connTime ;
                if ( this.new_order_time_max_last < connTime ) {
                    this.new_order_time_max_last = connTime ;
                }
                this.new_order_dbtime_total += dbTime ;
                if ( this.new_order_dbtime_max_last < dbTime ) {
                    this.new_order_dbtime_max_last = dbTime ;
                }
            }
        }
    }

    public void signalUserEnded( User user ) {
        synchronized ( users ) {
            boolean found = false ;
            usersStarted-- ;
            for ( int i = 0; i < users.length && !found; i++ ) {
                if ( users[i] == user ) {
                    users[i] = null ;
                    found = true ;
                }
            }
            countDownLatch.countDown() ;
        }

        if ( usersStarted == 0 ) {
            logMessage( "All users finished executing "
                    + new Timestamp( System.currentTimeMillis() ) ) ;
            connectionPool.destroy() ;
            logMessage( "End destroy connectionPool" ) ;
        }
    }

    private void errorMessage( String message ) {
        synchronized ( printStreamErrors ) {
            printStreamErrors.println( new Timestamp( System
                    .currentTimeMillis() ) + " [ERROR] " + message ) ;
        }
    }

    private void logMessage( String message ) {
        printStreamLogs.println( message ) ;
        System.out.println( message ) ;
    }

    private long randomNumber( long min, long max ) {
        return ( long ) ( random.nextDouble() * ( max - min + 1 ) + min ) ;
    }

    public void run() {
        try {
            BufferedReader request = new BufferedReader( new InputStreamReader(
                    master.getInputStream() ) ) ;
            DataOutputStream response = new DataOutputStream(
                    master.getOutputStream() ) ;
            while ( true ) {
                String type = null ;
                double tpm = 0 ;
                double avg_rt = 0 ;
                long max_rt = 0 ;
                double avg_db_rt = 0 ;
                long max_db_rt = 0 ;
                StringBuilder reportString = null ;
                String command = request.readLine() ;
                if ( command.equalsIgnoreCase( SOCK_GET_TRANSACTION_COUNTERS ) ) {
                    long currentTimestamp = System.currentTimeMillis() ;
                    synchronized ( master ) {
                        response.writeBytes( String
                                .format(
                                        "%d,%d,%d,%d,%d %d,%d,%d,%d,%d %d,%d,%d,%d,%d %d,%d,%d,%d,%d %d,%d,%d,%d,%d\n",
                                        payment_num_total, payment_time_total,
                                        payment_time_max_last,
                                        payment_dbtime_total,
                                        payment_dbtime_max_last,
                                        stock_level_num_total,
                                        stock_level_time_total,
                                        stock_level_time_max_last,
                                        stock_level_dbtime_total,
                                        stock_level_dbtime_max_last,
                                        order_status_num_total,
                                        order_status_time_total,
                                        order_status_time_max_last,
                                        order_status_dbtime_total,
                                        order_status_dbtime_max_last,
                                        delivery_num_total,
                                        delivery_time_total,
                                        delivery_time_max_last,
                                        delivery_dbtime_total,
                                        delivery_dbtime_max_last,
                                        new_order_num_total,
                                        new_order_time_total,
                                        new_order_time_max_last,
                                        new_order_dbtime_total,
                                        new_order_dbtime_max_last ) ) ;

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
                                new Timestamp( currentTimestamp ), type, tpm,
                                avg_rt, max_rt, avg_db_rt, max_db_rt ) ) ;

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
                                new Timestamp( currentTimestamp ), type, tpm,
                                avg_rt, max_rt, avg_db_rt, max_db_rt ) ) ;

                        type = "order_status" ;
                        tpm = ( double ) ( order_status_num_total - order_status_num_total_last ) ;
                        if ( order_status_num_total
                                - order_status_num_total_last == 0 ) {
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
                                new Timestamp( currentTimestamp ), type, tpm,
                                avg_rt, max_rt, avg_db_rt, max_db_rt ) ) ;

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
                                new Timestamp( currentTimestamp ), type, tpm,
                                avg_rt, max_rt, avg_db_rt, max_db_rt ) ) ;

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
                                new Timestamp( currentTimestamp ), type, tpm,
                                avg_rt, max_rt, avg_db_rt, max_db_rt ) ) ;

                        logMessage( reportString.toString() ) ;

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
                        payment_num_total_last = payment_num_total ;
                        payment_dbtime_total_last = payment_dbtime_total ;
                        payment_time_total_last = payment_time_total ;
                        payment_dbtime_max_last = 0 ;
                        payment_time_max_last = 0 ;

                        stock_level_num_total_last = stock_level_num_total ;
                        stock_level_dbtime_total_last = stock_level_dbtime_total ;
                        stock_level_time_total_last = stock_level_time_total ;
                        stock_level_dbtime_max_last = 0 ;
                        stock_level_time_max_last = 0 ;

                        order_status_num_total_last = order_status_num_total ;
                        order_status_dbtime_total_last = order_status_dbtime_total ;
                        order_status_time_total_last = order_status_time_total ;
                        order_status_dbtime_max_last = 0 ;
                        order_status_time_max_last = 0 ;

                        delivery_num_total_last = delivery_num_total ;
                        delivery_dbtime_total_last = delivery_dbtime_total ;
                        delivery_time_total_last = delivery_time_total ;
                        delivery_dbtime_max_last = 0 ;
                        delivery_time_max_last = 0 ;

                        new_order_num_total_last = new_order_num_total ;
                        new_order_dbtime_total_last = new_order_dbtime_total ;
                        new_order_time_total_last = new_order_time_total ;
                        new_order_dbtime_max_last = 0 ;
                        new_order_time_max_last = 0 ;
                    }
                } else if ( command
                        .equalsIgnoreCase( SOCK_SIGNAL_WARMUP_PHASE_END ) ) {

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
                } else if ( command.equalsIgnoreCase( SOCK_SIGNAL_RUN_END ) ) {
                    long currentTimestamp = System.currentTimeMillis() ;
                    logMessage( "Run End.\n" ) ;

                    double executeMinutes = ( ( ( double ) ( currentTimestamp - this.warmupTimestamp ) / 1000 ) / 60 ) ;

                    reportString = new StringBuilder( REPORT_HEADER ) ;

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
                    logMessage( "Terminating users" ) ;
                    for ( User user : users ) {
                        user.stopRunningWhenPossible() ;
                    }
                    try {
                        countDownLatch.await() ;
                    } catch ( InterruptedException e1 ) {
                        e1.printStackTrace() ;
                    }
                    master.close() ;
                    this.printStreamErrors.close() ;
                    this.printStreamLogs.close() ;
                    break ;
                }
            }
        } catch ( IOException e ) {
            e.printStackTrace() ;
            for ( User user : users ) {
                user.stopRunningWhenPossible() ;
            }
            try {
                countDownLatch.await() ;
            } catch ( InterruptedException e1 ) {
                e1.printStackTrace() ;
            }
        }
    }

    public static void main( String[] args ) {
        if ( args.length != 1 ) {
            System.err
                    .println( "USAGE: java TPCCRunner.Slave [properties file]" ) ;
            System.exit( -1 ) ;
        }
        try {
            new Thread( new Slave( args[0] ) ).start() ;
        } catch ( IOException e ) {
            e.printStackTrace() ;
        } catch ( ClassNotFoundException e ) {
            e.printStackTrace() ;
        }
    }

}

class InteractiveException extends Exception {
    private static final long serialVersionUID = -479663584643889568L ;

    public InteractiveException( String errorMessage ) {
        super( errorMessage ) ;
    }
}