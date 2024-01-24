db=new Sdb();
db.createCS("tpcc",{Domain:"dtrans"});
db.tpcc.createCL("warehouse",{ShardingKey:{w_id:1},ShardingType:"hash",Partition:1024,ReplSize:-1,EnsureShardingIndex:false});
db.tpcc.createCL("district",{ShardingKey:{d_w_id:1, d_id:1},ShardingType:"hash",Partition:1024,ReplSize:-1,EnsureShardingIndex:false});
db.tpcc.createCL("customer",{ShardingKey:{c_w_id:1, c_d_id:1, c_id:1},ShardingType:"hash",Partition:1024,ReplSize:-1,EnsureShardingIndex:false});
db.tpcc.createCL("oorder",{ShardingKey:{o_w_id:1, o_d_id:1, o_id:1},ShardingType:"hash",Partition:1024,ReplSize:-1,EnsureShardingIndex:false});
db.tpcc.createCL("new_order",{ShardingKey:{no_w_id:1, no_d_id:1, no_o_id:1},ShardingType:"hash",Partition:1024,ReplSize:-1,EnsureShardingIndex:false});
db.tpcc.createCL("order_line",{ShardingKey:{ol_w_id:1, ol_d_id:1, ol_o_id:1, ol_number:1},ShardingType:"hash",Partition:1024,ReplSize:-1,EnsureShardingIndex:false});
db.tpcc.createCL("stock",{ShardingKey:{s_w_id:1, s_i_id:1},ShardingType:"hash",Partition:1024,ReplSize:-1,EnsureShardingIndex:false});
db.tpcc.createCL("item",{ShardingKey:{i_id:1},ShardingType:"hash",Partition:1024,ReplSize:-1,EnsureShardingIndex:false});
db.tpcc.createCL("history",{ShardingKey:{h_c_id:1,h_c_d_id:1,h_c_w_id:1},ShardingType:"hash",Partition:1024,ReplSize:-1,EnsureShardingIndex:false});
