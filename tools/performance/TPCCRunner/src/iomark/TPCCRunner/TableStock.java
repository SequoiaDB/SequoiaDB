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
 * Table stock
 * 
 * @author "Jarvis Wang"
 * @version 1.00
 */
public class TableStock implements Serializable {

    private static final long serialVersionUID = -6692835176719597893L ;
    public int s_i_id ;
    public int s_w_id ;
    public int s_order_cnt ;
    public int s_remote_cnt ;
    public int s_quantity ;
    public float s_ytd ;
    public String s_data ;
    public String s_dist_01 ;
    public String s_dist_02 ;
    public String s_dist_03 ;
    public String s_dist_04 ;
    public String s_dist_05 ;
    public String s_dist_06 ;
    public String s_dist_07 ;
    public String s_dist_08 ;
    public String s_dist_09 ;
    public String s_dist_10 ;

    public String toString() {
        StringBuffer desc = new StringBuffer() ;
        desc.append( "\n***************** Stock ********************" ) ;
        desc.append( "\n*       s_i_id = " + s_i_id ) ;
        desc.append( "\n*       s_w_id = " + s_w_id ) ;
        desc.append( "\n*   s_quantity = " + s_quantity ) ;
        desc.append( "\n*        s_ytd = " + s_ytd ) ;
        desc.append( "\n*  s_order_cnt = " + s_order_cnt ) ;
        desc.append( "\n* s_remote_cnt = " + s_remote_cnt ) ;
        desc.append( "\n*       s_data = " + s_data ) ;
        desc.append( "\n*    s_dist_01 = " + s_dist_01 ) ;
        desc.append( "\n*    s_dist_02 = " + s_dist_02 ) ;
        desc.append( "\n*    s_dist_03 = " + s_dist_03 ) ;
        desc.append( "\n*    s_dist_04 = " + s_dist_04 ) ;
        desc.append( "\n*    s_dist_05 = " + s_dist_05 ) ;
        desc.append( "\n*    s_dist_06 = " + s_dist_06 ) ;
        desc.append( "\n*    s_dist_07 = " + s_dist_07 ) ;
        desc.append( "\n*    s_dist_08 = " + s_dist_08 ) ;
        desc.append( "\n*    s_dist_09 = " + s_dist_09 ) ;
        desc.append( "\n*    s_dist_10 = " + s_dist_10 ) ;
        desc.append( "\n**********************************************" ) ;
        return desc.toString() ;
    }
}