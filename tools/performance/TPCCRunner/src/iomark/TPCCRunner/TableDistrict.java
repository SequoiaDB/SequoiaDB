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
 * Table district
 * 
 * @author "Jarvis Wang"
 * @version 1.00
 */
public class TableDistrict implements Serializable {
    private static final long serialVersionUID = 7545911403391981213L ;
    public int d_id ;
    public int d_w_id ;
    public int d_next_o_id ;
    public float d_ytd ;
    public float d_tax ;
    public String d_name ;
    public String d_street_1 ;
    public String d_street_2 ;
    public String d_city ;
    public String d_state ;
    public String d_zip ;

    public String toString() {
        StringBuffer desc = new StringBuffer() ;
        desc.append( "\n***************** District ********************" ) ;
        desc.append( "\n*        d_id = " + d_id ) ;
        desc.append( "\n*      d_w_id = " + d_w_id ) ;
        desc.append( "\n*       d_ytd = " + d_ytd ) ;
        desc.append( "\n*       d_tax = " + d_tax ) ;
        desc.append( "\n* d_next_o_id = " + d_next_o_id ) ;
        desc.append( "\n*      d_name = " + d_name ) ;
        desc.append( "\n*  d_street_1 = " + d_street_1 ) ;
        desc.append( "\n*  d_street_2 = " + d_street_2 ) ;
        desc.append( "\n*      d_city = " + d_city ) ;
        desc.append( "\n*     d_state = " + d_state ) ;
        desc.append( "\n*       d_zip = " + d_zip ) ;
        desc.append( "\n**********************************************" ) ;
        return desc.toString() ;
    }
}