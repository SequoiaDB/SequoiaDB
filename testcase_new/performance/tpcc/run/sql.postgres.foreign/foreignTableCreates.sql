--DROP EXTENSION IF EXISTS sdb_test_fdw CASCADE;
CREATE EXTENSION IF NOT EXISTS sdb_fdw;
DROP SERVER IF EXISTS sdb_test_server CASCADE;
CREATE SERVER sdb_test_server FOREIGN DATA WRAPPER sdb_fdw options(address coordAddrs);

drop foreign table IF EXISTS bmsql_config;
create foreign table IF NOT EXISTS bmsql_config (
  cfg_name    varchar(30) ,
  cfg_value   varchar(50)
)server sdb_test_server options(collectionspace 'tpc_c', collection 'config');

drop foreign table IF EXISTS bmsql_warehouse;
create foreign table IF NOT EXISTS bmsql_warehouse (
  w_id        integer,
  w_ytd       decimal(12,2),
  w_tax       decimal(4,4),
  w_name      varchar(10),
  w_street_1  varchar(20),
  w_street_2  varchar(20),
  w_city      varchar(20),
  w_state     varchar(2),
  w_zip       varchar(9)
)server sdb_test_server options(collectionspace 'tpc_c', collection 'warehouse');

drop foreign table IF EXISTS bmsql_district;
create foreign table IF NOT EXISTS bmsql_district (
  d_w_id       integer   ,
  d_id         integer   ,
  d_ytd        decimal(12,2),
  d_tax        decimal(4,4),
  d_next_o_id  integer,
  d_name       varchar(10),
  d_street_1   varchar(20),
  d_street_2   varchar(20),
  d_city       varchar(20),
  d_state      varchar(2),
  d_zip        varchar(9)
)server sdb_test_server options(collectionspace 'tpc_c', collection 'district');

drop foreign table IF EXISTS bmsql_customer;
create foreign table IF NOT EXISTS bmsql_customer (
  c_w_id         integer  ,
  c_d_id         integer  ,
  c_id           integer  ,
  c_discount     decimal(4,4),
  c_credit       varchar(2),
  c_last         varchar(16),
  c_first        varchar(16),
  c_credit_lim   decimal(12,2),
  c_balance      decimal(12,2),
  c_ytd_payment  decimal(12,2),
  c_payment_cnt  integer,
  c_delivery_cnt integer,
  c_street_1     varchar(20),
  c_street_2     varchar(20),
  c_city         varchar(20),
  c_state        varchar(2),
  c_zip          varchar(9),
  c_phone        varchar(16),
  c_since        timestamp,
  c_middle       varchar(2),
  c_data         varchar(500)
)server sdb_test_server options(collectionspace 'tpc_c', collection 'customer');

drop sequence IF EXISTS bmsql_hist_id_seq CASCADE;
create sequence bmsql_hist_id_seq;

drop foreign table IF EXISTS bmsql_history;
create foreign table IF NOT EXISTS bmsql_history (
  hist_id  integer,
  h_c_id   integer,
  h_c_d_id integer,
  h_c_w_id integer,
  h_d_id   integer,
  h_w_id   integer,
  h_date   timestamp,
  h_amount decimal(6,2),
  h_data   varchar(24)
)server sdb_test_server options(collectionspace 'tpc_c', collection 'history');

drop foreign table IF EXISTS bmsql_new_order;
create foreign table IF NOT EXISTS bmsql_new_order (
  no_w_id  integer   ,
  no_d_id  integer   ,
  no_o_id  integer   
)server sdb_test_server options(collectionspace 'tpc_c', collection 'new_order');

drop foreign table IF EXISTS bmsql_oorder;
create foreign table IF NOT EXISTS bmsql_oorder (
  o_w_id       integer ,
  o_d_id       integer ,
  o_id         integer ,
  o_c_id       integer,
  o_carrier_id integer,
  o_ol_cnt     integer,
  o_all_local  integer,
  o_entry_d    timestamp
)server sdb_test_server options(collectionspace 'tpc_c', collection 'oorder');

drop foreign table IF EXISTS bmsql_order_line;
create foreign table IF NOT EXISTS bmsql_order_line (
  ol_w_id         integer   ,
  ol_d_id         integer   ,
  ol_o_id         integer   ,
  ol_number       integer   ,
  ol_i_id         integer   ,
  ol_delivery_d   timestamp,
  ol_amount       decimal(6,2),
  ol_supply_w_id  integer,
  ol_quantity     integer,
  ol_dist_info    varchar(24)
)server sdb_test_server options(collectionspace 'tpc_c', collection 'order_line');

drop foreign table IF EXISTS bmsql_item;
create foreign table IF NOT EXISTS bmsql_item (
  i_id     integer     ,
  i_name   varchar(24),
  i_price  decimal(5,2),
  i_data   varchar(50),
  i_im_id  integer
)server sdb_test_server options(collectionspace 'tpc_c', collection 'item');

drop foreign table IF EXISTS bmsql_stock;
create foreign table IF NOT EXISTS bmsql_stock (
  s_w_id       integer       ,
  s_i_id       integer       ,
  s_quantity   integer,
  s_ytd        integer,
  s_order_cnt  integer,
  s_remote_cnt integer,
  s_data       varchar(50),
  s_dist_01    varchar(24),
  s_dist_02    varchar(24),
  s_dist_03    varchar(24),
  s_dist_04    varchar(24),
  s_dist_05    varchar(24),
  s_dist_06    varchar(24),
  s_dist_07    varchar(24),
  s_dist_08    varchar(24),
  s_dist_09    varchar(24),
  s_dist_10    varchar(24)
)server sdb_test_server options(collectionspace 'tpc_c', collection 'stock');
