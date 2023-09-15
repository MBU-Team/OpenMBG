//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GUICONTROL_H_
#define _GUICONTROL_H_

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _MPOINT_H_
#include "math/mPoint.h"
#endif
#ifndef _MRECT_H_
#include "math/mRect.h"
#endif
#ifndef _COLOR_H_
#include "core/color.h"
#endif
#ifndef _SIMBASE_H_
#include "console/simBase.h"
#endif
#ifndef _GUITYPES_H_
#include "gui/guiTypes.h"
#endif
#ifndef _EVENT_H_
#include "platform/event.h"
#endif

class GuiCanvas;

/// Root class for all GUI controls in Torque.
///
/// @see GUI for an overview of the Torque GUI system.
///
/// @section GuiControl_Intro Introduction
///
/// GuiControl is the base class for GUI controls in Torque. It provides these
/// basic areas of functionality:
///      - Inherits from SimGroup, so that controls can have children.
///      - Interfacing with a GuiControlProfile.
///      - An abstraction from the details of handling user input
///        and so forth, providing friendly hooks like onMouseEnter(), onMouseMove(),
///        and onMouseLeave(), onKeyDown(), and so forth.
///      - An abstraction from the details of rendering and resizing.
///      - Helper functions to manipulate the mouse (mouseLock and
///        mouseUnlock), and convert coordinates (localToGlobalCoord() and
///        globalToLocalCoord()).
///
/// @ref GUI has an overview of the GUI system.
///
/// @nosubgrouping
class GuiControl : public SimGroup
{
private:
   typedef SimGroup Parent;

public:

    /// @name Control State
    /// @{

    GuiControlProfile *mProfile;

    bool    mVisible;
    bool    mActive;
    bool    mAwake;
    bool    mSetFirstResponder;

    S32     mLayer;
    RectI   mBounds;
    Point2I mMinExtent;

    /// @}

    /// @name Keyboard Input
    /// @{
    GuiControl *mFirstResponder;   
    static GuiControl *gPrevResponder;
    static GuiControl *gCurResponder;
    /// @}

    enum horizSizingOptions
    {
        horizResizeRight = 0,   ///< fixed on the left and width
        horizResizeWidth,       ///< fixed on the left and right
        horizResizeLeft,        ///< fixed on the right and width
        horizResizeCenter,
        horizResizeRelative     ///< resize relative
    };
    enum vertSizingOptions
    {
        vertResizeBottom = 0,   ///< fixed on the top and in height
        vertResizeHeight,       ///< fixed on the top and bottom
        vertResizeTop,          ///< fixed in height and on the bottom
        vertResizeCenter,
        vertResizeRelative      ///< resize relative
    };

protected:
    /// @name Control State
    /// @{

    S32 mHorizSizing;      ///< Set from horizSizingOptions.
    S32 mVertSizing;       ///< Set from vertSizingOptions.
    
    StringTableEntry mConsoleVariable;
    StringTableEntry mConsoleCommand;
    StringTableEntry mAltConsoleCommand;

    StringTableEntry mAcceleratorKey;

    /// @}

    /// @name Console
    /// The console variable collection of functions allows a console variable to be bound to the GUI control.
    ///
    /// This allows, say, an edit field to be bound to '$foo'. The value of the console
    /// variable '$foo' would then be equal to the text inside the text field. Changing
    /// either changes the other.
    /// @{
       
    /// Sets the value of the console variable bound to this control
    /// @param   value   String value to assign to control's console variable
    void setVariable(const char *value);
    
    /// Sets the value of the console variable bound to this control
    /// @param   value   Integer value to assign to control's console variable
    void setIntVariable(S32 value);
    
    /// Sets the value of the console variable bound to this control
    /// @param   value   Float value to assign to control's console variable
    void setFloatVariable(F32 value);
    
    const char* getVariable(); ///< Returns value of control's bound variable as a string
    S32 getIntVariable();      ///< Returns value of control's bound variable as a integer
    F32 getFloatVariable();    ///< Returns value of control's bound variable as a float

public:
    /// Set the name of the console variable which this GuiObject is bound to
    /// @param   variable   Variable name
    void setConsoleVariable(const char *variable);
    
    /// Set the name of the console function bound to, such as a script function
    /// a button calls when clicked.
    /// @param   newCmd   Console function to attach to this GuiControl
    void setConsoleCommand(const char *newCmd);
    const char * getConsoleCommand(); ///< Returns the name of the function bound to this GuiControl

    /// @}

    /// @name Editor
    /// These functions are used by the GUI Editor
    /// @{

    /// Sets the size of the GuiControl
    /// @param   horz   Width of the control
    /// @param   vert   Height of the control
    void setSizing(S32 horz, S32 vert);

    /// @}
public:
    /// @name Initialization
    /// @{

    DECLARE_CONOBJECT(GuiControl);
    GuiControl();
    virtual ~GuiControl();
    bool onAdd();
    static void initPersistFields();
    /// @}

    /// @name Accessors
    /// @{

    const Point2I& getPosition() { return mBounds.point; } ///< Returns position of the control
    const Point2I& getExtent() { return mBounds.extent; } ///< Returns extents of the control
    const Point2I& getMinExtent() { return mMinExtent; } ///< Returns minimum size the control can be
    /// @}

    /// @name Flags
    /// @{
       
    /// Sets the visibility of the control
    /// @param   value   True if object should be visible
    virtual void setVisible(bool value);
    inline bool isVisible() { return mVisible; } ///< Returns true if the object is visible

    /// Sets the status of this control as active and responding or inactive
    /// @param   value   True if this is active
    void setActive(bool value);
    bool isActive() { return mActive; } ///< Returns true if this control is active

    bool isAwake() { return mAwake; } ///< Returns true if this control is awake

    /// @}

    /// Get information about the size of a scroll line.
    ///
    /// @param   rowHeight   The height, in pixels, of a row
    /// @param   columnWidth The width, in pixels, of a column
    virtual void getScrollLineSizes(U32 *rowHeight, U32 *columnWidth);

    /// Get information about the cursor.
    /// @param   cursor   Cursor information will be stored here
    /// @param   showCursor Will be set to true if the cursor is visible
    /// @param   lastGuiEvent GuiEvent containing cursor position and modifyer keys (ie ctrl, shift, alt etc)
    virtual void getCursor(GuiCursor *&cursor, bool &showCursor, const GuiEvent &lastGuiEvent);

    /// @name Children
    /// @{
       
    /// Adds an object as a child of this object.
    /// @param   obj   New child object of this control
    void addObject(SimObject *obj);
    
    /// Removes a child object from this control.
    /// @param   obj Object to remove from this control
    void removeObject(SimObject *obj);
    
    GuiControl *getParent();  ///< Returns the control which owns this one.
    GuiCanvas *getRoot();     ///< Returns the root canvas of this control.
    /// @}

    /// @name Coordinates
    /// @{
    
    /// Translates local coordinates (wrt this object) into global coordinates
    ///
    /// @param   src   Local coordinates to translate
    Point2I localToGlobalCoord(const Point2I &src);
    
    /// Returns global coordinates translated into local space
    ///
    /// @param   src   Global coordinates to translate
    Point2I globalToLocalCoord(const Point2I &src);
    /// @}

    /// @name Resizing
    /// @{
    
    /// Changes the size and/or position of this control
    /// @param   newPosition   New position of this control
    /// @param   newExtent   New size of this control
    virtual void resize(const Point2I &newPosition, const Point2I &newExtent);
    
    /// Called when a child control of the object is resized
    /// @param   child   Child object
    virtual void childResized(GuiControl *child);
    
    /// Called when this objects parent is resized
    /// @param   oldParentExtent   The old size of the parent object
    /// @param   newParentExtent   The new size of the parent object
    virtual void parentResized(const Point2I &oldParentExtent, const Point2I &newParentExtent);
    /// @}

    /// @name Rendering
    /// @{
       
    /// Called when this control is to render itself
    /// @param   offset   The location this control is to begin rendering
    /// @param   updateRect   The screen area this control has drawing access to
    virtual void onRender(Point2I offset, const RectI &updateRect);
    
    /// Called when this control should render its children
    /// @param   offset   The location this control is to begin rendering
    /// @param   updateRect   The screen area this control has drawing access to  
    void renderChildControls(Point2I offset, const RectI &updateRect);
    
    /// Sets the area (local coordinates) this control wants refreshed each frame
    /// @param   pos   UpperLeft point on rectangle of refresh area
    /// @param   ext   Extent of update rect
    void setUpdateRegion(Point2I pos, Point2I ext);
    
    /// Sets the update area of the control to encompass the whole control
    void setUpdate();
    /// @}

    //child hierarchy calls
    void awaken();          ///< Called when this control and its children have been wired up.
    void sleep();           ///< Called when this control is no more.
    void preRender();       ///< Prerender this control and all its children.

    /// @name Events
    ///
    /// If you subclass these, make sure to call the Parent::'s versions.
    ///
    /// @{
       
    /// Called when this object is asked to wake up returns true if it's actually awake at the end
    virtual bool onWake();
    
    /// Called when this object is asked to sleep
    virtual void onSleep();
    
    /// Do special pre-render proecessing
    virtual void onPreRender();
    
    /// Called when this object is removed
    virtual void onRemove();

    /// @}

    /// @name Console
    /// @{
       
    /// Returns the value of the variable bound to this object
    virtual const char *getScriptValue();
    
    /// Sets the value of the variable bound to this object
    virtual void setScriptValue(const char *value);
    /// @}

    /// @name Input (Keyboard/Mouse)
    /// @{
       
    /// This function will return true if the provided coordinates (wrt parent object) are
    /// within the bounds of this control
    /// @param   parentCoordPoint   Coordinates to test
    virtual bool pointInControl(const Point2I& parentCoordPoint);
    
    /// Returns true if the global cursor is inside this control
    bool cursorInControl();
    
    /// Returns the control which the provided point is under, with layering
    /// @param   pt   Point to test
    /// @param   initialLayer  Layer of gui objects to begin the search
    virtual GuiControl* findHitControl(const Point2I &pt, S32 initialLayer = -1);

    /// Lock the mouse within the provided control
    /// @param   lockingControl   Control to lock the mouse within
    void mouseLock(GuiControl *lockingControl);
    
    /// Turn on mouse locking with last used lock control
    void mouseLock();
    
    /// Unlock the mouse
    void mouseUnlock();
    
    /// Returns true if the mouse is locked
    bool isMouseLocked();
    /// @}


    /// General input handler.
    virtual bool onInputEvent(const InputEvent &event);

    /// @name Mouse Events
    /// These functions are called when the input event which is
    /// in the name of the function occurs.
    /// @{
    virtual void onMouseUp(const GuiEvent &event);
    virtual void onMouseDown(const GuiEvent &event);
    virtual void onMouseMove(const GuiEvent &event);
    virtual void onMouseDragged(const GuiEvent &event);
    virtual void onMouseEnter(const GuiEvent &event);
    virtual void onMouseLeave(const GuiEvent &event);

    virtual bool onMouseWheelUp(const GuiEvent &event);
    virtual bool onMouseWheelDown(const GuiEvent &event);

    virtual void onRightMouseDown(const GuiEvent &event);
    virtual void onRightMouseUp(const GuiEvent &event);
    virtual void onRightMouseDragged(const GuiEvent &event);
    /// @}

    /// @name Tabs
    /// @{

    /// Find the first tab-accessable child of this control
    virtual GuiControl* findFirstTabable();
    
    /// Find the last tab-accessable child of this control
    /// @param   firstCall   Set to true to clear the global previous responder
    virtual GuiControl* findLastTabable(bool firstCall = true);
    
    /// Find previous tab-accessable control with respect to the provided one
    /// @param   curResponder   Current control
    /// @param   firstCall   Set to true to clear the global previous responder
    virtual GuiControl* findPrevTabable(GuiControl *curResponder, bool firstCall = true);
    
    /// Find next tab-accessable control with regards to the provided control.
    ///
    /// @param   curResponder   Current control
    /// @param   firstCall   Set to true to clear the global current responder
    virtual GuiControl* findNextTabable(GuiControl *curResponder, bool firstCall = true);
    /// @}

    /// Returns true if the provided control is a child (grandchild, or greatgrandchild) of this one.
    ///
    /// @param   child   Control to test
    virtual bool ControlIsChild(GuiControl *child);

    /// @name First Responder
    /// A first responder is the control which reacts first, in it's responder chain, to keyboard events
    /// The responder chain is set for each parent and so there is only one first responder amongst it's
    /// children.
    /// @{
    
    /// Sets the first responder for child controls
    /// @param   firstResponder   First responder for this chain
    virtual void setFirstResponder(GuiControl *firstResponder);
    
    /// Sets up this control to be the first in it's group to respond to an input event
    /// @param   value   True if this should be a first responder
    virtual void makeFirstResponder(bool value); 
    
    /// Returns true if this control is a first responder
    bool isFirstResponder();
    
    /// Sets this object to be a first responder
    void setFirstResponder();
    
    /// Clears the first responder for this chain
    void clearFirstResponder();
    
    /// Returns the first responder for this chain
    GuiControl *getFirstResponder() { return mFirstResponder; }
    
    /// Occurs when the first responder for this chain is lost
    virtual void onLoseFirstResponder();
    /// @}

    /// @name Keyboard Events
    /// @{
    
    /// Adds the accelerator key for this object to the canvas
    void addAcceleratorKey();
    
    /// Adds this control's accelerator key to the accelerator map, and
    /// recursively tells all children to do the same.
    virtual void buildAcceleratorMap();
    
    /// Occurs when the accelerator key for this control is pressed
    ///
    /// @param   index   Index in the acclerator map of the key
    virtual void acceleratorKeyPress(U32 index);
    
    /// Occurs when the accelerator key for this control is released
    ///
    /// @param   index   Index in the acclerator map of the key
    virtual void acceleratorKeyRelease(U32 index);
    
    /// Happens when a key is depressed
    /// @param   event   Event descriptor (which contains the key)
    virtual bool onKeyDown(const GuiEvent &event);
    
    /// Happens when a key is released
    /// @param   event   Event descriptor (which contains the key)
    virtual bool onKeyUp(const GuiEvent &event);
    
    /// Happens when the same key that was pressed last press is pressed again
    /// @param   event   Event descriptor (which contains the key)
    virtual bool onKeyRepeat(const GuiEvent &event);
    /// @}

    /// Sets the control profile for this control.
    ///
    /// @see GuiControlProfile
    /// @param   prof   Control profile to apply
    void setControlProfile(GuiControlProfile *prof);

    /// Occurs when this control performs its "action"
    virtual void onAction();

    /// @name Peer Messaging
    /// Used to send a message to other GUIControls which are children of the same parent.
    ///
    /// This is mostly used by radio controls.
    /// @{
    void messageSiblings(S32 message);                      ///< Send a message to all siblings
    virtual void onMessage(GuiControl *sender, S32 msg);    ///< Receive a message from another control
    /// @}

    /// @name Canvas Events
    /// Functions called by the canvas
    /// @{
    
    /// Called if this object is a dialog, when it is added to the visible layers
    virtual void onDialogPush();
    
    /// Called if this object is a dialog, when it is removed from the visible layers
    virtual void onDialogPop();
    /// @}

    /// Renders justified text using the profile.
    ///
    /// @note This should move into the graphics library at some point
    void renderJustifiedText(Point2I offset, Point2I extent, const char *text);

    void inspectPostApply();
    void inspectPreApply();
};

#endif
