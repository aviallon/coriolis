
# This file is part of the Coriolis Software.
# Copyright (c) Sorbonne Université 2014-2021, All Rights Reserved
#
# +-----------------------------------------------------------------+
# |                   C O R I O L I S                               |
# |      C u m u l u s  -  P y t h o n   T o o l s                  |
# |                                                                 |
# |  Author      :                    Jean-Paul CHAPUT              |
# |  E-mail      :            Jean-Paul.Chaput@lip6.fr              |
# | =============================================================== |
# |  Python      :       "./plugins/clocktree/clocktree.py"         |
# +-----------------------------------------------------------------+

import sys
import traceback
import os.path
import optparse
import math
import Cfg
import Hurricane
from   Hurricane  import DbU
from   Hurricane  import Transformation
from   Hurricane  import Box
from   Hurricane  import Path
from   Hurricane  import Occurrence
from   Hurricane  import UpdateSession
from   Hurricane  import Breakpoint
from   Hurricane  import Net
from   Hurricane  import RoutingPad
from   Hurricane  import Contact
from   Hurricane  import Horizontal
from   Hurricane  import Vertical
from   Hurricane  import Plug
from   Hurricane  import Instance
from   Hurricane  import HyperNet
import Viewer
import CRL
from   CRL import RoutingLayerGauge
import helpers
from   helpers    import trace
from   helpers.io import ErrorMessage
import Etesian
import Unicorn
import plugins
from   plugins.cts.rsmt           import RSMT
from   plugins.chip.configuration import GaugeConf
from   plugins.chip.configuration import getPlugByNet
from   plugins.chip.configuration import getPlugByName
from   plugins.chip.configuration import getRpBb
from   plugins.chip.configuration import destroyNetComponents


class HTree ( object ):

  @staticmethod
  def create ( conf, cell, clockNet, clockBox ):
    if clockBox.isEmpty(): raise ErrorMessage( 3, 'ClockTree: The clock area is empty.' )

    aspectRatio = DbU.toLambda( clockBox.getWidth() ) / DbU.toLambda( clockBox.getHeight() )
    if aspectRatio > 1.5 or aspectRatio < 0.5:
      raise ErrorMessage( 3, 'ClockTree: aspect ratio %f is disproportionate, must be between 0.5 and 1.5.' \
                             % aspectRatio )

    ht = HTree( conf, cell, clockNet, clockBox )
    print( '  o  Creating clock H-Tree for "{}".'.format(cell.getName()) )
    print( '     - Clock is "{}"'.format(ht.masterClock.getName()) )
    ht.build()
    trace( 550, '\tht.build() OK\n' )
    ht.place()
    trace( 550, '\tht.place() OK\n' )
   #ht.route()
    print( '     - H-Tree depth: {}'.format(ht.getTreeDepth()) )
    trace( 550, '\tusedVTracks: %s\n' % str(ht.usedVTracks) )
    return ht

  def __init__ ( self, conf, cell, clockNet, area ):
    self.conf = conf
    
    self.minSide = Cfg.getParamInt('clockTree.minimumSide').asInt()
    if self.minSide < DbU.fromLambda(100.0):
      raise ErrorMessage( 3, 'ClockTree: clockTree.minimumSide (%g) is less than 100 lambda.' \
                             % DbU.toLambda(self.minSide) )
    print( '     - Minimum side for clock area: {}'.format(DbU.toLambda(self.minSide)) )

    UpdateSession.open()
    self.framework    = CRL.AllianceFramework.get()
    self.cell         = cell
    self.area         = area
    self.childs       = []
    self._getBufferIo()
    self.tieCell      = self.framework.getCell( 'rowend_x0', CRL.Catalog.State.Views )
    self.topBuffer    = Instance.create( self.cell, 'ck_htree', self.bufferCell )
    self.cloneds      = [ self.cell ]
    self.usedVTracks  = []
    self._feedCount   = 0

    self.masterClock = clockNet
    if not self.masterClock:
      for net in cell.getNets():
        if net.isClock():
          self.masterClock = net
          break
      if not self.masterClock:
        raise ErrorMessage( 3, 'ClockTree: Cell %s has no clock net.' % cell.getName() )
    self._createChildNet( self.topBuffer, 'ck_htree' )
    UpdateSession.close()

    return

  def _getBufferIo ( self ):
    self.bufferCell = self.framework.getCell( Cfg.getParamString('clockTree.buffer').asString()
                                            , CRL.Catalog.State.Views )
    if not self.bufferCell:
      raise ErrorMessage( 3, [ 'ClockTree: Buffer cell "%s" not found in library,' \
                               % Cfg.getParamString('clockTree.buffer').asString()
                             , '           please check the "clockTree.buffer" configuration parameter in "plugins.conf".' ] )

    for net in self.bufferCell.getNets():
      if not net.isExternal(): continue
      if     net.isGlobal(): continue
      if     net.getDirection() & Net.Direction.IN:  self.bufferIn  = net.getName()
      elif   net.getDirection() & Net.Direction.OUT: self.bufferOut = net.getName()

    trace( 550, '\tbufferIn :<%s>\n' % self.bufferIn  ) 
    trace( 550, '\tbufferOut:<%s>\n' % self.bufferOut ) 
    return

  def _createChildNet ( self, ibuffer, tag ):
    childNet = Net.create( self.cell, tag )
    childNet.setType( Net.Type.CLOCK )
    getPlugByName(ibuffer, self.bufferOut).setNet( childNet )
    return

  def feedCounter ( self ):
    self._feedCount += 1
    return self._feedCount

  def toXCellGrid ( self, x ): return x - (x % self.conf.cellGauge.getSliceStep ())
  def toYCellGrid ( self, y ): return y - (y % self.conf.cellGauge.getSliceHeight())

  def rpDistance  ( self, rp1, rp2 ):
    dx = abs( rp1.getX() - rp2.getX() )
    dy = abs( self.toYCellGrid(rp1.getY()) - self.toYCellGrid(rp2.getY()) )
    return dx+dy

  def placeInstance ( self, instance, x, y ):
    xslice = self.toXCellGrid(x)
    yslice = self.toYCellGrid(y)

    transformation = Transformation.Orientation.ID
    if ((yslice-self.area.getYMin()) // self.conf.cellGauge.getSliceHeight()) % 2 != 0:
      transformation = Transformation.Orientation.MY
      yslice        += self.conf.cellGauge.getSliceHeight()

    instance.setTransformation ( Transformation(xslice, yslice, transformation) )
    instance.setPlacementStatus( Instance.PlacementStatus.FIXED )
    return

  def destroyInstance ( self, instance ):
    transformation = instance.getTransformation()
    instanceWidth  = instance.getMasterCell().getAbutmentBox().getWidth()
    tieWidth       = self.tieCell.getAbutmentBox().getWidth()

    instance.destroy()
    x = transformation.getTx()
    for i in range(instanceWidth//tieWidth):
      feed = Instance.create( self.cell
                            , 'htree_feed_%i' % self.feedCounter()
                            , self.tieCell
                            , Transformation(x,transformation.getTy(),transformation.getOrientation())
                            , Instance.PlacementStatus.PLACED
                            )
      x += tieWidth
    return

  def getTreeDepth ( self ):
    return self.childs[0].getTreeDepth()

  def getLeafBufferUnder ( self, point ):
    return self.childs[0].getLeafBufferUnder( point )

  def addLeaf ( self, point, plugOccurrence ):
    self.childs[0].addLeaf( point, plugOccurrence )

  def build ( self ):
    self.childs.append( HTreeNode( self, self.topBuffer, self.area, '', HTreeNode.RootBranch ) )
    return

  def place ( self ):
    UpdateSession.open()
    center = self.area.getCenter()
    self.placeInstance( self.topBuffer, center.getX(), center.getY() )
    trace( 550, '\rplace top level\n' )
    self.usedVTracks += [ getRpBb(self.topBuffer, self.bufferIn).getCenter().getX() ]
    self.childs[0].place()
    UpdateSession.close()
    return

  def route ( self ):
    UpdateSession.open()
    self.childs[0].route()
    UpdateSession.close()
    return

  def prune ( self ):
    UpdateSession.open()
    self.childs[0].prune()
    UpdateSession.close()
    return

  def addCloned ( self, masterCell ):
    if not masterCell in self.cloneds: self.cloneds.append( masterCell )
    return

  def addDeepPlug ( self, topNet, path ):
    if path.isEmpty(): return None

    tailPath     = path.getTailPath()
    headInstance = path.getHeadInstance()
    headPlug     = getPlugByNet(headInstance,topNet)
    if headPlug:
      if tailPath.isEmpty(): return headPlug
      return self.addDeepPlug( headPlug.getMasterNet(), tailPath )

    masterCell = headInstance.getMasterCell()
    masterNet  = Net.create( masterCell, topNet.getName() )
    masterNet.setExternal ( True )
    masterNet.setType     ( Net.Type.CLOCK )
    masterNet.setDirection( Net.Direction.IN )
    headPlug   = headInstance.getPlug( masterNet )
    if not headPlug:
      raise ErrorMessage( 3, 'Plug not created for %s on instance %s of %s' \
                             % (topNet.getName(),headInstance.getName(),masterCell.getName()) )
    headPlug.setNet( topNet )
    self.addCloned( masterCell )

    if tailPath.isEmpty(): return headPlug
    return self.addDeepPlug( masterNet, tailPath )

  def _RSMTtoLayout( self, mst, net ):
    for node in mst.nodes:
      if not node.component:
        x = node.realX
        if node.realX in self.usedVTracks:
          x += self.conf.routingGauge.getLayerGauge(self.conf.verticalDeepDepth).getPitch()
       # This is a Steiner point.
        node.component = self.conf.createContact( net
                                                , x
                                                , node.y + self.conf.cellGauge.getSliceHeight()//2 - self.conf.routingGauge.getLayerGauge(self.horizontalDeepDepth).getPitch()
                                                , GaugeConf.DeepDepth )
        trace( 550, '\tCreate (Steiner) node.component: @Y%d (y:%d - %d) %s\n' \
                 % (DbU.toLambda(node.realY)
                   ,DbU.toLambda(node.y)
                   ,DbU.toLambda(self.conf.routingGauge.getLayerGauge(self.horizontalDeepDepth).getPitch())
                   ,node.component) )
      else:
       # This a terminal (graph) point
        for edge in node.edges:
          flags = GaugeConf.HAccess|GaugeConf.DeepDepth
          if not edge.isHorizontal():
            if node.isSame(edge.source) or edge.isVertical():
              flags &= ~GaugeConf.HAccess
              break
        flags |= GaugeConf.OffsetTop1
        if node.realX in self.usedVTracks:
          flags |= GaugeConf.OffsetRight1
        node.component = self.conf.rpAccess( node.component, flags )

    for edge in mst.edges:
      sourceContact = edge.source.component
      targetContact = edge.target.component

      if edge.isHorizontal():
        self.createHorizontal( sourceContact, targetContact, targetContact.getY(), GaugeConf.DeepDepth )
      elif edge.isVertical():
        self.createVertical  ( sourceContact, targetContact, sourceContact.getX(), GaugeConf.DeepDepth )
      else:
        turn = self.conf.createContact( edge.source.component.getNet()
                                      , sourceContact.getX()
                                      , targetContact.getY()
                                      , GaugeConf.DeepDepth )
        self.conf.createVertical  ( sourceContact, turn, sourceContact.getX(), GaugeConf.DeepDepth )
        self.conf.createHorizontal( turn, targetContact, targetContact.getY(), GaugeConf.DeepDepth )
    return

  def _connectLeafs ( self, leafBuffer, leafs ):
    trace( 550, ',+', '\tBuffer <%s> has %i leafs.\n' % (leafBuffer.getName(),len(leafs)) )
   #if len(leafs) == 0: return
   #
   #leafCk   = getPlugByName(leafBuffer,self.bufferOut).getNet()
   #bufferRp = self.rpByPlugName( leafBuffer, self.bufferOut, leafCk )
   #
   #rsmt = RSMT( leafCk.getName() )
   #rsmt.addNode( bufferRp, bufferRp.getX(), self.toYCellGrid(bufferRp.getY()) )
   #for leaf in leafs:
   #  registerRp = self.rpByOccurrence( leaf, leafCk )
   #  rsmt.addNode( registerRp, registerRp.getX(), self.toYCellGrid(registerRp.getY()) )
   #
   #rsmt.runI1S()
   #self._RSMTtoLayout( rsmt, leafCk )

    trace( 550, '-' )
    return

  def connectLeaf ( self ):
    trace( 550, '\tConnecting leafs.\n' )
    UpdateSession.open()
    
    leafsByBuffer    = {}
    hyperMasterClock = HyperNet.create( Occurrence(self.masterClock) )
    for plugOccurrence in hyperMasterClock.getTerminalNetlistPlugOccurrences():
      trace( 550, '\tAdding leaf <%s>.\n' % plugOccurrence )
      position   = plugOccurrence.getBoundingBox().getCenter()
      self.addLeaf( position, plugOccurrence )

    self.childs[0].connectLeafs()
    sys.stdout.flush()

    getPlugByName( self.topBuffer, self.bufferIn ).setNet( self.masterClock )
    UpdateSession.close()

    return

  def _rsave ( self, cell ):
    flags = CRL.Catalog.State.Physical
    if cell.getName().endswith('_cts'):
      flags = flags | CRL.Catalog.State.Logical
    self.framework.saveCell( cell, flags )

    for instance in  cell.getInstances():
      masterCell = instance.getMasterCell()
      if not masterCell.isTerminal():
        self._rsave( masterCell )

  def save ( self, topCell ):
    for cell in self.cloneds:
      cell.setName( cell.getName()+'_cts' )

    self._rsave( topCell )
    return


class HTreeNode ( object ):

  RootBranch   = 0x0001
  LeftBranch   = 0x0002
  RightBranch  = 0x0004
  UpBranch     = 0x0008
  DownBranch   = 0x0010

  def __init__ ( self, topTree, sourceBuffer, area, prefix, flags ):
    UpdateSession.open()
    self.topTree      = topTree
    self.childs       = []
    self.blLeafs      = []
    self.brLeafs      = []
    self.tlLeafs      = []
    self.trLeafs      = []
    self.flags        = flags
    self.sourceBuffer = sourceBuffer
    self.area         = area
    self.prefix       = prefix

    self.blBuffer     = Instance.create( self.topTree.cell, 'ck_htree'+self.prefix+'_bl_ins', self.topTree.bufferCell )
    self.brBuffer     = Instance.create( self.topTree.cell, 'ck_htree'+self.prefix+'_br_ins', self.topTree.bufferCell )
    self.tlBuffer     = Instance.create( self.topTree.cell, 'ck_htree'+self.prefix+'_tl_ins', self.topTree.bufferCell )
    self.trBuffer     = Instance.create( self.topTree.cell, 'ck_htree'+self.prefix+'_tr_ins', self.topTree.bufferCell )
    self.ckNet        = getPlugByName(self.sourceBuffer, self.topTree.bufferOut).getNet()
    getPlugByName(self.blBuffer, self.topTree.bufferIn).setNet( self.ckNet )
    getPlugByName(self.brBuffer, self.topTree.bufferIn).setNet( self.ckNet )
    getPlugByName(self.tlBuffer, self.topTree.bufferIn).setNet( self.ckNet )
    getPlugByName(self.trBuffer, self.topTree.bufferIn).setNet( self.ckNet )

    self.topTree._createChildNet( self.blBuffer, 'ck_htree'+self.prefix+'_bl' )
    self.topTree._createChildNet( self.brBuffer, 'ck_htree'+self.prefix+'_br' )
    self.topTree._createChildNet( self.tlBuffer, 'ck_htree'+self.prefix+'_tl' )
    self.topTree._createChildNet( self.trBuffer, 'ck_htree'+self.prefix+'_tr' )

    halfWidth  = self.area.getHalfWidth ()
    halfHeight = self.area.getHalfHeight()
    if halfWidth >= self.topTree.minSide and halfHeight >= self.topTree.minSide:
     # Recursive call.
      self.childs.append( HTreeNode( self.topTree, self.blBuffer, self.blArea(), self.prefix+'_bl', 0 ) )
      self.childs.append( HTreeNode( self.topTree, self.brBuffer, self.brArea(), self.prefix+'_br', 0 ) )
      self.childs.append( HTreeNode( self.topTree, self.tlBuffer, self.tlArea(), self.prefix+'_tl', 0 ) )
      self.childs.append( HTreeNode( self.topTree, self.trBuffer, self.trArea(), self.prefix+'_tr', 0 ) )

    UpdateSession.close()
    return

  def xmin      ( self ): return self.area.getXMin()
  def xmax      ( self ): return self.area.getXMax()
  def ymin      ( self ): return self.area.getYMin()
  def ymax      ( self ): return self.area.getYMax()
  def halfWidth ( self ): return self.area.getHalfWidth()
  def halfHeight( self ): return self.area.getHalfHeight()

  def blArea ( self ):
    return Box( self.xmin()                 , self.ymin()
              , self.xmin()+self.halfWidth(), self.ymin()+self.halfHeight() )

  def brArea ( self ):
    return Box( self.xmin()+self.halfWidth(), self.ymin()
              , self.xmax()                 , self.ymin()+self.halfHeight() )

  def tlArea ( self ):
    return Box( self.xmin()                 , self.ymin()+self.halfHeight()
              , self.xmin()+self.halfWidth(), self.ymax() )

  def trArea ( self ):
    return Box( self.xmin()+self.halfWidth(), self.ymin()+self.halfHeight()
              , self.xmax()                 , self.ymax() )

  def getTreeDepth ( self ):
    if self.childs: return self.childs[0].getTreeDepth()+1
    return 1

  def hasLeafs ( self ):
    hasLeafsFlag = False
    if self.childs:
      hasLeafsFlag |= self.childs[0].hasLeafs()
      hasLeafsFlag |= self.childs[1].hasLeafs()
      hasLeafsFlag |= self.childs[2].hasLeafs()
      hasLeafsFlag |= self.childs[3].hasLeafs()
      return hasLeafsFlag

    hasLeafsFlag |= (len(self.blLeafs) != 0)
    hasLeafsFlag |= (len(self.brLeafs) != 0)
    hasLeafsFlag |= (len(self.tlLeafs) != 0)
    hasLeafsFlag |= (len(self.trLeafs) != 0)
    return hasLeafsFlag

  def addLeaf ( self, point, plugOccurrence ):
    if self.childs:
      if    self.blArea().contains(point): self.childs[0].addLeaf( point, plugOccurrence )
      elif  self.brArea().contains(point): self.childs[1].addLeaf( point, plugOccurrence )
      elif  self.tlArea().contains(point): self.childs[2].addLeaf( point, plugOccurrence )
      else: self.childs[3].addLeaf( point, plugOccurrence )
      return

    leafBuffer = None
    if self.blArea().contains(point):
      self.blLeafs.append( plugOccurrence )
      leafBuffer = self.blBuffer
    elif self.brArea().contains(point):
      self.brLeafs.append( plugOccurrence )
      leafBuffer = self.brBuffer
    elif self.tlArea().contains(point):
      self.tlLeafs.append( plugOccurrence )
      leafBuffer = self.tlBuffer
    else:
      self.trLeafs.append( plugOccurrence )
      leafBuffer = self.trBuffer

    leafCk   = getPlugByName(leafBuffer,self.topTree.bufferOut).getNet()
    deepPlug = self.topTree.addDeepPlug( leafCk, plugOccurrence.getPath() )
    if deepPlug:
      leafCk = deepPlug.getMasterNet()
    plugOccurrence.getEntity().setNet( leafCk )

    trace( 550, '\tLeaf clock set to <%s>.\n' % plugOccurrence.getEntity() )

    return

  def getLeafBufferUnder ( self, point ):
    if self.childs:
      if self.blArea().contains(point): return self.childs[0].getLeafBufferUnder(point)
      if self.brArea().contains(point): return self.childs[1].getLeafBufferUnder(point)
      if self.tlArea().contains(point): return self.childs[2].getLeafBufferUnder(point)
      if self.trArea().contains(point): return self.childs[3].getLeafBufferUnder(point)

    if self.blArea().contains(point): return self.blBuffer
    if self.brArea().contains(point): return self.brBuffer
    if self.tlArea().contains(point): return self.tlBuffer
    return self.trBuffer

  def place ( self ):
    trace( 550, '\rplace HTreeNode %s\n' % self.ckNet.getName() )
    x          = self.area.getXMin() + self.area.getWidth ()//4
    y          = self.area.getYMin() + self.area.getHeight()//4
    halfWidth  = self.area.getHalfWidth ()
    halfHeight = self.area.getHalfHeight()

    self.topTree.placeInstance( self.blBuffer, x          , y )
    self.topTree.placeInstance( self.brBuffer, x+halfWidth, y )
    self.topTree.placeInstance( self.tlBuffer, x          , y+halfHeight )
    self.topTree.placeInstance( self.trBuffer, x+halfWidth, y+halfHeight )

    self.topTree.usedVTracks += \
        [ self.topTree.conf.rpAccessByPlugName( self.blBuffer, self.topTree.bufferIn, self.ckNet ).getX()
        , self.topTree.conf.rpAccessByPlugName( self.brBuffer, self.topTree.bufferIn, self.ckNet ).getX() ]

    for child in self.childs: child.place()
    return

  def route ( self ):
    trace( 550, '\tHTreeNode.route() %s\n' % self.sourceBuffer.getName() )

    leftSourceContact  = self.topTree.conf.rpAccessByPlugName( self.sourceBuffer, self.topTree.bufferOut, self.ckNet , GaugeConf.HAccess|GaugeConf.OffsetBottom1 )
    rightSourceContact = self.topTree.conf.rpAccessByPlugName( self.sourceBuffer, self.topTree.bufferOut, self.ckNet , GaugeConf.HAccess|GaugeConf.OffsetBottom1 )
    blContact          = self.topTree.conf.rpAccessByPlugName( self.blBuffer    , self.topTree.bufferIn , self.ckNet )
    brContact          = self.topTree.conf.rpAccessByPlugName( self.brBuffer    , self.topTree.bufferIn , self.ckNet )
    tlContact          = self.topTree.conf.rpAccessByPlugName( self.tlBuffer    , self.topTree.bufferIn , self.ckNet )
    trContact          = self.topTree.conf.rpAccessByPlugName( self.trBuffer    , self.topTree.bufferIn , self.ckNet )
    leftContact        = self.topTree.conf.createContact( self.ckNet, blContact.getX(),  leftSourceContact.getY() )
    rightContact       = self.topTree.conf.createContact( self.ckNet, brContact.getX(), rightSourceContact.getY() )

    leftSourceX  = self.topTree.conf.getNearestVerticalTrack  ( self.topTree.area,  leftSourceContact.getX(), 0 )
    leftSourceY  = self.topTree.conf.getNearestHorizontalTrack( self.topTree.area,  leftSourceContact.getY(), 0 )
    rightSourceX = self.topTree.conf.getNearestVerticalTrack  ( self.topTree.area, rightSourceContact.getX(), 0 )
    rightSourceY = self.topTree.conf.getNearestHorizontalTrack( self.topTree.area, rightSourceContact.getY(), 0 )
    leftX        = self.topTree.conf.getNearestVerticalTrack  ( self.topTree.area,        leftContact.getX(), 0 )
    rightX       = self.topTree.conf.getNearestVerticalTrack  ( self.topTree.area,       rightContact.getX(), 0 )
    tlY          = self.topTree.conf.getNearestHorizontalTrack( self.topTree.area,          tlContact.getY(), 0 )
    blY          = self.topTree.conf.getNearestHorizontalTrack( self.topTree.area,          blContact.getY(), 0 )

    self.topTree.conf.setStackPosition(  leftSourceContact,  leftSourceX,  leftSourceY )
    self.topTree.conf.setStackPosition( rightSourceContact, rightSourceX, rightSourceY )
    self.topTree.conf.setStackPosition( tlContact,  leftX, tlY )
    self.topTree.conf.setStackPosition( blContact,  leftX, blY )
    self.topTree.conf.setStackPosition( trContact, rightX, tlY )
    self.topTree.conf.setStackPosition( brContact, rightX, blY )

    leftContact .setX(        leftX )
    leftContact .setY(  leftSourceY )
    rightContact.setX(       rightX )
    rightContact.setY( rightSourceY )

    self.topTree.conf.createHorizontal( leftContact       , leftSourceContact, leftSourceY , 0 )
    self.topTree.conf.createHorizontal( rightSourceContact, rightContact     , rightSourceY, 0 )
    self.topTree.conf.createVertical  ( leftContact       , blContact        , leftX       , 0 )
    self.topTree.conf.createVertical  ( tlContact         , leftContact      , leftX       , 0 )
    self.topTree.conf.createVertical  ( rightContact      , brContact        , rightX      , 0 )
    self.topTree.conf.createVertical  ( trContact         , rightContact     , rightX      , 0 )

    for child in self.childs: child.route()
    return

  def connectLeafs ( self ):
    if not self.hasLeafs():
      trace( 550, '\tHTreeNode.connectLeafs() %s has no leafs\n' % self.sourceBuffer.getName() )

    if self.childs:
      self.childs[0].connectLeafs()
      self.childs[1].connectLeafs()
      self.childs[2].connectLeafs()
      self.childs[3].connectLeafs()
      return

    if len(self.blLeafs): self.topTree._connectLeafs( self.blBuffer, self.blLeafs )
    if len(self.brLeafs): self.topTree._connectLeafs( self.brBuffer, self.brLeafs )
    if len(self.tlLeafs): self.topTree._connectLeafs( self.tlBuffer, self.tlLeafs )
    if len(self.trLeafs): self.topTree._connectLeafs( self.trBuffer, self.trLeafs )
    return

  def prune ( self ):
    for child in self.childs: child.prune()
    if self.hasLeafs(): return

    destroyNetComponents( self.ckNet )

    self.topTree.destroyInstance( self.blBuffer )
    self.topTree.destroyInstance( self.brBuffer )
    self.topTree.destroyInstance( self.tlBuffer )
    self.topTree.destroyInstance( self.trBuffer )
    return


def computeAbutmentBox ( cell, spaceMargin, aspectRatio, cellGauge ):
  #  sliceHeight = DbU.toLambda( cellGauge.getSliceHeight() ) 
  #
  #  instancesNb = 0
  #  cellLength  = 0
  #  for occurrence in cell.getTerminalNetlistInstanceOccurrences():
  #    instance     = occurrence.getEntity()
  #    instancesNb += 1
  #    cellLength  += int( DbU.toLambda(instance.getMasterCell().getAbutmentBox().getWidth()) )
  #
  # # ar = x//y    S = x*y = spaceMargin*SH    x=S//y    ar = S//y^2
  # # y = sqrt(S//AR)
  #  gcellLength = float(cellLength)*(1+spaceMargin) // sliceHeight
  #  rows = math.sqrt( gcellLength//aspectRatio )
  #  if math.trunc(rows) != rows: rows = math.trunc(rows) + 1
  #  else:                        rows = math.trunc(rows)
  #  columns = gcellLength // rows
  #  if math.trunc(columns) != columns: columns = math.trunc(columns) + 1
  #  else:                              columns = math.trunc(columns)
  #
  #  print('  o  Creating abutment box (margin:%.1f%%, aspect ratio:%.1f%%, g-length:%.1fl)' \
  #      % (spaceMargin*100.0,aspectRatio*100.0,(cellLength//sliceHeight)))
  #  print('     - GCell grid: [%dx%d]' % (columns,rows))

  UpdateSession.open()
  etesian = Etesian.EtesianEngine.create( cell )
  etesian.setDefaultAb()
  etesian.destroy()
  
  #abutmentBox =  Box( DbU.fromLambda(0)
  #                  , DbU.fromLambda(0)
  #                  , DbU.fromLambda(columns*sliceHeight)
  #                  , DbU.fromLambda(rows   *sliceHeight)
  #                  )
  #cell.setAbutmentBox( abutmentBox )
  UpdateSession.close()

  return cell.getAbutmentBox()
