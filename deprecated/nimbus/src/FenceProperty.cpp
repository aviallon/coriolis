
// This file is part of the Coriolis Project.
// Copyright (C) Laboratoire LIP6 - Departement ASIM
// Universite Pierre et Marie Curie
//
// Date   : 29/01/2004
// Author : Hugo Cl�ment                  <Hugo.Clement@lip6.fr>


#include "hurricane/Property.h"

#include "nimbus/FenceProperty.h"
#include "nimbus/Fence.h"

template<>
Hurricane::Name  Hurricane::StandardPrivateProperty<Nimbus::Fence*>::_name = "ComponentFencePropName";

namespace Nimbus {


Fence* getFence ( const Component& component )
{
  Property* property = component.getProperty(FenceProperty::staticGetName());
    if (!property) return NULL;
    
    FenceProperty* fenceProperty = dynamic_cast<FenceProperty*>(property);
    if (!fenceProperty) {
      throw Error("Property is not a FenceProperty");
    }

    return fenceProperty->getValue();
}


void setFence ( Component& component, Fence* fence ) 
{
  Property* property = component.getProperty(FenceProperty::staticGetName());
    if (!property) {
      property = FenceProperty::create(fence);
      component.put(property);
      return;
    }
    
    FenceProperty* fenceProperty = dynamic_cast<FenceProperty*>(property);
    if (!fenceProperty) {
      throw Error("Property is not a FenceProperty");
    }

    fenceProperty->setValue(fence);
}



}  // End of Nimbus namespace.
