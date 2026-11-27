#pragma once
#include "CoreMinimal.h"
#include "Input/Events.h"

class SWidget;
class FSlateWindowElementList;

// Controller support for the Teleport Logistics windows (Steam Deck and other gamepads).
// The game's navigation config does not reliably turn the D-pad into Slate navigation
// inside mod windows, so the windows route gamepad keys here instead:
// B closes, D-pad / left stick move focus spatially between buttons and text boxes,
// and A presses the focused button.
namespace TeleportLogisticsGamepad
{
enum class EResult
{
    Unhandled,
    Handled,
    Close,
};

// Call from NativeOnPreviewKeyDown. Root is the window's own Slate content.
EResult HandleKey(const TSharedPtr<SWidget> &Root, const FKeyEvent &Key);

// True for any gamepad key, so the window can show the focus outline only while a
// controller is in use.
bool IsGamepadKey(const FKeyEvent &Key);

// True while the built-in on-screen keyboard is open over this window. The window then
// sends it every key and blocks engine navigation, so nothing moves underneath.
bool KeyboardOpen(const TSharedPtr<SWidget> &Root);

// Draws an outline around the focused control. Call from NativePaint.
int32 PaintFocus(const TSharedPtr<SWidget> &Root, FSlateWindowElementList &OutDrawElements, int32 LayerId);
} // namespace TeleportLogisticsGamepad
