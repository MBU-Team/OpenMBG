//-----------------------------------------------------------------------------
// Torque Game Engine
// 
// Copyright (c) 2001 GarageGames.Com
// Portions Copyright (c) 2001 by Sierra Online, Inc.
//-----------------------------------------------------------------------------

// Normally the trigger class would be sub-classed to support different
// functionality, but we're going to use the name of the trigger instead
// as an experiment.

//-----------------------------------------------------------------------------

datablock TriggerData(InBoundsTrigger)
{
   tickPeriodMS = 100;
};

function InBoundsTrigger::onLeaveTrigger(%this,%trigger,%obj)
{
   // Leaving an in bounds area.
   %obj.getDatablock().onOutOfBounds(%obj);
}


//-----------------------------------------------------------------------------

datablock TriggerData(OutOfBoundsTrigger)
{
   tickPeriodMS = 100;
};

function OutOfBoundsTrigger::onEnterTrigger(%this,%trigger,%obj)
{
   // Entering an out of bounds area
   %obj.getDatablock().onOutOfBounds(%obj);
}

//-----------------------------------------------------------------------------

datablock TriggerData(HelpTrigger)
{
   tickPeriodMS = 100;
};

function HelpTrigger::onEnterTrigger(%this,%trigger,%obj)
{
   // Leaving an in bounds area.
   addHelpLine(%trigger.text, true);
}

