from pysequoiadb.error import (SDBEndOfCursor)

def check_Result( cl, condition, expect_result, check_id ):
   actual_record_num = cl.get_count( condition = condition )
   expect_record_num = len(expect_result)
   if(actual_record_num != expect_record_num):
      raise Exception( 'check count error,actual_record_num is:' + str(actual_record_num) )
   cursor = cl.query( condition = condition )
   while True:
      try:
         record = cursor.next()
         if not check_id:
            del record['_id']
         if record not in expect_result:
            raise Exception( 'check record error, actual record is:' + str(record))
      except SDBEndOfCursor:
         break
   cursor.close()

