drop foreign table IF EXISTS bmsql_config ;
drop foreign table IF EXISTS bmsql_customer ;
drop foreign table IF EXISTS bmsql_district ;
drop sequence  IF EXISTS bmsql_hist_id_seq CASCADE;
drop foreign table IF EXISTS bmsql_history ;
drop foreign table IF EXISTS bmsql_item ;
drop foreign table IF EXISTS bmsql_new_order ;
drop foreign table IF EXISTS bmsql_oorder ;
drop foreign table IF EXISTS bmsql_order_line ;
drop foreign table IF EXISTS bmsql_stock ;
drop foreign table IF EXISTS bmsql_warehouse ;

drop server IF EXISTS sdb_test_server CASCADE;
