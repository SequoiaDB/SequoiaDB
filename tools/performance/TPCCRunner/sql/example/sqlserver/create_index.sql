USE hsdb
GO

alter table warehouse add constraint pk_warehouse 
  primary key (w_id)

alter table district add constraint pk_district 
  primary key (d_w_id, d_id)

alter table customer add constraint pk_customer 
  primary key (c_w_id, c_d_id, c_id)

create index ndx_customer_name 
  on customer (c_w_id, c_d_id, c_last, c_first)

alter table oorder add constraint pk_oorder 
  primary key (o_w_id, o_d_id, o_id)

create unique index ndx_oorder_carrier 
  on oorder (o_w_id, o_d_id, o_carrier_id, o_id)
 
alter table new_order add constraint pk_new_order 
  primary key (no_w_id, no_d_id, no_o_id)

alter table order_line add constraint pk_order_line 
  primary key (ol_w_id, ol_d_id, ol_o_id, ol_number)

alter table stock add constraint pk_stock
  primary key (s_w_id, s_i_id)

alter table item add constraint pk_item
  primary key (i_id)

GO

EXEC sp_MSForEachTable 'UPDATE STATISTICS ?'
GO

select t.name tablename,i.name indexname 
from sys.tables t,sys.indexes i 
where t.object_id=i.object_id and i.name is not null
GO

SELECT object_name(object_id) AS name, index_id, partition_number, rows, type_desc, 
       total_pages*8 as total_kb, used_pages*8 as used_kb, data_pages*8 as data_kb
FROM sys.partitions p JOIN sys.allocation_units a
ON p.partition_id = a.container_id
WHERE object_id in (select object_id from sys.tables where name not like '%bak')
GO