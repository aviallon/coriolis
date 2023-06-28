.. -*- Mode: rst -*-


8. Tool Engines (CRL Core)
==========================

The ToolEngine_ class is the base class for all tools developped in
|Coriolis|. In the rest of the tutorial we will use the names ``tool``
or ``engine`` as synonyms.


8.1 Placer -- Etesian
~~~~~~~~~~~~~~~~~~~~~

To run the placer, create the |Etesian| engine, then call the ``place()``
function.

.. code-block:: Python

   import Etesian

   # [...]

   etesian = Etesian.EtesianEngine.create(cell)
   etesian.place()

You can configure the placer in two ways:

#. Prior to the creation of the engine, setup an abutment for the cell.
   The placer will fit the cells into that area. If the area is too
   small, it will issue an error.

#. Setup |Etesian| parameters through the ``settings.py`` configuration
   file. For example:

   .. code-block:: Python
   
      with overlay.CfgCache(priority=Cfg.Parameter.Priority.UserFile) as cfg:
          cfg.etesian.effort           = 2
          cfg.etesian.spaceMargin      = 0.8
          cfg.etesian.densityVariation = 0.1
          cfg.etesian.aspectRatio      = 1.0


   With this setup, the placement will have ``80%`` of free space added and
   an aspect ratio of ``100%`` (square shape). Some variations in density
   is allowed, with at most ``10%`` unused space.


8.1 Router -- Katana
~~~~~~~~~~~~~~~~~~~~

Like for |Etesian|, you have to create the engine on the cell then call
the sequence of functions detailed below.

.. note:: **Kite vs. Katana.** There are currently two routers in |Coriolis|,
	  |Kite| is the old one and digital only. |Katana| is a re-implementation
	  with support for mixed routing (digital **and** analog).
	  Until |Katana| is fully implemented we keep both of them.

.. code-block:: Python

   import Anabatic
   import Katana

   # [...]

   katana = Katana.KatanaEngine.create(cell)
   katana.digitalInit      ()
   katana.runGlobalRouter  ()
   katana.loadGlobalRouting( Anabatic.EngineLoadGrByNet )
   katana.layerAssign      ( Anabatic.EngineNoNetLayerAssign )
   katana.runNegociate     ( Katana.Flags.NoFlags )


8.2  A Complete Example
~~~~~~~~~~~~~~~~~~~~~~~

The example file ``toolengines.py`` can be found in the ``share/doc/coriolis2/examples/scripts/``
directory (under the the root of the |Coriolis| installation).

This script automatically places and routes the ``fulladder`` netlist as seen
previously. The call to the ToolEngines_ is made inside the new function
``placeAndRoute()``.

.. note:: As the ``ToolEngine`` take care of opening and closing UpdateSession_, we
	  do not need the wrapper function ``doBreak()`` around the breakpoints.
          We directly call Breakpoint_.

.. note:: The space margin for this example is very high (``80%``), it's only
	  because it's too small for the placer to run correctly. For normal
	  case it is around ``3%``.

.. code-block:: Python

   #!/usr/bin/python
   
   import sys
   from   Hurricane import *
   from   CRL       import *
   import Etesian
   import Anabatic
   import Katana
   
   # Everybody needs it.
   af = AllianceFramework.get()
   
   def buildFulladder ( editor ):
       # Get the Framework and all the master cells.
       xr2_x2 = af.getCell( 'xr2_x1', Catalog.State.Views )
       a2_x2  = af.getCell( 'a2_x2' , Catalog.State.Views )
       o2_x2  = af.getCell( 'o2_x2' , Catalog.State.Views )
   
       with UpdateSession():
           fulladder = af.createCell( 'fulladder' )
           
           # Create Instances.
           a2_1  = Instance.create( fulladder,  'a2_1',  a2_x2 )
           a2_2  = Instance.create( fulladder,  'a2_2',  a2_x2 ) 
           xr2_1 = Instance.create( fulladder, 'xr2_1', xr2_x2 )
           xr2_2 = Instance.create( fulladder, 'xr2_2', xr2_x2 )
           o2_1  = Instance.create( fulladder,  'o2_1',  o2_x2 )
           
           # Create Nets.
           vss = Net.create( fulladder, "vss" )
           vss.setExternal( True )
           vss.setGlobal  ( True )
           
           vdd = Net.create( fulladder, "vdd" )
           vdd.setExternal( True )
           vdd.setGlobal  ( True )
           
           cin = Net.create( fulladder, "cin" )
           cin.setExternal( True )
           xr2_2.getPlug( xr2_x2.getNet('i0') ).setNet( cin )
           a2_2 .getPlug(  a2_x2.getNet('i0') ).setNet( cin )
           
           a = Net.create( fulladder, 'a' )
           a.setExternal( True )
           xr2_1.getPlug( xr2_x2.getNet('i0') ).setNet( a )
           a2_1 .getPlug(  a2_x2.getNet('i0') ).setNet( a )
           
           b = Net.create( fulladder, 'b' )
           b.setExternal( True )
           xr2_1.getPlug( xr2_x2.getNet('i1') ).setNet( b )
           a2_1 .getPlug(  a2_x2.getNet('i1') ).setNet( b )
           
           sout_1 = Net.create( fulladder, 'sout_1' )
           xr2_1.getPlug( xr2_x2.getNet('q' ) ).setNet( sout_1 )
           xr2_2.getPlug( xr2_x2.getNet('i1') ).setNet( sout_1 )
           a2_2 .getPlug(  a2_x2.getNet('i1') ).setNet( sout_1 )
           
           carry_1 = Net.create( fulladder, 'carry_1' )
           a2_1.getPlug( a2_x2.getNet('q' ) ).setNet( carry_1 )
           o2_1.getPlug( o2_x2.getNet('i1') ).setNet( carry_1 )
           
           carry_2 = Net.create( fulladder, 'carry_2' )
           a2_2.getPlug( a2_x2.getNet('q' ) ).setNet( carry_2 )
           o2_1.getPlug( o2_x2.getNet('i0') ).setNet( carry_2 )
           
           sout = Net.create( fulladder, 'sout' )
           sout.setExternal( True )
           xr2_2.getPlug( xr2_x2.getNet('q') ).setNet( sout )
           
           cout = Net.create( fulladder, 'cout' )
           cout.setExternal( True )
           o2_1.getPlug( o2_x2.getNet('q') ).setNet( cout )
   
       af.saveCell( fulladder, Catalog.State.Views )
       return fulladder
   
   
   def placeAndRoute ( editor, cell ):
       # Run the placer.
       etesian = Etesian.EtesianEngine.create(cell)
       etesian.place()
       if editor:
           editor.setCell( cell )
           editor.fit()
   
       Breakpoint.stop( 1, 'After placement' )
   
       # Run the router.
       katana = Katana.KatanaEngine.create(cell)
       katana.digitalInit      ()
       katana.runGlobalRouter  ()
       katana.loadGlobalRouting( Anabatic.EngineLoadGrByNet )
       katana.layerAssign      ( Anabatic.EngineNoNetLayerAssign )
       katana.runNegociate     ( Katana.Flags.NoFlags )
   
       af.saveCell( cell, Catalog.State.Views )
   
   
   def scriptMain ( **kw ):
       editor = None
       if 'editor' in kw and kw['editor']:
           editor = kw['editor']
       fulladder = buildFulladder( editor )
       placeAndRoute( editor, fulladder )
       return True 
