// ****************************************************************************************************
// File: ./Net.cpp
// Authors: R. Escassut
// Copyright (c) BULL S.A. 2000-2018, All Rights Reserved
//
// This file is part of Hurricane.
//
// Hurricane is free software: you can redistribute it  and/or  modify it under the  terms  of the  GNU
// Lesser General Public License as published by the Free Software Foundation, either version 3 of  the
// License, or (at your option) any later version.
//
// Hurricane is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without  even
// the implied warranty of MERCHANTABILITY or FITNESS FOR A  PARTICULAR  PURPOSE. See  the  Lesser  GNU
// General Public License for more details.
//
// You should have received a copy of the Lesser GNU General Public License along  with  Hurricane.  If
// not, see <http://www.gnu.org/licenses/>.
// ****************************************************************************************************

#include "hurricane/Warning.h"
#include "hurricane/Error.h"
#include "hurricane/Net.h"
#include "hurricane/Cell.h"
#include "hurricane/Instance.h"
#include "hurricane/Plug.h"
#include "hurricane/RoutingPad.h"
#include "hurricane/RoutingPads.h"
#include "hurricane/Pin.h"
#include "hurricane/Contact.h"
#include "hurricane/Vertical.h"
#include "hurricane/Horizontal.h"
#include "hurricane/Pad.h"
#include "hurricane/UpdateSession.h"
#include "hurricane/NetExternalComponents.h"
#include "hurricane/NetRoutingProperty.h"

namespace Hurricane {



// ****************************************************************************************************
// Filters declaration & implementation
// ****************************************************************************************************

class Net_IsCellNetFilter : public Filter<Net*> {
// *******************************************

    public: Net_IsCellNetFilter() {};

    public: Net_IsCellNetFilter(const Net_IsCellNetFilter& filter) {};

    public: Net_IsCellNetFilter& operator=(const Net_IsCellNetFilter& filter) {return *this;};

    public: virtual Filter<Net*>* getClone() const {return new Net_IsCellNetFilter(*this);};

    public: virtual bool accept(Net* net) const {return !net->isDeepNet();};

    public: virtual string _getString() const {return "<" + _TName("Net::IsCellNetFilter>");};

};

class Net_IsDeepNetFilter : public Filter<Net*> {
// *******************************************

    public: Net_IsDeepNetFilter() {};

    public: Net_IsDeepNetFilter(const Net_IsDeepNetFilter& filter) {};

    public: Net_IsDeepNetFilter& operator=(const Net_IsDeepNetFilter& filter) {return *this;};

    public: virtual Filter<Net*>* getClone() const {return new Net_IsDeepNetFilter(*this);};

    public: virtual bool accept(Net* net) const {return net->isDeepNet();};

    public: virtual string _getString() const {return "<" + _TName("Net::IsDeepNetFilter>");};

};

class Net_IsGlobalFilter : public Filter<Net*> {
// *******************************************

    public: Net_IsGlobalFilter() {};

    public: Net_IsGlobalFilter(const Net_IsGlobalFilter& filter) {};

    public: Net_IsGlobalFilter& operator=(const Net_IsGlobalFilter& filter) {return *this;};

    public: virtual Filter<Net*>* getClone() const {return new Net_IsGlobalFilter(*this);};

    public: virtual bool accept(Net* net) const {return net->isGlobal();};

    public: virtual string _getString() const {return "<" + _TName("Net::IsGlobalFilter>");};

};

class Net_IsExternalFilter : public Filter<Net*> {
// *********************************************

    public: Net_IsExternalFilter() {};

    public: Net_IsExternalFilter(const Net_IsExternalFilter& filter) {};

    public: Net_IsExternalFilter& operator=(const Net_IsExternalFilter& filter) {return *this;};

    public: virtual Filter<Net*>* getClone() const {return new Net_IsExternalFilter(*this);};

    public: virtual bool accept(Net* net) const {return net->isExternal();};

    public: virtual string _getString() const {return "<" + _TName("Net::IsExternalFilter>");};

};

class Net_IsClockFilter : public Filter<Net*> {
// ******************************************

    public: Net_IsClockFilter() {};

    public: Net_IsClockFilter(const Net_IsClockFilter& filter) {};

    public: Net_IsClockFilter& operator=(const Net_IsClockFilter& filter) {return *this;};

    public: virtual Filter<Net*>* getClone() const {return new Net_IsClockFilter(*this);};

    public: virtual bool accept(Net* net) const {return net->isClock();};

    public: virtual string _getString() const {return "<" + _TName("Net::IsClockFilter>");};

};

class Net_IsSupplyFilter : public Filter<Net*> {
// *******************************************

    public: Net_IsSupplyFilter() {};

    public: Net_IsSupplyFilter(const Net_IsSupplyFilter& filter) {};

    public: Net_IsSupplyFilter& operator=(const Net_IsSupplyFilter& filter) {return *this;};

    public: virtual Filter<Net*>* getClone() const {return new Net_IsSupplyFilter(*this);};

    public: virtual bool accept(Net* net) const {return net->isSupply();};

    public: virtual string _getString() const {return "<" + _TName("Net::IsSupplyFilter>");};

};

class Net_IsPowerFilter : public Filter<Net*> {
// *******************************************

    public: Net_IsPowerFilter() {};

    public: Net_IsPowerFilter(const Net_IsPowerFilter& filter) {};

    public: Net_IsPowerFilter& operator=(const Net_IsPowerFilter& filter) {return *this;};

    public: virtual Filter<Net*>* getClone() const {return new Net_IsPowerFilter(*this);};

    public: virtual bool accept(Net* net) const {return net->isPower();};

    public: virtual string _getString() const {return "<" + _TName("Net::IsPowerFilter>");};

};

class Net_IsGroundFilter : public Filter<Net*> {
// *******************************************

    public: Net_IsGroundFilter() {};

    public: Net_IsGroundFilter(const Net_IsGroundFilter& filter) {};

    public: Net_IsGroundFilter& operator=(const Net_IsGroundFilter& filter) {return *this;};

    public: virtual Filter<Net*>* getClone() const {return new Net_IsGroundFilter(*this);};

    public: virtual bool accept(Net* net) const {return net->isGround();};

    public: virtual string _getString() const {return "<" + _TName("Net::IsGroundFilter>");};

};



// ****************************************************************************************************
// Net_SlavePlugs implementation
// ****************************************************************************************************

class Net_SlavePlugs : public Collection<Plug*> {
// ********************************************

// Types
// *****

    public: typedef Collection<Plug*> Inherit;

    public: class Locator : public Hurricane::Locator<Plug*> {
    // *****************************************************

        public: typedef Hurricane::Locator<Plug*> Inherit;

        private: const Net* _net;
        private: Plug* _plug;
        private: InstanceLocator _instanceLocator;

        public: Locator(const Net* net = NULL);
        public: Locator(const Locator& locator);

        public: Locator& operator=(const Locator& locator);

        public: virtual Plug* getElement() const;
        public: virtual Hurricane::Locator<Plug*>* getClone() const;

        public: virtual bool isValid() const;

        public: virtual void progress();

        public: virtual string _getString() const;

    };

// Attributes
// **********

    private: const Net* _net;

// Constructors
// ************

    public: Net_SlavePlugs(const Net* net = NULL);
    public: Net_SlavePlugs(const Net_SlavePlugs& slavePlugs);

// Operators
// *********

    public: Net_SlavePlugs& operator=(const Net_SlavePlugs& slavePlugs);

// Accessors
// *********

    public: virtual Collection<Plug*>* getClone() const;
    public: virtual Hurricane::Locator<Plug*>* getLocator() const;

// Others
// ******

    public: virtual string _getString() const;

};



// ****************************************************************************************************
// Net implementation
// ****************************************************************************************************

Net::Net(Cell* cell, const Name& name)
// ***********************************
:    Inherit(),
    _cell(cell),
    _name(name),
    _arity(1),
    _isGlobal(false),
    _isExternal(false),
    _isAutomatic(false),
    _type(Type::LOGICAL), // default is Type::LOGICAL : no more Type::Undefined - Damien.Dupuis 01/10/2010
    _direction(),
    _position(0,0),
    _componentSet(),
    _rubberSet(),
    _nextOfCellNetMap(NULL),
    _mainName(this)
{
  if (not _cell)      throw Error( "Net::Net(): Can't create Hurricane::Net, NULL cell" );
  if (name.isEmpty()) throw Error( "Net::Net(): Can't create Hurricane::Net, empty name" );
  if (_cell->getNet(_name))
    throw Error( "Net::Net(): Can't create Hurricane::Net named \""+getString(_name)+"\""
               + " in Cell \""+getString(_cell->getName())+"\", already exists");
}

Net* Net::create(Cell* cell, const Name& name)
// *******************************************
{
    Net* net = new Net(cell, name);

    net->_postCreate();

    return net;
}

Box Net::getBoundingBox() const
// ****************************
{
    Box boundingBox;
    for_each_component(component, getComponents()) {
        boundingBox.merge(component->getBoundingBox());
        end_for;
    }
    return boundingBox;
}

RoutingPads Net::getRoutingPads() const
// ************************
{
    // return getComponents().getSubSet<RoutingPad*>();
    return SubTypeCollection<Component*, RoutingPad*>(getComponents());
}

Plugs Net::getPlugs() const
// ************************
{
    // return getComponents().getSubSet<Plug*>();
    return SubTypeCollection<Component*, Plug*>(getComponents());
}

Pins Net::getPins() const
// **********************
{
    // return getComponents().getSubSet<Pin*>();
    return SubTypeCollection<Component*, Pin*>(getComponents());
}

Contacts Net::getContacts() const
// ******************************
{
    // return getComponents().getSubSet<Contact*>();
    return SubTypeCollection<Component*, Contact*>(getComponents());
}

Segments Net::getSegments() const
// ******************************
{
    // return getComponents().getSubSet<Segment*>();
    return SubTypeCollection<Component*, Segment*>(getComponents());
}

Verticals Net::getVerticals() const
// ********************************
{
    // return getComponents().getSubSet<Vertical*>();
    return SubTypeCollection<Component*, Vertical*>(getComponents());
}

Horizontals Net::getHorizontals() const
// ************************************
{
    // return getComponents().getSubSet<Horizontal*>();
    return SubTypeCollection<Component*, Horizontal*>(getComponents());
}

Pads Net::getPads() const
// **********************
{
    // return getComponents().getSubSet<Pad*>();
    return SubTypeCollection<Component*, Pad*>(getComponents());
}

Plugs Net::getSlavePlugs() const
// *****************************
{
    return Net_SlavePlugs(this);
}

Plugs Net::getConnectedSlavePlugs() const
// **************************************
{
    return getSlavePlugs().getSubSet(Plug::getIsConnectedFilter());
}

Plugs Net::getUnconnectedSlavePlugs() const
// ****************************************
{
    return getSlavePlugs().getSubSet(Plug::getIsUnconnectedFilter());
}

NetFilter Net::getIsCellNetFilter()
// *******************************
{
    return Net_IsCellNetFilter();
}

NetFilter Net::getIsDeepNetFilter()
// *******************************
{
    return Net_IsDeepNetFilter();
}

NetFilter Net::getIsGlobalFilter()
// *******************************
{
    return Net_IsGlobalFilter();
}

NetFilter Net::getIsExternalFilter()
// *********************************
{
    return Net_IsExternalFilter();
}

NetFilter Net::getIsInternalFilter()
// *********************************
{
    return !Net_IsExternalFilter();
}

NetFilter Net::getIsClockFilter()
// ******************************
{
    return Net_IsClockFilter();
}

NetFilter Net::getIsSupplyFilter()
// *******************************
{
    return Net_IsSupplyFilter();
}

NetFilter Net::getIsPowerFilter()
// *******************************
{
    return Net_IsPowerFilter();
}

NetFilter Net::getIsGroundFilter()
// *******************************
{
    return Net_IsGroundFilter();
}

void Net::setName(Name name)
// *************************
{
  if (name != _name) {
    if (name.isEmpty())
      throw Error( "Net::setName(): Empty name, keep \"%s\"", getString(_name).c_str() );

    bool swapAlias = hasAlias( name );
    if (not swapAlias and _cell->getNet(name))
      throw Error( "Net::setName(): On \"%s\", another net named \"%s\" already exists."
                 , getString(_name).c_str()
                 , getString( name).c_str() );

    if (swapAlias) removeAlias(name);

    _cell->_getNetMap()._remove(this);
    std::swap( _name, name );
    _cell->_getNetMap()._insert(this);

    if (swapAlias) addAlias(name);
  }
}

void Net::setArity(const Arity& arity)
// ***********************************
{
    _arity = arity;
}

void Net::setGlobal(bool isGlobal)
// *******************************
{
    _isGlobal = isGlobal;
}

void Net::setExternal(bool isExternal)
// ***********************************
{
  if (isExternal != _isExternal) {
    if (not isExternal) {
      if (not getConnectedSlavePlugs().isEmpty())
        throw Error( "Net::setExternal(): Cannot set \"%s\" of \"%s\" internal, has connected slave plugs"
                   , getString(getName()).c_str()
                   , getString(getCell()->getName()).c_str()
                   );
      _direction = Direction::UNDEFINED;
    }
    _isExternal = isExternal;
    if (_isExternal) {
      setPosition( Point(0,0) );
      for ( Instance* instance : _cell->getSlaveInstances() ) {
        Plug::_create( instance, this );
      }
    }
  }
}

void Net::setAutomatic(bool isAutomatic)
// *************************************
{
    _isAutomatic = isAutomatic;
}

void Net::setType(const Type& type)
// ********************************
{
    _type = type;
}

void Net::setPosition(const Point& position)
// *****************************************
{
    if (_position != position) {
        for_each_plug(plug, getSlavePlugs()) {
            plug->invalidate(true);
            end_for;
        }
        _position = position;
    }
}

void Net::setDirection(const Direction& direction)
// ***********************************************
{
    _direction = direction;
}

void Net::setRoutingState(uint32_t state)
// **************************************
{ NetRoutingExtension::get(this)->setFlags( state ); }

bool Net::hasAlias(const Name& name) const
// ***************************************
{
  if (name == _name) return true;
  for ( NetAliasHook* alias : getAliases() ) {
    if (alias->getName() == name) return true;
  }
  return false;
}

NetAliasHook* Net::getAlias(const Name& name) const
// ************************************************
{
  if (name == _name) return dynamic_cast<NetAliasHook*>( const_cast<NetMainName*>( &_mainName ));
  for ( NetAliasHook* alias : getAliases() ) {
    if (alias->getName() == name) return alias;
  }
  return NULL;
}

bool Net::addAlias(const Name& name, bool isExternal )
// ***************************************************
{
  NetAliasHook* alias = getAlias( name );
  if (alias) {
    if (isExternal) alias->setExternal( true );
    return true;
  }

  if (getCell()->getNet(name)) {
    cerr << Warning( "Net::addAlias(): Cannot add alias %s to net %s, already taken."
                   , getString(name).c_str()
                   , getString(getName()).c_str()
                   ) << endl;
    return false;
  }

  NetAliasName* slave = new NetAliasName ( name, isExternal );
  _mainName.attach( slave );
  getCell()->_addNetAlias( slave );

  return true;
}

bool Net::removeAlias(const Name& name )
// *************************************
{
  NetAliasName* slave = _mainName.find( name );
  if (slave) {
    slave->detach();
    getCell()->_removeNetAlias( slave );
    return true;
  }
  return false;
}

Net* Net::getClone(Cell* clonedCell)
// *********************************
{
  Net* clonedNet = Net::create( clonedCell, getName() );
  clonedNet->setArity    ( getArity() );
  clonedNet->setGlobal   ( isGlobal() );
  clonedNet->setExternal ( isExternal() );
  clonedNet->setType     ( getType() );
  clonedNet->setDirection( getDirection() );

  return clonedNet;
}

void Net::materialize()
// ********************
{
    for_each_component(component, getComponents()) {
        component->materialize();
        end_for;
    }
    for_each_rubber(rubber, getRubbers()) {
        rubber->materialize();
        end_for;
    }
}

void Net::unmaterialize()
// **********************
{
    for_each_rubber(rubber, getRubbers()) {
        rubber->unmaterialize();
        end_for;
    }
    for_each_component(component, getComponents()) {
        component->unmaterialize();
        end_for;
    }
}

static void mergeNets(Net* net1, Net* net2)
// ****************************************
{
    assert(net1);
    assert(net2);

    if (net2->getName()[0] != '~') {
        if ((net1->getName()[0] == '~') ||
             (net2->isGlobal() && !net1->isGlobal()) ||
             (net2->isExternal() && !net1->isExternal())) {
            Net* tmpNet = net1;
            net1 = net2;
            net2 = tmpNet;
        }
    }

    if (net2->isExternal() && !net1->isExternal()) {
        Net* tmpNet = net1;
        net1 = net2;
        net2 = tmpNet;
    }

    net1->merge(net2);
}

void Net::merge(Net* net)
// **********************
{
    cdebug_log(18,0) << "Net::merge(): " << this << " with " << net << " (deleted)." << endl;

    if (!net)
        throw Error("Can't merge net : null net");

    if (net == this)
        throw Error("Can't merge net : itself");

    if (net->getCell() != _cell)
      throw Error( "Net::merge(): Cannot merge %s (%s) with %s (%s)."
                 , getString(getName()).c_str()
                 , getString(getCell()->getName()).c_str()
                 , getString(net->getName()).c_str()
                 , getString(net->getCell()->getName()).c_str()
                 );

    if (!isExternal() && net->isExternal() && !net->getConnectedSlavePlugs().isEmpty())
        throw Error( "Net::merge(): Cannot merge external (%s) with an internal net (%s)."
                   , getString(net->getName()).c_str()
                   , getString(getName()).c_str()
                   );

    for_each_rubber(rubber, net->getRubbers()) rubber->_setNet(this); end_for;
    for_each_component(component, net->getComponents()) component->_setNet(this); end_for;

    if (isExternal() && net->isExternal()) {
        for_each_plug(plug, net->getConnectedSlavePlugs()) {
            Plug* mainPlug = plug->getInstance()->getPlug(this);
            if (mainPlug->isConnected() && (mainPlug->getNet() != plug->getNet()))
                mergeNets(mainPlug->getNet(), plug->getNet());
            end_for;
        }
        for_each_plug(plug, net->getConnectedSlavePlugs()) {
            Plug* mainPlug = plug->getInstance()->getPlug(this);
            if (!mainPlug->isConnected()) mainPlug->setNet(plug->getNet());
            Hook* masterHook = plug->getBodyHook();
            Hook* nextMasterHook = masterHook->getNextMasterHook();
            if (nextMasterHook != masterHook) {
                masterHook->detach();
                mainPlug->getBodyHook()->merge(nextMasterHook);
            }
            Hooks slaveHooks = masterHook->getSlaveHooks();
            while (!slaveHooks.isEmpty()) {
                Hook* slaveHook = slaveHooks.getFirst();
                slaveHook->detach();
                slaveHook->attach(mainPlug->getBodyHook());
            }
            plug->_destroy();
            end_for;
        }
    }

    bool mergedExternal = net->isExternal();
    Name mergedName     = net->getName();
    NetAliasName* slaves = NULL;
    if (net->_mainName.isAttached()) {
      slaves = dynamic_cast<NetAliasName*>( net->_mainName.getNext() );
      net->_mainName.detach();
    }

    if (mergedExternal and not isExternal()) {
      setExternal( true );
    }
    net->destroy();

    if (slaves) _mainName.attach( slaves );
    addAlias( mergedName, mergedExternal );
    
}

void Net::_postCreate()
// ********************
{
    _cell->_getNetMap()._insert(this);

    if (_isExternal) {
        for_each_instance(instance, _cell->getSlaveInstances()) {
            Plug::_create(instance, this);
            end_for;
        }
    }

    Inherit::_postCreate();
}

void Net::_preDestroy()
// *******************
{
  cdebug_log(18,1) << "entering Net::_preDestroy: " << this << endl;
  Inherit::_preDestroy();

  cdebug_log(18,0) << "Net::_preDestroy: " << this << " slave Plugs..." << endl;
  Plugs plugs = getSlavePlugs();
  while ( plugs.getFirst() ) plugs.getFirst()->_destroy();

  unmaterialize();

  cdebug_log(18,0) << "Net::_preDestroy: " << this << " slave Rubbers..." << endl;
  Rubbers rubbers = getRubbers();
  while ( rubbers.getFirst() ) rubbers.getFirst()->_destroy();

  for ( Component* component : getComponents() ) {
    for ( Hook* hook : component->getHooks() ) {
    // 15 05 2006 xtof : detach all hooks in rings when
    // a net deletion occurs, can't see why master hooks were not detached.
    //if (!hook->IsMaster()) hook->detach();
      hook->detach();
    // 24/02/2016 jpc: the answer, at last... we cannot iterate
    // over a collection as it is modificated/destroyed!
    }
  }

  cdebug_log(18,0) << "Net::_preDestroy: " << this << " RoutingPads..." << endl;
  vector<RoutingPad*> rps;
  for ( RoutingPad* rp : getRoutingPads() ) rps.push_back( rp );
  for ( RoutingPad* rp : rps ) rp->destroy();
    
  cdebug_log(18,0) << "Net::_preDestroy: " << this << " Components..." << endl;
  Components components = getComponents();
  while ( components.getFirst() ) {
    Component* component = components.getFirst();
    if (!dynamic_cast<Plug*>(component)) component->destroy();
    else (static_cast<Plug*>(component))->setNet(NULL);
  }

  cdebug_log(18,0) << "Net::_preDestroy: " << this << " Names/Aliases..." << endl;
  _mainName.clear();
  _cell->_getNetMap()._remove(this);

  cdebug_log(18,0) << "exiting Net::_preDestroy: " << this << endl;
  cdebug_tabw(18,-1);
}

string Net::_getFlagsAsString() const
// **********************************
{
  string ds;
  ds += ((isDeepNet() ) ? "d" : "-");
  ds += ((_isExternal ) ? "e" : "-");
  ds += ((_isGlobal   ) ? "g" : "-");
  ds += ((_isAutomatic) ? "a" : "-");
  return ds;
}

string Net::_getString() const
// ***************************
{
  string bs = Inherit::_getString();
  string ds = "\"" + getString(_name) + "\" ";
  ds += _getFlagsAsString();
  ds += " ";
  ds += getString(_type     ) + " ";
  ds += getString(_direction);
  
  bs.insert( bs.length() - 1, " " + ds );
  return bs;
}

Record* Net::_getRecord() const
// **********************
{
    Record* record = Inherit::_getRecord();
    if (record) {
        record->add(getSlot("_cell", _cell));
        record->add(getSlot("_name", &_name));
        record->add(getSlot("_arity", &_arity));
        record->add(getSlot("_isGlobal", &_isGlobal));
        record->add(getSlot("_isExternal", &_isExternal));
        record->add(getSlot("_isAutomatic", &_isAutomatic));
        record->add(getSlot("_type", &_type));
        record->add(getSlot("_direction", &_direction));
        record->add(getSlot("_position", &_position));
        record->add(getSlot("_componentsSet", &_componentSet));
        record->add(getSlot("_rubberSet", &_rubberSet));
        record->add(getSlot("_mainName", &_mainName));
    }
    return record;
}

void Net::_toJson(JsonWriter* writer) const
// ****************************************
{
  Inherit::_toJson( writer );

  jsonWrite( writer, "_name"        , getName()      );
  jsonWrite( writer, "_isGlobal"    , isGlobal()     );
  jsonWrite( writer, "_isExternal"  , isExternal()   );
  jsonWrite( writer, "_isAutomatic" , isAutomatic()  );
  jsonWrite( writer, "_type"        , getString(getType())      );
  jsonWrite( writer, "_direction"   , getString(getDirection()) );
}

void Net::_toJsonSignature(JsonWriter* writer) const
// *************************************************
{
  jsonWrite( writer, "_name", getName() );
}

void Net::_toJsonCollections(JsonWriter* writer) const
// ***************************************************
{
  jsonWrite( writer, "+aliases", getAliases() );
  writer->setFlags( JsonWriter::UsePlugReference );
  jsonWrite( writer, "+componentSet", getComponents() );
  writer->resetFlags( JsonWriter::UsePlugReference );

  writer->key( "+externalComponents" );
  NetExternalComponents::toJson( writer, this );

  Inherit::_toJsonCollections( writer );
}


// ****************************************************************************************************
// Net::Type implementation
// ****************************************************************************************************

Net::Type::Type(const Code& code)
// ******************************
:    _code(code)
{ }

Net::Type::Type(const Type& type)
// ******************************
:    _code(type._code)
{ }

Net::Type::Type(string s)
// **********************
:    _code(UNDEFINED)
{
  if      (s == "UNDEFINED") _code = UNDEFINED;
  else if (s == "LOGICAL"  ) _code = LOGICAL;
  else if (s == "CLOCK"    ) _code = CLOCK;
  else if (s == "POWER"    ) _code = POWER;
  else if (s == "GROUND"   ) _code = GROUND;
  else if (s == "BLOCKAGE" ) _code = BLOCKAGE;
  else if (s == "FUSED"    ) _code = FUSED;
}

Net::Type& Net::Type::operator=(const Type& type)
// **********************************************
{
    _code = type._code;
    return *this;
}

string Net::Type::_getString() const
// *********************************
{
  return getString(&_code);
}

Record* Net::Type::_getRecord() const
// ****************************
{
    Record* record = new Record(getString(this));
    record->add(getSlot("Code", &_code));
    return record;
}



// ****************************************************************************************************
// Net::Direction implementation
// ****************************************************************************************************

Net::Direction::Direction(const Code& code)
// ****************************************
:    _code(code)
{ }

Net::Direction::Direction(const Direction& direction)
// **************************************************
:    _code(direction._code)
{ }

Net::Direction::Direction(string s)
// ********************************
:    _code(UNDEFINED)
{
  if (s.size() > 3) {
    if (s[0] == 'i') *this |= DirIn;
    if (s[0] == 'o') *this |= DirOut;
    if (s[0] == 't') *this |= ConnTristate;
    if (s[0] == 'w') *this |= ConnWiredOr;
  }
}

Net::Direction& Net::Direction::operator=(const Direction& direction)
// ******************************************************************
{
    _code = direction._code;
    return *this;
}

Net::Direction& Net::Direction::operator|=(const Direction& direction)
// ******************************************************************
{
  _code = (Code)((unsigned int)_code | (unsigned int)direction._code);
  return *this;
}

string Net::Direction::_getString() const
// **************************************
{
  return getString(&_code);
}

Record* Net::Direction::_getRecord() const
// *********************************
{
    Record* record = new Record(getString(this));
    record->add(getSlot("Code", &_code));
    return record;
}



// ****************************************************************************************************
// Net::ComponentSet implementation
// ****************************************************************************************************

Net::ComponentSet::ComponentSet()
// ******************************
:    Inherit()
{
}

unsigned Net::ComponentSet::_getHashValue(Component* component) const
// ******************************************************************
{
  return component->getId() / 8;
}

Component* Net::ComponentSet::_getNextElement(Component* component) const
// **********************************************************************
{
    return component->_getNextOfNetComponentSet();
}

void Net::ComponentSet::_setNextElement(Component* component, Component* nextComponent) const
// ******************************************************************************************
{
    component->_setNextOfNetComponentSet(nextComponent);
}



// ****************************************************************************************************
// Net::RubberSet implementation
// ****************************************************************************************************

Net::RubberSet::RubberSet()
// ************************
:    Inherit()
{
}

unsigned Net::RubberSet::_getHashValue(Rubber* rubber) const
// *********************************************************
{
  return rubber->getId() / 8;
}

Rubber* Net::RubberSet::_getNextElement(Rubber* rubber) const
// **********************************************************
{
    return rubber->_getNextOfNetRubberSet();
}

void Net::RubberSet::_setNextElement(Rubber* rubber, Rubber* nextRubber) const
// ***************************************************************************
{
    rubber->_setNextOfNetRubberSet(nextRubber);
}



// ****************************************************************************************************
// Net_SlavePlugs implementation
// ****************************************************************************************************

Net_SlavePlugs::Net_SlavePlugs(const Net* net)
// *******************************************
:     Inherit(),
    _net(net)
{
}

Net_SlavePlugs::Net_SlavePlugs(const Net_SlavePlugs& slavePlugs)
// *************************************************************
:     Inherit(),
    _net(slavePlugs._net)
{
}

Net_SlavePlugs& Net_SlavePlugs::operator=(const Net_SlavePlugs& slavePlugs)
// ************************************************************************
{
    _net = slavePlugs._net;
    return *this;
}

Collection<Plug*>* Net_SlavePlugs::getClone() const
// ************************************************
{
    return new Net_SlavePlugs(*this);
}

Locator<Plug*>* Net_SlavePlugs::getLocator() const
// ***********************************************
{
    return new Locator(_net);
}

string Net_SlavePlugs::_getString() const
// **************************************
{
    string s = "<" + _TName("Net::SlavePlugs");
    if (_net) s += " " + getString(_net);
    s += ">";
    return s;
}



// ****************************************************************************************************
// Net_SlavePlugs::Locator implementation
// ****************************************************************************************************

Net_SlavePlugs::Locator::Locator(const Net* net)
// *********************************************
:    Inherit(),
    _net(net),
    _plug(NULL),
    _instanceLocator()
{
    if (_net) {
        _instanceLocator = _net->getCell()->getSlaveInstances().getLocator();
        while (!_plug && _instanceLocator.isValid()) {
            _plug = _instanceLocator.getElement()->getPlug(_net);
            _instanceLocator.progress();
        }
    }
}

Net_SlavePlugs::Locator::Locator(const Locator& locator)
// *****************************************************
:    Inherit(),
    _net(locator._net),
    _plug(locator._plug),
    _instanceLocator(locator._instanceLocator)
{
}

Net_SlavePlugs::Locator& Net_SlavePlugs::Locator::operator=(const Locator& locator)
// ********************************************************************************
{
    _net = locator._net;
    _plug = locator._plug;
    _instanceLocator = locator._instanceLocator;
    return *this;
}

Plug* Net_SlavePlugs::Locator::getElement() const
// **********************************************
{
    return _plug;
}

Locator<Plug*>* Net_SlavePlugs::Locator::getClone() const
// ******************************************************
{
    return new Locator(*this);
}

bool Net_SlavePlugs::Locator::isValid() const
// ******************************************
{
    return (_plug != NULL);
}

void Net_SlavePlugs::Locator::progress()
// *************************************
{
    if (isValid()) {
        _plug = NULL;
        while (!_plug && _instanceLocator.isValid()) {
            _plug = _instanceLocator.getElement()->getPlug(_net);
            _instanceLocator.progress();
        }
    }
}

string Net_SlavePlugs::Locator::_getString() const
// ***********************************************
{
    string s = "<" + _TName("Net::SlavePlugs::Locator");
    if (_net) s += " " + getString(_net);
    s += ">";
    return s;
}


// -------------------------------------------------------------------
// Class  :  "JsonNet".


  Initializer<JsonNet>  jsonNetInit ( 0 );


  void  JsonNet::initialize ()
  { JsonTypes::registerType( new JsonNet (JsonWriter::RegisterMode) ); }


  JsonNet::JsonNet ( unsigned long flags )
    : JsonEntity      (flags)
    , _autoMaterialize(not Go::autoMaterializationIsDisabled())
    , _net            (NULL)
    , _hooks          ()
  {
    if (flags & JsonWriter::RegisterMode) return;

    cdebug_log(19,0) << "JsonNet::JsonNet()" << endl;

    add( "_name"              , typeid(string)    );
    add( "_isGlobal"          , typeid(bool)      );
    add( "_isExternal"        , typeid(bool)      );
    add( "_isAutomatic"       , typeid(bool)      );
    add( "_type"              , typeid(string)    );
    add( "_direction"         , typeid(string)    );
    add( "+aliases"           , typeid(JsonArray) );
    add( "+componentSet"      , typeid(JsonArray) );
    add( "+externalComponents", typeid(JsonArray) );

    cdebug_log(19,0) << "Disabling auto-materialization (" << _autoMaterialize << ")." << endl;
    Go::disableAutoMaterialization();
  }


  JsonNet::~JsonNet ()
  {
    checkRings();
    buildRings();
    clearHookLinks();
    
    _net->materialize();

    if (_autoMaterialize) {
      Go::enableAutoMaterialization();
      cdebug_log(18,0) << "Enabling auto-materialization." << endl;
    }
  }


  string  JsonNet::getTypeName () const
  { return "Net"; }


  JsonNet* JsonNet::clone( unsigned long flags ) const
  { return new JsonNet ( flags ); }


  void JsonNet::toData ( JsonStack& stack )
  {
    cdebug_tabw(19,1);

    check( stack, "JsonNet::toData" );
    presetId( stack );

    _net = Net::create( get<Cell*>(stack,".Cell") , get<string>(stack,"_name") );
    _net->setGlobal   ( get<bool>(stack,"_isGlobal"   ) );
    _net->setExternal ( get<bool>(stack,"_isExternal" ) );
    _net->setAutomatic( get<bool>(stack,"_isAutomatic") );
    _net->setType     ( Net::Type     (get<string>(stack,"_type")) );
    _net->setDirection( Net::Direction(get<string>(stack,"_direction")) );

    update( stack, _net );

    cdebug_tabw(19,-1);
  }


  void  JsonNet::addHookLink ( Hook* hook, unsigned int jsonId, const string& jsonNext )
  {
    if (jsonNext.empty()) return;

    unsigned int id      = jsonId;
    string       tname   = hook->_getTypeName();

    auto ielement = _hooks.find( HookKey(id,tname) );
    if (ielement == _hooks.end()) {
      auto r = _hooks.insert( make_pair( HookKey(id,tname), HookElement(hook) ) );
      ielement = r.first;
      (*ielement).second.setFlags( HookElement::OpenRingStart );
    }
    HookElement* current = &((*ielement).second);
    if (not current->hook()) current->setHook( hook );

    hookFromString( jsonNext, id, tname );
    ielement = _hooks.find( HookKey(id,tname) );
    if (ielement == _hooks.end()) {
      auto r = _hooks.insert( make_pair( HookKey(id,tname), HookElement(NULL) ) );
      ielement = r.first;
    } else {
      (*ielement).second.resetFlags( HookElement::OpenRingStart );
    }
    current->setNext( &((*ielement).second) );
  }


  Hook* JsonNet::getHook ( unsigned int jsonId, const std::string& tname ) const
  {
    auto ihook = _hooks.find( HookKey(jsonId,tname) );
    if (ihook == _hooks.end()) return NULL;

    return (*ihook).second.hook();
  }


  bool  JsonNet::hookFromString ( std::string s, unsigned int& id, std::string& tname )
  {
    size_t dot = s.rfind('.');
    if (dot == string::npos) return false;

    tname = s.substr( 0, dot );
    id    = stoul( s.substr(dot+1) );
    return true;
  }


  bool  JsonNet::checkRings () const
  {
    bool status = true;

    for ( auto kv : _hooks ) {
      HookElement* ringStart = &(kv.second);
      if (ringStart->issetFlags(HookElement::ClosedRing)) continue;

      if (ringStart->issetFlags(HookElement::OpenRingStart)) {
        cerr << Error( "JsonNet::checkRing(): Open ring found, starting with %s.\n"
                     "        Closing the ring..."
                     , getString(ringStart->hook()).c_str() ) << endl;

        status = false;
        HookElement* element = ringStart;
        while ( true ) {
          if (not element->next()) {
          // The ring is open: close it (loop on ringStart).
            element->setNext( ringStart );
            element->setFlags( HookElement::ClosedRing );

            cerr << Error( "Simple open ring." ) << endl;
            break;
          }
          if (element->next()->issetFlags(HookElement::ClosedRing)) {
          // The ring is half merged with itself, or another ring.
          // (i.e. *multiple* hooks pointing the *same* next element)
            element->setNext( ringStart );
            element->setFlags( HookElement::ClosedRing );

            cerr << Error( "Complex fault: ring partially merged (convergent)." ) << endl;
            break;
          }
          element = element->next();
        }
      }
    }

    return status;
  }


  void  JsonNet::buildRings () const
  {
    for ( auto kv : _hooks ) {
      kv.second.hook()->_setNextHook( kv.second.next()->hook() );
    }
  }


} // Hurricane namespace.


// ****************************************************************************************************
// Copyright (c) BULL S.A. 2000-2018, All Rights Reserved
// ****************************************************************************************************
