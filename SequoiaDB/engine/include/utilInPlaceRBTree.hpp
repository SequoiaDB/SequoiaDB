/*******************************************************************************

   Copyright (C) 2011-2023 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Source File Name = utilInPlaceRBTree.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/31/2023  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef UTIL_INPLACE_RBTREE_HPP__
#define UTIL_INPLACE_RBTREE_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "pd.hpp"


namespace engine
{

   /*
      struct T
      {
         T*    left() ;
         void  left( T* ) ;
         T*    right() ;
         void  right( T* ) ;
         T*    parent() ;
         void  parent( T* ) ;
         INT32 color() ;
         void  color( INT32 ) ;
         bool  operator<( const T& ) const ;
      }
   */

   template< typename T >
   class _utilInPlaceRBTree : public SDBObject
   {
      private:
         enum { RED, BLACK } ;

      public:
         enum TREE_DIR { FORWARD, BACKWARD } ;

      public:
         _utilInPlaceRBTree()
         {
             _root  = NULL ;
             _count = 0 ;
             _insertBalanceCount = 0 ;
             _deleteBalanceCount = 0 ;
         }
         ~_utilInPlaceRBTree()
         {
            clear() ;
         }

         void clear( bool unlink = false )
         {
             if ( unlink )
             {
                  T *x = begin() ;
                  T *y = NULL ;
                  while( x )
                  {
                      y = x ;
                      x = next( x ) ;
                      y->left( NULL ) ;
                      y->right( NULL ) ;
                      y->parent( NULL ) ;
                  }
             }
             _root  = NULL ;
             _count = 0 ;
             _insertBalanceCount = 0 ;
             _deleteBalanceCount = 0 ;
         }

         bool insert( T *x, T **ppPos = NULL )
         {
             return _insert( _root, x, ppPos ) ;
         }

         bool find( T *x )
         {
             return _find( _root, x ) ;
         }

         T* remove( T *x, bool checkExist = false, TREE_DIR dir = FORWARD )
         {
             if ( checkExist && !_checkExist( _root, x ) )
             {
                 SDB_ASSERT( FALSE, "Node is not exits in tree" ) ;
                 return NULL ;
             }
             return _delete( _root, x, dir ) ;
         }

         T* begin( TREE_DIR dir = FORWARD )
         {
             T *x = _root ;

             SDB_ASSERT( FORWARD == dir || BACKWARD == dir, "Invalid dir" ) ;

             if ( FORWARD == dir )
             {
                while( x && x->left() )
                {
                    x = x->left() ;
                }
             }
             else
             {
                 while( x && x->right() )
                 {
                     x = x->right() ;
                 }
             }
             return x ;
         }

         T* next( T *x, TREE_DIR dir = FORWARD )
         {
             SDB_ASSERT( FORWARD == dir || BACKWARD == dir, "Invalid dir" ) ;
             return _next( _root, x, dir ) ;
         }

         UINT32 count() const { return _count ; }
         UINT64 insertBalanceCount() const { return _insertBalanceCount ; }
         UINT64 deleteBalanceCount() const { return _deleteBalanceCount ; }

      private:
        /*
         *       xp                 xp
         *       /                  /
         *     x                   y
         *    / \      ===>       / \
         *   lx  y               x   ry
         *      / \             / \
         *     ly  ry          lx  ly  
         */
         void _rotate_left( T *&root, T *x )
         {
             // x is rotate node
             T* y = x->right() ;

             x->right( y->left() ) ;
             if ( y->left() )
             {
                 y->left()->parent( x ) ;
             }

             y->parent( x->parent() ) ;
             if ( x == root )
             {
                 root = y ;
             } 
             else if ( x == x->parent()->left() )
             {
                 x->parent()->left( y ) ;
             }
             else
             {
                 x->parent()->right( y ) ;
             }

             y->left( x ) ;
             x->parent( y ) ;  
         }

        /*
         *         xp               xp
         *        /                 /
         *       x                 y
         *      / \    ===>       / \
         *     y   rx           ly   x   
         *    / \                   / \
         *   ly  ry                ry  rx
         */
         void _rotate_right( T *&root, T *x )
         {
             T* y = x->left() ;

             x->left( y->right() ) ;
             if ( y->right() )
             {
                 y->right()->parent( x ) ;
             }

             y->parent( x->parent() ) ;
             if ( x == root )
             {
                 root = y ;
             } 
             else if ( x == x->parent()->right() )
             {
                 x->parent()->right( y ) ;
             }
             else
             {
                 x->parent()->left( y ) ;
             }

             y->right( x ) ;
             x->parent( y ) ;  
         }

         void _insert_balance( T *&root, T *z )
         {
             while ( z != root && RED == z->parent()->color() )
             {
                 ++_insertBalanceCount ;

                 if ( z->parent() == z->parent()->parent()->left() ) /// parent is left
                 {
                     if ( z->parent()->parent()->right() &&
                          RED == z->parent()->parent()->right()->color() ) /// uncle is red
                     {
                        /// grand parent change to red
                         z->parent()->parent()->color( RED ) ;
                         /// parent and uncle change to black
                         z->parent()->color( BLACK ) ;
                         z->parent()->parent()->right()->color( BLACK ) ; 
                         /// set z to grand parent, and loop check
                         z = z->parent()->parent() ;
                     }
                     else
                     {
                         if ( z == z->parent()->right() )
                         {
                             z = z->parent() ;
                             _rotate_left( root, z ) ;
                         }
                         z->parent()->color( BLACK ) ;
                         z->parent()->parent()->color( RED ) ;
                         _rotate_right( root, z->parent()->parent() ) ;
                     }
                 }
                 else
                 {
                     if ( z->parent()->parent()->left() &&
                          RED == z->parent()->parent()->left()->color() ) 
                     {
                         z->parent()->parent()->color( RED ) ;
                         z->parent()->color( BLACK ) ;
                         z->parent()->parent()->left()->color( BLACK ) ;
                         z = z->parent()->parent() ;
                     }
                     else 
                     {
                         if ( z == z->parent()->left() ) 
                         {
                             z = z->parent() ;
                             _rotate_right( root, z ) ;
                         }      
                         z->parent()->color( BLACK ) ;
                         z->parent()->parent()->color( RED ) ;
                         _rotate_left( root, z->parent()->parent() ) ;
                     }
                 }
             }

             root->color( BLACK ) ;
         }

         bool _find( T *root, T *z )
         {
             T *x = root ;

             while ( x )
             {
                 if ( *x < *z )
                 {
                     x = x->right() ;
                 }
                 else if ( *z < *x )
                 {
                     x = x->left() ;
                 }
                 else
                 {
                     /// find
                     return true ;
                 }
             }

             return false ;
         }

         bool _insert( T *&root, T *z, T **ppPos )
         {
             bool fromleft = true ;
             T* x = root ;
             T* y = NULL ;

             while ( x )
             {
                 y = x ;
                 if ( *x < *z )
                 {
                     fromleft = false ;
                     x = x->right() ;
                 }
                 else if ( *z < *x )
                 {
                     fromleft = true ;
                     x = x->left() ;
                 }
                 else
                 {
                     if ( ppPos )
                     {
                         *ppPos = y ;
                     }
                     return false ;
                 }
                 ++_insertBalanceCount ;
             }

             z->parent( y ) ;
             if ( !y )
             {
                 root = z ;
             }
             else if ( fromleft )
             {
                 y->left( z ) ;
             }
             else
             {
                 y->right( z ) ;
             }

             z->left( NULL ) ;
             z->right( NULL ) ;
             z->color( RED ) ;
             ++_count ;

             if ( ppPos )
             {
                *ppPos = z ;
             }

             _insert_balance( root, z ) ;

             return true ;
         }

         void _delete_balance( T *&root, T *x )
         {
             while ( ( x != root ) && ( BLACK == x->color() ) )
             {
                 ++_deleteBalanceCount ;

                 if ( x == x->parent()->left() )
                 {
                     T *z = x->parent()->right() ;
                     if ( RED == z->color() )
                     {
                         z->color( BLACK ) ;
                         x->parent()->color( RED ) ;
                         _rotate_left( root, x->parent() ) ;
                         z = x->parent()->right() ;
                     }

                     if ( ( !z->left() || BLACK == z->left()->color() ) &&
                          ( !z->right() || BLACK == z->right()->color() ) )
                     {
                         z->color( RED ) ;
                         x = x->parent() ;
                     }
                     else 
                     {
                         if ( !z->right() || BLACK == z->right()->color() )
                         {
                             z->left()->color( BLACK ) ;
                             z->color( RED ) ;
                             _rotate_right( root, z ) ;
                             z = x->parent()->right() ;
                         }

                         z->color( x->parent()->color() ) ;
                         x->parent()->color( BLACK ) ;
                         z->right()->color( BLACK ) ;
                         _rotate_left( root, x->parent() ) ;

                         x = root ;
                     }
                 }
                 else
                 {
                     T *z = x->parent()->left() ; 
                     if ( RED == z->color() ) 
                     {
                         z->color( BLACK ) ;
                         x->parent()->color( RED ) ;
                         _rotate_right( root, x->parent() ) ;
                         z = x->parent()->left() ;
                     }

                     if ( ( !z->left() || BLACK == z->left()->color() ) &&
                          ( !z->right() || BLACK == z->right()->color() ) )
                     {
                         z->color( RED ) ;
                         x = x->parent() ;
                     } 
                     else 
                     {
                         if ( !z->left() || BLACK == z->left()->color() )
                         {
                             z->right()->color( BLACK ) ;
                             z->color( RED ) ;
                             _rotate_left( root, z ) ;
                             z = x->parent()->left() ;
                         }

                         z->color( x->parent()->color() ) ;
                         x->parent()->color( BLACK ) ;
                         z->left()->color( BLACK ) ;
                         _rotate_right( root, x->parent() ) ;

                         x = root ;
                     }
                 }
             } /// end while

             x->color( BLACK ) ;
         }

         T* _delete( T *&root, T *z, TREE_DIR dir ) 
         {
             T *y = z ;
             T *x = NULL ;
             T *r = _next( root, y, dir ) ;

             if ( y->left() && y->right() )
             {
                 y = y->right() ;
                 while( y->left() )
                 {
                     y = y->left() ;
                     ++_deleteBalanceCount ;
                 }

                 _swap( root, z, y ) ;
                 y = z ;
             }

             if ( y->left() )
             {
                 x = y->left() ;
                 _transplant( root, y, x ) ;
             }
             else if ( y->right() )
             {
                 x = y->right() ;
                 _transplant( root, y, x ) ;
             }

             if ( x )
             {
                 if ( BLACK == y->color() )
                 {
                     _delete_balance( root, x ) ;
                 }
             }
             else
             {
                 /// the leaf and also the root node
                 if ( !y->parent() )
                 {
                     root = NULL ;
                 }
                 else
                 {
                     if ( BLACK == y->color() )
                     {
                         _delete_balance( root, y ) ;
                     }
                     _transplant( root, y, x ) ;
                 }
             }

             z->left( NULL ) ;
             z->right( NULL ) ;
             z->parent( NULL ) ;

             --_count ;

             return r ;
         }

         void _transplant( T *&root, T *u, T *v )
         {
             if ( !u->parent() )
             {
                 root = v ;
             }
             else if ( u == u->parent()->left() )
             {
                 u->parent()->left( v ) ;
             }
             else
             {
                 u->parent()->right( v ) ;
             }

             if ( v )
             {
                 v->parent( u->parent() ) ;
             }
         }

         void _swapParentSub( T *&root, T *parent, T *sub )
         {
            if ( sub->parent() == parent )
            {
                if ( parent->left() == sub )
                {
                    parent->left( sub->left() ) ;
                    if ( parent->left() )
                    {
                        parent->left()->parent( parent ) ;
                    }
                    sub->left( parent ) ;
                    sub->parent( parent->parent() ) ;
                    if ( !sub->parent() )
                    {
                        root = sub ;
                    }
                    else if ( sub->parent()->left() == parent )
                    {
                       sub->parent()->left( sub ) ;
                    }
                    else
                    {
                       sub->parent()->right( sub ) ;
                    }
                    parent->parent( sub ) ;
                    T *tmp = sub->right() ;
                    sub->right( parent->right() ) ;
                    if ( sub->right() )
                    {
                        sub->right()->parent( sub ) ;
                    }
                    parent->right( tmp ) ;
                    if ( parent->right() )
                    {
                        parent->right()->parent( parent ) ;
                    }
                }
                else
                {
                   parent->right( sub->right() ) ;
                   if ( parent->right() )
                   {
                       parent->right()->parent( parent ) ;
                   }
                   sub->right( parent ) ;
                   sub->parent( parent->parent() ) ;
                   if ( !sub->parent() )
                   {
                       root = sub ;
                   }
                   else if ( sub->parent()->left() == parent )
                   {
                      sub->parent()->left( sub ) ;
                   }
                   else
                   {
                      sub->parent()->right( sub ) ;
                   }
                   parent->parent( sub ) ;
                   T *tmp = sub->left() ;
                   sub->left( parent->left() ) ;
                   if ( sub->left() )
                   {
                       sub->left()->parent( sub ) ;
                   }
                   parent->left( tmp ) ;
                   if ( parent->left() )
                   {
                       parent->left()->parent( parent ) ;
                   }
                }
            }
            else
            {
               SDB_ASSERT( FALSE, "Not parent and sub" ) ;
            }
         }

         void _swap( T *&root, T *u, T *v )
         {
             /*
                   u   u       v  v
                  /     \     /    \
                 v       v   u      u
             */
             if ( u->parent() == v )
             {
                 _swapParentSub( root, v, u ) ;
             }
             else if ( v->parent() == u )
             {
                 _swapParentSub( root, u, v ) ;
             }
             else
             {
                 T *tmp = u->left() ;
                 u->left( v->left() ) ;
                 if ( u->left() )
                 {
                     u->left()->parent( u ) ;
                 }
                 v->left( tmp ) ;
                 if ( v->left() )
                 {
                     v->left()->parent( v ) ;
                 }

                 tmp = u->right() ;
                 u->right( v->right() ) ;
                 if ( u->right() )
                 {
                     u->right()->parent( u ) ;
                 }
                 v->right( tmp ) ;
                 if ( v->right() )
                 {
                     v->right()->parent( v ) ;
                 }

                 tmp = u->parent() ;
                 u->parent( v->parent() ) ;
                 if ( !u->parent() )
                 {
                    root = u ;
                 }
                 else if ( u->parent()->left() == v )
                 {
                     u->parent()->left( u ) ;
                 }
                 else
                 {
                     u->parent()->right( u ) ;
                 }
                 v->parent( tmp ) ;
                 if ( !v->parent() )
                 {
                     root = v ;
                 }
                 else if ( v->parent()->left() == u )
                 {
                     v->parent()->left( v ) ;
                 }
                 else
                 {
                     v->parent()->right( v ) ;
                 }
             }

             INT32 color = u->color() ;
             u->color( v->color() ) ;
             v->color( color ) ;
         }

         bool _checkExist( T *root, T *x )
         {
             T *y = x ;
             while ( y )
             {
                 if ( y == root )
                 {
                     return true ;
                 }
                 y = y->parent() ;
             }
             return false ;
         }

         T* _next( T *root, T *x, TREE_DIR dir )
         {
             T *y = NULL ;
             ///   1, 2, 3, 4
             ///         |->
             if ( FORWARD == dir )
             {
                  while( x )
                  {
                      if ( x->right() && x->right() != y )
                      {
                          x = x->right() ;
                          while( x->left() )
                          {
                              x = x->left() ;
                          }
                          break ;
                      }
                      else if ( x->parent() && x->parent()->left() == x )
                      {
                           x = x->parent() ;
                           break ;
                      }
                      y = x ;
                      x = x->parent() ;
                  }
             }
             ///   1, 2, 3, 4
             ///          <-|
             else if ( BACKWARD == dir )
             {
                 while( x )
                 {
                      if ( x->left() && x->left() != y )
                      {
                           x = x->left() ;
                           while ( x->right() )
                           {
                               x = x->right() ;
                           }
                           break ;
                      }
                      else if ( x->parent() && x->parent()->right() == x )
                      {
                           x = x->parent() ;
                           break ;
                      }
                      y = x ;
                      x = x->parent() ;
                 }
             }

             return x ;
         }

      private:
         T           *_root ;
         UINT32      _count ;

         /// stat info
         UINT64      _insertBalanceCount ;
         UINT64      _deleteBalanceCount ;

   } ;

}

#endif // UTIL_INPLACE_RBTREE_HPP__

