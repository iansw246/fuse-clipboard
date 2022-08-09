#pragma once
/*
 * Interface for clipboard data. In progress
* /
#include <string>
enum class ClipboardMode
{
    // Ctrl+c/v clipboard
    Clipboard,
    // Mouse selection clipboard. Pasted via middle click
    Selection,
};

class ClipboardInfo
{
    virtual std::string text(ClipboardMode mode = ClipboardMode::Clipboard) = 0;

};
