/******************************************************************************
@Description  seqDB-7430: 内置SQL语法中的子句的多表测试
                        1. inner join
                        2. left outer join
                        3. right ouert join
@author liyuanyue
@date 2020-4-14
******************************************************************************/
main( test );

function test ()
{
   var csName = COMMCSNAME;
   var personsclName = COMMCLNAME + "_7430_person";
   var ordersclName = COMMCLNAME + "_7430_order";

   // inner join
   commDropCL( db, csName, personsclName );
   commDropCL( db, csName, ordersclName );
   var cl_per = commCreateCL( db, csName, personsclName, {}, false );
   var cl_ord = commCreateCL( db, csName, ordersclName, {}, false );

   cl_per.insert( { "Id_P": 1, "LastName": "Adams", "FirstName": "John", "Address": "Oxford Street", "City": "London" } );
   cl_per.insert( { "Id_P": 2, "LastName": "Bush", "FirstName": "George", "Address": "Fifth Avenue", "City": "New York" } );
   cl_per.insert( { "Id_P": 3, "LastName": "Carter", "FirstName": "Thomas", "Address": "Changan Street", "City": "Beijing" } );
   cl_ord.insert( { "Id_O": 1, "OrderNo": 77895, "Id_P": 3 } );
   cl_ord.insert( { "Id_O": 2, "OrderNo": 44678, "Id_P": 3 } );
   cl_ord.insert( { "Id_O": 3, "OrderNo": 22456, "Id_P": 1 } );
   cl_ord.insert( { "Id_O": 4, "OrderNo": 24562, "Id_P": 1 } );
   cl_ord.insert( { "Id_O": 5, "OrderNo": 34764, "Id_P": 65 } );

   var rc = db.exec( "select t1.LastName, t1.FirstName, t2.OrderNo from " + csName + "." + personsclName + " as t1 inner join " + csName + "." + ordersclName + " as t2 on t1.Id_P=t2.Id_P" );
   var expRecs = [
      { "LastName": "Adams", "FirstName": "John", "OrderNo": 22456 },
      { "LastName": "Adams", "FirstName": "John", "OrderNo": 24562 },
      { "LastName": "Carter", "FirstName": "Thomas", "OrderNo": 77895 },
      { "LastName": "Carter", "FirstName": "Thomas", "OrderNo": 44678 }];
   commCompareResults( rc, expRecs );

   // use_hash()
   var rc = db.exec( "select t1.LastName, t1.FirstName, t2.OrderNo from " + csName + "." + personsclName + " as t1 inner join " + csName + "." + ordersclName + " as t2 on t1.Id_P=t2.Id_P /*+use_hash()*/" );
   var expRecs = [
      { "LastName": "Adams", "FirstName": "John", "OrderNo": 22456 },
      { "LastName": "Adams", "FirstName": "John", "OrderNo": 24562 },
      { "LastName": "Carter", "FirstName": "Thomas", "OrderNo": 77895 },
      { "LastName": "Carter", "FirstName": "Thomas", "OrderNo": 44678 }];
   commCompareResults( rc, expRecs );

   // left outer join 
   var rc = db.exec( "select t1.LastName, t1.FirstName, t2.OrderNo from " + csName + "." + personsclName + " as t1 left outer  join " + csName + "." + ordersclName + " as t2 on t1.Id_P=t2.Id_P" );
   var expRecs = [
      { "LastName": "Adams", "FirstName": "John", "OrderNo": 22456 },
      { "LastName": "Adams", "FirstName": "John", "OrderNo": 24562 },
      { "LastName": "Bush", "FirstName": "George", "OrderNo": null },
      { "LastName": "Carter", "FirstName": "Thomas", "OrderNo": 77895 },
      { "LastName": "Carter", "FirstName": "Thomas", "OrderNo": 44678 }];
   commCompareResults( rc, expRecs );

   var rc = db.exec( "select t1.LastName, t1.FirstName, t2.OrderNo from " + csName + "." + personsclName + " as t1 left outer  join " + csName + "." + ordersclName + " as t2 on t1.Id_P=t2.Id_P /*+use_hash()*/" );
   var expRecs = [
      { "LastName": "Adams", "FirstName": "John", "OrderNo": 22456 },
      { "LastName": "Adams", "FirstName": "John", "OrderNo": 24562 },
      { "LastName": "Bush", "FirstName": "George", "OrderNo": null },
      { "LastName": "Carter", "FirstName": "Thomas", "OrderNo": 77895 },
      { "LastName": "Carter", "FirstName": "Thomas", "OrderNo": 44678 }];
   commCompareResults( rc, expRecs );

   // right ouert join
   var rc = db.exec( "select t1.LastName, t1.FirstName, t2.OrderNo from " + csName + "." + personsclName + " as t1 right outer join " + csName + "." + ordersclName + " as t2 on t1.Id_P=t2.Id_P" );
   var expRecs = [
      { "LastName": "Carter", "FirstName": "Thomas", "OrderNo": 77895 },
      { "LastName": "Carter", "FirstName": "Thomas", "OrderNo": 44678 },
      { "LastName": "Adams", "FirstName": "John", "OrderNo": 22456 },
      { "LastName": "Adams", "FirstName": "John", "OrderNo": 24562 },
      { "LastName": null, "FirstName": null, "OrderNo": 34764 }];
   commCompareResults( rc, expRecs );

   var rc = db.exec( "select t1.LastName, t1.FirstName, t2.OrderNo from " + csName + "." + personsclName + " as t1 right outer join " + csName + "." + ordersclName + " as t2 on t1.Id_P=t2.Id_P /*+use_hash()*/" );
   var expRecs = [
      { "LastName": "Carter", "FirstName": "Thomas", "OrderNo": 77895 },
      { "LastName": "Carter", "FirstName": "Thomas", "OrderNo": 44678 },
      { "LastName": "Adams", "FirstName": "John", "OrderNo": 22456 },
      { "LastName": "Adams", "FirstName": "John", "OrderNo": 24562 },
      { "LastName": null, "FirstName": null, "OrderNo": 34764 }];
   commCompareResults( rc, expRecs );

   commDropCL( db, csName, personsclName );
   commDropCL( db, csName, ordersclName );
}