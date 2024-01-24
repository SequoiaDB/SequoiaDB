/*
 * Copyright (c) 2011 IOMark.CO.CC All rights reserved.
 *
 * The contents of this file are subject to the terms of the IOMARK LICENSE.
 * You may not modify or re-distribute the contents of this file.
 *
 */
package iomark.TPCCRunner ;

import java.io.Serializable ;

/**
 * Table warehouse
 * 
 * @author "Jarvis Wang"
 * @version 1.00
 */
public class TableWarehouse implements Serializable {
    private static final long serialVersionUID = 8103535505127540517L ;
    public int w_id ; // PRIMARY KEY
    public float w_ytd ;
    public float w_tax ;
    public String w_name ;
    public String w_street_1 ;
    public String w_street_2 ;
    public String w_city ;
    public String w_state ;
    public String w_zip ;

    public String toString() {
        StringBuffer desc = new StringBuffer() ;
        desc.append( "\n***************** Warehouse ********************" ) ;
        desc.append( "\n*       w_id = " + w_id ) ;
        desc.append( "\n*      w_ytd = " + w_ytd ) ;
        desc.append( "\n*      w_tax = " + w_tax ) ;
        desc.append( "\n*     w_name = " + w_name ) ;
        desc.append( "\n* w_street_1 = " + w_street_1 ) ;
        desc.append( "\n* w_street_2 = " + w_street_2 ) ;
        desc.append( "\n*     w_city = " + w_city ) ;
        desc.append( "\n*    w_state = " + w_state ) ;
        desc.append( "\n*      w_zip = " + w_zip ) ;
        desc.append( "\n************************************************" ) ;
        return desc.toString() ;
    }
}