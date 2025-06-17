#pragma once

// Modes for cursor appearance and behavior
enum class CursorMode 
{
    Normal,    // visible arrow, free movement
    Hidden,    // invisible, free movement
    Disabled,  // invisible, locked to window
    Arrow,     // visible arrow
    IBeam,     // text select I-beam
    Crosshair, // precision crosshair
    Hand,      // pointing hand
    HResize,   // horizontal resize
    VResize,   // vertical resize
    NoDrop     // circle-slash “not permitted”
};

class Cursor 
{
public:
    static void Initialize(void* glfwWindow);
    // Switch to the specified mode
    static void SetMode(CursorMode mode);
};