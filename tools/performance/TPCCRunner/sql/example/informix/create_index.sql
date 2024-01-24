create unique index pk_warehouse
 on warehouse(w_id);

alter table warehouse add constraint
 primary key (w_id) constraint pk_warehouse;  

create unique index pk_district
 on district(d_w_id, d_id);

alter table district add constraint
 primary key (d_w_id, d_id) constraint pk_district;

create unique index pk_customer
 on customer(c_w_id, c_d_id, c_id);

alter table customer add constraint
 primary key (c_w_id, c_d_id, c_id) constraint pk_customer;

create index ndx_customer_name 
 on customer (c_w_id, c_d_id, c_last, c_first);

create unique index pk_oorder
 on oorder(o_w_id, o_d_id, o_id);

alter table oorder add constraint
 primary key (o_w_id, o_d_id, o_id) constraint pk_oorder;

create unique index ndx_oorder_carrier 
 on oorder (o_w_id, o_d_id, o_carrier_id, o_id);
 
create unique index pk_new_order 
 on new_order(no_w_id, no_d_id, no_o_id);

alter table new_order add constraint
 primary key (no_w_id, no_d_id, no_o_id) constraint pk_new_order;

create unique index pk_order_line
 on order_line (ol_w_id, ol_d_id, ol_o_id, ol_number);

alter table order_line add constraint
 primary key (ol_w_id, ol_d_id, ol_o_id, ol_number) constraint pk_order_line;

create unique index pk_stock
 on stock (s_w_id, s_i_id);

alter table stock add constraint
 primary key (s_w_id, s_i_id) constraint pk_stock;

create unique index pk_item
 on item(i_id);

alter table item add constraint
 primary key (i_id) constraint pk_item;

