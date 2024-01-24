create extension sdb_fdw;
create server sdb_server foreign data wrapper sdb_fdw options(address '192.168.30.91:11810,192.168.30.94:11810',preferedinstance 'M');
create foreign table warehouse (
  w_id        integer   not null,
  w_ytd       decimal(12,2),
  w_tax       decimal(4,4),
  w_name      varchar(10),
  w_street_1  varchar(20),
  w_street_2  varchar(20),
  w_city      varchar(20),
  w_state     char(2),
  w_zip       char(9)
) server sdb_server options(collectionspace 'tpcc',collection 'warehouse',decimal 'on');

create foreign table district (
  d_w_id       integer       not null,
  d_id         integer       not null,
  d_ytd        decimal(12,2),
  d_tax        decimal(4,4),
  d_next_o_id  integer,
  d_name       varchar(10),
  d_street_1   varchar(20),
  d_street_2   varchar(20),
  d_city       varchar(20),
  d_state      char(2),
  d_zip        char(9)
) server sdb_server options(collectionspace 'tpcc',collection 'district',decimal 'on');

create foreign table customer (
  c_w_id         integer        not null,
  c_d_id         integer        not null,
  c_id           integer        not null,
  c_discount     decimal(4,4),
  c_credit       char(2),
  c_last         varchar(16),
  c_first        varchar(16),
  c_credit_lim   decimal(12,2),
  c_balance      decimal(12,2),
  c_ytd_payment  float,
  c_payment_cnt  integer,
  c_delivery_cnt integer,
  c_street_1     varchar(20),
  c_street_2     varchar(20),
  c_city         varchar(20),
  c_state        char(2),
  c_zip          char(9),
  c_phone        char(16),
  c_since        timestamp(0),
  c_middle       char(2),
  c_data         varchar(500)
) server sdb_server options(collectionspace 'tpcc',collection 'customer',decimal 'on');

create foreign table history (
  h_c_id   integer,
  h_c_d_id integer,
  h_c_w_id integer,
  h_d_id   integer,
  h_w_id   integer,
  h_date   timestamp(0),
  h_amount decimal(6,2),
  h_data   varchar(24)
) server sdb_server options(collectionspace 'tpcc',collection 'history',decimal 'on');

create foreign table oorder (
  o_w_id       integer      not null,
  o_d_id       integer      not null,
  o_id         integer      not null,
  o_c_id       integer,
  o_carrier_id integer,
  o_ol_cnt     decimal(2,0),
  o_all_local  decimal(1,0),
  o_entry_d    timestamp(0)
) server sdb_server options(collectionspace 'tpcc',collection 'oorder',decimal 'on');

create foreign table new_order (
  no_w_id  integer   not null,
  no_d_id  integer   not null,
  no_o_id  integer   not null
) server sdb_server options(collectionspace 'tpcc',collection 'new_order',decimal 'on');

create foreign table order_line (
  ol_w_id         integer   not null,
  ol_d_id         integer   not null,
  ol_o_id         integer   not null,
  ol_number       integer   not null,
  ol_i_id         integer   not null,
  ol_delivery_d   timestamp(0),
  ol_amount       decimal(6,2),
  ol_supply_w_id  integer,
  ol_quantity     decimal(2,0),
  ol_dist_info    char(24)
) server sdb_server options(collectionspace 'tpcc',collection 'order_line',decimal 'on');

create foreign table stock (
  s_w_id       integer       not null,
  s_i_id       integer       not null,
  s_quantity   decimal(4,0),
  s_ytd        decimal(8,2),
  s_order_cnt  integer,
  s_remote_cnt integer,
  s_data       varchar(50),
  s_dist_01    char(24),
  s_dist_02    char(24),
  s_dist_03    char(24),
  s_dist_04    char(24),
  s_dist_05    char(24),
  s_dist_06    char(24),
  s_dist_07    char(24),
  s_dist_08    char(24),
  s_dist_09    char(24),
  s_dist_10    char(24)
) server sdb_server options(collectionspace 'tpcc',collection 'stock',decimal 'on');

create foreign table item (
  i_id     integer      not null,
  i_name   varchar(24),
  i_price  decimal(5,2),
  i_data   varchar(50),
  i_im_id  integer
) server sdb_server options(collectionspace 'tpcc',collection 'item',decimal 'on');
