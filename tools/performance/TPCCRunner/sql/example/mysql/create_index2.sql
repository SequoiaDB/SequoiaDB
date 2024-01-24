use hsdb;
create unique index pk_warehouse
 on warehouse(w_id);

create unique index pk_district
 on district(d_w_id, d_id);

create unique index pk_customer
 on customer(c_w_id, c_d_id, c_id);

create index ndx_customer_name 
 on customer (c_w_id, c_d_id, c_last, c_first);

create unique index pk_oorder
 on oorder(o_w_id, o_d_id, o_id);

create unique index ndx_oorder_carrier 
 on oorder (o_w_id, o_d_id, o_carrier_id, o_id);
 
create unique index pk_new_order 
 on new_order(no_w_id, no_d_id, no_o_id);

create unique index pk_order_line
 on order_line (ol_w_id, ol_d_id, ol_o_id, ol_number);

create unique index pk_stock
 on stock (s_w_id, s_i_id);

create unique index pk_item
 on item(i_id);
