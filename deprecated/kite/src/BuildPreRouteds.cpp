// -*- C++ -*-
//
// This file is part of the Coriolis Software.
// Copyright (c) UPMC 2014-2018, All Rights Reserved
//
// +-----------------------------------------------------------------+
// |                   C O R I O L I S                               |
// |      K i t e  -  D e t a i l e d   R o u t e r                  |
// |                                                                 |
// |  Author      :                    Jean-Paul CHAPUT              |
// |  E-mail      :       Jean-Paul.Chaput@asim.lip6.fr              |
// | =============================================================== |
// |  C++ Module  :       "./BuildPreRouteds.cpp"                    |
// +-----------------------------------------------------------------+


#include <map>
#include <list>
#include "hurricane/Error.h"
#include "hurricane/Warning.h"
#include "hurricane/DataBase.h"
#include "hurricane/Technology.h"
#include "hurricane/BasicLayer.h"
#include "hurricane/RegularLayer.h"
#include "hurricane/Horizontal.h"
#include "hurricane/Vertical.h"
#include "hurricane/RoutingPad.h"
#include "hurricane/Pin.h"
#include "hurricane/NetExternalComponents.h"
#include "hurricane/NetRoutingProperty.h"
#include "hurricane/DeepNet.h"
#include "hurricane/Instance.h"
#include "hurricane/Plug.h"
#include "hurricane/Path.h"
#include "hurricane/Query.h"
#include "crlcore/AllianceFramework.h"
#include "katabatic/AutoContact.h"
#include "kite/RoutingPlane.h"
#include "kite/TrackFixedSegment.h"
#include "kite/Track.h"
#include "kite/KiteEngine.h"


namespace {


} // Anonymous namespace.


namespace Kite {

  using namespace std;
  using Hurricane::tab;
  using Hurricane::ForEachIterator;
  using Hurricane::Warning;
  using Hurricane::Error;
  using Hurricane::DbU;
  using Hurricane::Box;
  using Hurricane::Interval;
  using Hurricane::Horizontal;
  using Hurricane::Vertical;
  using Hurricane::RoutingPad;
  using Hurricane::Pin;
  using Hurricane::NetExternalComponents;
  using Hurricane::NetRoutingExtension;
  using Hurricane::NetRoutingState;
  using Hurricane::DeepNet;
  using Hurricane::Instance;
  using Hurricane::Plug;
  using Hurricane::Path;
  using Hurricane::Query;
  using Hurricane::Go;
  using Hurricane::Rubber;
  using Hurricane::Layer;
  using Hurricane::BasicLayer;
  using Hurricane::RegularLayer;
  using Hurricane::Transformation;
  using Hurricane::Technology;
  using Hurricane::DataBase;
  using CRL::AllianceFramework;
  using Hurricane::ForEachIterator;
  using Katabatic::AutoContact;
  using Katabatic::AutoSegment;
  using Katabatic::ChipTools;


  void  KiteEngine::buildPreRouteds ()
  {
    cmess1 << "  o  Looking for fixed or manually global routed nets." << endl;

    for( Net* net : getCell()->getNets() ) {
      if (net == _blockageNet) continue;
      if (net->getType() == Net::Type::POWER ) continue;
      if (net->getType() == Net::Type::GROUND) continue;
    // Don't skip the clock.

      vector<Segment*>  segments;
      vector<Contact*>  contacts;

      bool   isPreRouted = false;
      bool   isFixed     = false;
      size_t rpCount     = 0;

      if (net->isDeepNet()) {
        rpCount = 2;

        Net* rootNet = dynamic_cast<Net*>(
                         dynamic_cast<DeepNet*>(net)->getRootNetOccurrence().getEntity() );
        for( Component* component : rootNet->getComponents() ) {
          if (dynamic_cast<Horizontal*>(component)) { isFixed = true; break; }
          if (dynamic_cast<Vertical*>  (component)) { isFixed = true; break; }
          if (dynamic_cast<Contact*>   (component)) { isFixed = true; break; }
        }
      } else {
        for( Component* component : net->getComponents() ) {
          if (dynamic_cast<Pin*>(component)) continue;

          const RegularLayer* layer = dynamic_cast<const RegularLayer*>(component->getLayer());
          if (layer and (layer->getBasicLayer()->getMaterial() == BasicLayer::Material::blockage))
            continue;

          Horizontal* horizontal = dynamic_cast<Horizontal*>(component);
          if (horizontal) {
            segments.push_back( horizontal );
            isPreRouted = true;
            if (horizontal->getWidth() != Session::getWireWidth(horizontal->getLayer()))
              isFixed = true;
          } else {
            Vertical* vertical = dynamic_cast<Vertical*>(component);
            if (vertical) {
              isPreRouted = true;
              segments.push_back( vertical );
              if (vertical->getWidth() != Session::getWireWidth(vertical->getLayer()))
                isFixed = true;
            } else {
              Contact* contact = dynamic_cast<Contact*>(component);
              if (contact) {
                isPreRouted = true;
                contacts.push_back( contact );
                if (  (contact->getWidth () != Session::getViaWidth(contact->getLayer()))
                   or (contact->getHeight() != Session::getViaWidth(contact->getLayer())) )
                  isFixed = true;
              } else {
                RoutingPad* rp = dynamic_cast<RoutingPad*>(component);
                if (rp) {
                  ++rpCount;
                } else {
                  // Plug* plug = dynamic_cast<Plug*>(component);
                  // if (plug) {
                  //   cerr << "buildPreRouteds(): " << plug << endl;
                  //   ++rpCount;
                  // }
                }
              }
            }
          }
        }
      }

      if (isFixed or isPreRouted or (rpCount < 2)) {
        NetRoutingState* state = getRoutingState( net, Katabatic::KbCreate );
        state->unsetFlags( NetRoutingState::AutomaticGlobalRoute );
        state->setFlags  ( NetRoutingState::ManualDetailRoute );
        if (rpCount < 2)
          state->setFlags  ( NetRoutingState::Unconnected );

        if (isFixed) {
          cmess2 << "     - <" << net->getName() << "> is fixed." << endl;
          state->unsetFlags( NetRoutingState::ManualDetailRoute );
          state->setFlags  ( NetRoutingState::Fixed );
        } else {
          if (rpCount > 1) {
            cmess2 << "     - <" << net->getName() << "> is manually global routed." << endl;
            for ( auto icontact : contacts ) {
              AutoContact::createFrom( icontact );
            }
          
            for ( auto isegment : segments ) {
              AutoContact* source = Session::base()->lookup( dynamic_cast<Contact*>( isegment->getSource() ));
              AutoContact* target = Session::base()->lookup( dynamic_cast<Contact*>( isegment->getTarget() ));
              AutoSegment* autoSegment = AutoSegment::create( source, target, isegment );
              autoSegment->setFlags( Katabatic::SegUserDefined|Katabatic::SegAxisSet );
            }
          }
        }
      }
    }

    Session::revalidate();
  }


  void  KiteEngine::setFixedPreRouted ()
  {
    for ( size_t depth=0 ; depth<_routingPlanes.size() ; ++depth ) {
      RoutingPlane* rp = _routingPlanes[depth];
      if (rp->getLayerGauge()->getType() == Constant::PinOnly ) continue;
      if (rp->getLayerGauge()->getDepth() > getConfiguration()->getAllowedDepth() ) continue;

      size_t tracksSize = rp->getTracksSize();
      for ( size_t itrack=0 ; itrack<tracksSize ; ++itrack ) {
        Track* track = rp->getTrackByIndex( itrack );

        for ( size_t ielement=0 ; ielement<track->getSize() ; ++ielement ) {
          TrackElement* element = track->getSegment( ielement );
         
          if (element->getNet() == NULL) continue;
          element->setRouted();
        }
      }
    }
  }


} // Kite namespace.
