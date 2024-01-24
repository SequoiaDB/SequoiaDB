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
 * Table customer
 * 
 * @author "Jarvis Wang"
 * @version 1.00
 */
public class TableCustomer implements Serializable {
    private static final long serialVersionUID = -7929796222327805957L ;
    public int c_id ;
    public int c_d_id ;
    public int c_w_id ;
    public int c_payment_cnt ;
    public int c_delivery_cnt ;
    public long c_since ;
    public float c_discount ;
    public float c_credit_lim ;
    public float c_balance ;
    public float c_ytd_payment ;
    public String c_credit ;
    public String c_last ;
    public String c_first ;
    public String c_street_1 ;
    public String c_street_2 ;
    public String c_city ;
    public String c_state ;
    public String c_zip ;
    public String c_phone ;
    public String c_middle ;
    public String c_data ;

    public String toString() {
        java.sql.Timestamp since = new java.sql.Timestamp( c_since ) ;
        StringBuffer desc = new StringBuffer() ;
        desc.append( "\n***************** Customer ********************" ) ;
        desc.append( "\n*           c_id = " + c_id ) ;
        desc.append( "\n*         c_d_id = " + c_d_id ) ;
        desc.append( "\n*         c_w_id = " + c_w_id ) ;
        desc.append( "\n*     c_discount = " + c_discount ) ;
        desc.append( "\n*       c_credit = " + c_credit ) ;
        desc.append( "\n*         c_last = " + c_last ) ;
        desc.append( "\n*        c_first = " + c_first ) ;
        desc.append( "\n*   c_credit_lim = " + c_credit_lim ) ;
        desc.append( "\n*      c_balance = " + c_balance ) ;
        desc.append( "\n*  c_ytd_payment = " + c_ytd_payment ) ;
        desc.append( "\n*  c_payment_cnt = " + c_payment_cnt ) ;
        desc.append( "\n* c_delivery_cnt = " + c_delivery_cnt ) ;
        desc.append( "\n*     c_street_1 = " + c_street_1 ) ;
        desc.append( "\n*     c_street_2 = " + c_street_2 ) ;
        desc.append( "\n*         c_city = " + c_city ) ;
        desc.append( "\n*        c_state = " + c_state ) ;
        desc.append( "\n*          c_zip = " + c_zip ) ;
        desc.append( "\n*        c_phone = " + c_phone ) ;
        desc.append( "\n*        c_since = " + since ) ;
        desc.append( "\n*       c_middle = " + c_middle ) ;
        desc.append( "\n*         c_data = " + c_data ) ;
        desc.append( "\n**********************************************" ) ;
        return desc.toString() ;
    }
}