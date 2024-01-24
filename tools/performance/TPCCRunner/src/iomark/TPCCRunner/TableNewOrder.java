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
 * Table NewOrder
 * 
 * @author "Jarvis Wang"
 * @version 1.00
 */
public class TableNewOrder implements Serializable {
    private static final long serialVersionUID = -7157565243376951198L ;
    public int no_w_id ;
    public int no_d_id ;
    public int no_o_id ;

    public String toString() {
        StringBuffer desc = new StringBuffer() ;
        desc.append( "\n***************** NewOrder ********************" ) ;
        desc.append( "\n*      no_w_id = " + no_w_id ) ;
        desc.append( "\n*      no_d_id = " + no_d_id ) ;
        desc.append( "\n*      no_o_id = " + no_o_id ) ;
        desc.append( "\n**********************************************" ) ;
        return desc.toString() ;
    }
}