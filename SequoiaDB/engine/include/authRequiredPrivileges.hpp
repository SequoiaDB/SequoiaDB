#ifndef AUTH_REQUIRED_PRIVILEGES_HPP__
#define AUTH_REQUIRED_PRIVILEGES_HPP__

#include "ossTypes.hpp"
#include "authResource.hpp"
#include "authActionSet.hpp"
#include "utilSharedPtrHelper.hpp"

namespace engine
{
   class _authRequiredPrivileges : public SDBObject
   {
   public:
      typedef ossPoolVector< std::pair<boost::shared_ptr< authResource >,
                             const authRequiredActionSets*> >
         DATA_TYPE;

      void addActionSetsOnSimpleType( const authRequiredActionSets *actionSets )
      {
         RESOURCE_TYPE type = actionSets->getResourceType();
         if( type == RESOURCE_TYPE_CLUSTER || type == RESOURCE_TYPE_ANY ||
             type == RESOURCE_TYPE_NON_SYSTEM )
         {
            _data.push_back(std::make_pair(authResource::forSimpleType(type), actionSets));
         }
      }

      void addActionSetsOnResource( const boost::shared_ptr< authResource >&resource,
                                    const authRequiredActionSets *actionSets )
      {
         _data.push_back( std::make_pair( resource, actionSets ) );
      }

      DATA_TYPE &getWritableData()
      {
         return _data;
      }

      const DATA_TYPE &getData() const
      {
         return _data;
      }

   private:
      DATA_TYPE _data;
   };
   typedef _authRequiredPrivileges authRequiredPrivileges;
} // namespace engine

#endif