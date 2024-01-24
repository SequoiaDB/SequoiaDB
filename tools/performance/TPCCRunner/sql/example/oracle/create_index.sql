alter session enable parallel dml;
alter session enable parallel ddl;

create unique index k_warehouse on warehouse(w_id) NOLOGGING;
create unique index k_district on district(d_w_id,d_id) NOLOGGING;
create unique index k_item on item(i_id) NOLOGGING;
create unique index ndx_oorder_carrier on oorder (o_w_id, o_d_id, o_carrier_id, o_id) NOLOGGING;
create unique index k_new_order on new_order(no_w_id, no_d_id, no_o_id) NOLOGGING;
create index ndx_customer_name on customer (c_w_id, c_d_id, c_last, c_first) NOLOGGING;
create unique index k_oorder on oorder(o_w_id, o_d_id, o_id) NOLOGGING;
create unique index k_customer on customer(c_w_id, c_d_id, c_id) NOLOGGING;
create unique index k_order_line on order_line(ol_w_id, ol_d_id, ol_o_id, ol_number) NOLOGGING;
create unique index k_stock on stock(s_w_id, s_i_id) NOLOGGING;

alter table warehouse add constraint pk_warehouse primary key (w_id) using index k_warehouse;
alter table district add constraint pk_district primary key (d_w_id, d_id) using index k_district;
alter table item add constraint pk_item primary key (i_id) using index k_item;
alter table new_order add constraint pk_new_order primary key (no_w_id, no_d_id, no_o_id) using index k_new_order;
alter table oorder add constraint pk_oorder primary key (o_w_id, o_d_id, o_id) using index k_oorder;
alter table customer add constraint pk_customer primary key (c_w_id, c_d_id, c_id) using index k_customer;
alter table order_line add constraint pk_order_line primary key (ol_w_id, ol_d_id, ol_o_id, ol_number) using index k_order_line;
alter table stock add constraint pk_stock primary key (s_w_id, s_i_id) using index k_stock;


