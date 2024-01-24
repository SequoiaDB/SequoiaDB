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
 * Table order
 * 
 * @author "Jarvis Wang"
 * @version 1.00
 */
public class TableOorder implements Serializable {
    private static final long serialVersionUID = -721615621980459632L ;
    public int o_id ;
    public int o_w_id ;
    public int o_d_id ;
    public int o_c_id ;
    public int o_carrier_id ;
    public int o_ol_cnt ;
    public int o_all_local ;
    public long o_entry_d ;

    public String toString() {
        java.sql.Timestamp entry_d = new java.sql.Timestamp( o_entry_d ) ;
        StringBuffer desc = new StringBuffer() ;
        desc.append( "\n***************** Oorder ********************" ) ;
        desc.append( "\n*         o_id = " + o_id ) ;
        desc.append( "\n*       o_w_id = " + o_w_id ) ;
        desc.append( "\n*       o_d_id = " + o_d_id ) ;
        desc.append( "\n*       o_c_id = " + o_c_id ) ;
        desc.append( "\n* o_carrier_id = " + o_carrier_id ) ;
        desc.append( "\n*     o_ol_cnt = " + o_ol_cnt ) ;
        desc.append( "\n*  o_all_local = " + o_all_local ) ;
        desc.append( "\n*    o_entry_d = " + entry_d ) ;
        desc.append( "\n**********************************************" ) ;
        return desc.toString() ;
    }
}