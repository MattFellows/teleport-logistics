#include "TeleportLogisticsGamepad.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Framework/Application/SlateApplication.h"
#include "InputCoreTypes.h"
#include "Layout/Children.h"
#include "Rendering/DrawElements.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SWidget.h"
#include "Styling/CoreStyle.h"
#include "Fonts/FontMeasure.h"
#include "Rendering/SlateRenderer.h"
#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <Windows.h>
#include "Windows/HideWindowsPlatformTypes.h"
#endif
#include "TeleportLogisticsLog.h"

namespace
{
// SButton::SimulateClick is compiled out of shipping builds. A pointer to the protected
// ExecuteOnClick, named through a derived class, runs the same OnClicked handler.
struct FButtonAccess : SButton
{
    static FReply Click(SButton &Button)
    {
        return (Button.*(&FButtonAccess::ExecuteOnClick))();
    }
};
const FName ButtonType(TEXT("SButton"));
const FName TextBoxType(TEXT("SEditableTextBox"));
const FName ScrollBoxType(TEXT("SScrollBox"));

bool Navigable(const TSharedRef<SWidget> &Widget)
{
    const FName Type = Widget->GetType();
    return (Type == ButtonType || Type == TextBoxType) && Widget->IsEnabled() &&
           Widget->GetPaintSpaceGeometry().GetLocalSize().GetMin() > 1.f;
}

void Collect(const TSharedRef<SWidget> &Widget, TArray<TSharedRef<SWidget>> &Out)
{
    if (!Widget->GetVisibility().IsVisible() || !Widget->IsEnabled())
        return;
    if (Navigable(Widget))
    {
        // A button's label and a text box's inner editor are not separate stops.
        Out.Add(Widget);
        return;
    }
    if (FChildren *Children = Widget->GetChildren())
        for (int32 I = 0; I < Children->Num(); ++I)
            Collect(Children->GetChildAt(I), Out);
}

// The navigable control that holds user focus: the focused widget itself or the
// candidate it sits inside (a text box's inner SEditableText, for example).
TSharedPtr<SWidget> Current(const TArray<TSharedRef<SWidget>> &Candidates, uint32 UserIndex)
{
    for (TSharedPtr<SWidget> W = FSlateApplication::Get().GetUserFocusedWidget(UserIndex); W;
         W = W->GetParentWidget())
        for (const auto &C : Candidates)
            if (C == W)
                return C;
    return nullptr;
}

FVector2D Center(const TSharedRef<SWidget> &Widget)
{
    const FGeometry &G = Widget->GetPaintSpaceGeometry();
    return G.LocalToAbsolute(G.GetLocalSize() * 0.5f);
}

TSharedPtr<SWidget> Nearest(const TArray<TSharedRef<SWidget>> &Candidates, const TSharedRef<SWidget> &From,
                            EUINavigation Direction)
{
    const FVector2D Origin = Center(From);
    TSharedPtr<SWidget> Best;
    double BestScore = TNumericLimits<double>::Max();
    for (const auto &C : Candidates)
    {
        if (C == From)
            continue;
        const FVector2D D = Center(C) - Origin;
        double Along = 0, Across = 0;
        switch (Direction)
        {
        case EUINavigation::Up:
            Along = -D.Y, Across = FMath::Abs(D.X);
            break;
        case EUINavigation::Down:
            Along = D.Y, Across = FMath::Abs(D.X);
            break;
        case EUINavigation::Left:
            Along = -D.X, Across = FMath::Abs(D.Y);
            break;
        case EUINavigation::Right:
            Along = D.X, Across = FMath::Abs(D.Y);
            break;
        default:
            return nullptr;
        }
        // Must lie in the pressed direction; prefer controls in line with the current one.
        if (Along < 4.0)
            continue;
        const double Score = Along + Across * 2.0;
        if (Score < BestScore)
            BestScore = Score, Best = C;
    }
    return Best;
}

TSharedPtr<SWidget> First(const TArray<TSharedRef<SWidget>> &Candidates)
{
    TSharedPtr<SWidget> Best;
    FVector2D BestPos(TNumericLimits<double>::Max());
    for (const auto &C : Candidates)
    {
        const FVector2D P = Center(C);
        if (P.Y < BestPos.Y - 4.0 || (FMath::Abs(P.Y - BestPos.Y) <= 4.0 && P.X < BestPos.X))
            BestPos = P, Best = C;
    }
    return Best;
}

void Focus(const TSharedPtr<SWidget> &Target, uint32 UserIndex)
{
    // SetDirectly, not Navigation: the game swaps the engine's navigation focus ring for a
    // solid white box that hides the control's text. PaintFocus draws our own outline.
    FSlateApplication::Get().SetUserFocus(UserIndex, Target, EFocusCause::SetDirectly);
    for (TSharedPtr<SWidget> P = Target->GetParentWidget(); P; P = P->GetParentWidget())
        if (P->GetType() == ScrollBoxType)
            StaticCastSharedPtr<SScrollBox>(P)->ScrollDescendantIntoView(Target, true,
                                                                         EDescendantScrollDestination::IntoView, 8);
}

TSharedPtr<SWidget> FindType(const TSharedRef<SWidget> &Widget, FName Type)
{
    if (Widget->GetType() == Type)
        return Widget;
    if (FChildren *Children = Widget->GetChildren())
        for (int32 I = 0; I < Children->Num(); ++I)
            if (TSharedPtr<SWidget> Found = FindType(Children->GetChildAt(I), Type))
                return Found;
    return nullptr;
}

// Asks Steam for its floating on-screen keyboard (Steam Deck, Gaming Mode) just below the
// field. It types into whatever has focus, so the text box must be focused first. Goes
// through the Steamworks flat API in the steam_api64.dll the game has already loaded;
// Epic builds and older Steam clients get a logged reason, and Steam + X still works.
const TCHAR *ShowSteamKeyboard(const FGeometry &Field)
{
#if PLATFORM_WINDOWS
    using FSteamUtils = void *(*)();
    using FShowFloating = bool (*)(void *, int32, int32, int32, int32, int32);
    HMODULE Dll = ::GetModuleHandleW(L"steam_api64.dll");
    if (!Dll)
        return TEXT("steam_api64.dll is not loaded");
    const auto Utils = reinterpret_cast<FSteamUtils>(::GetProcAddress(Dll, "SteamAPI_SteamUtils_v010"));
    const auto Show =
        reinterpret_cast<FShowFloating>(::GetProcAddress(Dll, "SteamAPI_ISteamUtils_ShowFloatingGamepadTextInput"));
    if (!Utils || !Show)
        return TEXT("steam_api64.dll has no floating keyboard export");
    void *Self = Utils();
    if (!Self)
        return TEXT("SteamUtils v010 is unavailable");
    const FVector2D TopLeft = Field.LocalToAbsolute(FVector2D::ZeroVector);
    const FVector2D Size = Field.GetAbsoluteSize();
    // 0 = k_EFloatingGamepadTextInputModeModeSingleLine. The rectangle is the area the
    // keyboard must not cover.
    return Show(Self, 0, FMath::RoundToInt(TopLeft.X), FMath::RoundToInt(TopLeft.Y), FMath::RoundToInt(Size.X),
                FMath::RoundToInt(Size.Y))
               ? nullptr
               : TEXT("Steam declined (Gaming Mode only)");
#else
    return TEXT("not a Windows build");
#endif
}

// Built-in on-screen keyboard, used where Steam's floating keyboard is unavailable
// (Desktop Mode, Epic). It sends real key events to the focused text field.
struct FKeyboard
{
    TWeakPtr<SWidget> Root;
    int32 Row = 1, Col = 0;
    bool Shift = false;
};
FKeyboard Keyboard;
const TCHAR *const Rows[] = {TEXT("1234567890"), TEXT("qwertyuiop"), TEXT("asdfghjkl'"), TEXT("zxcvbnm-_.")};
constexpr int32 CharRows = UE_ARRAY_COUNT(Rows);
const TCHAR *const Specials[] = {TEXT("Shift"), TEXT("Space"), TEXT("Delete"), TEXT("Done")};
constexpr int32 SpecialCount = UE_ARRAY_COUNT(Specials);

bool KeyboardOpenFor(const TSharedPtr<SWidget> &Root)
{
    return Root && Keyboard.Root.IsValid() && Keyboard.Root.Pin() == Root;
}

void TypeChar(TCHAR C, uint32 User)
{
    FSlateApplication::Get().ProcessKeyCharEvent(FCharacterEvent(C, FModifierKeysState(), User, false));
}

void TypeKey(const FKey &Key, uint32 User)
{
    const FKeyEvent Event(Key, FModifierKeysState(), User, false, 0, 0);
    FSlateApplication::Get().ProcessKeyDownEvent(Event);
    FSlateApplication::Get().ProcessKeyUpEvent(Event);
}

int32 RowLength(int32 Row)
{
    return Row < CharRows ? FCString::Strlen(Rows[Row]) : SpecialCount;
}

void KeyboardPress(uint32 User)
{
    if (Keyboard.Row < CharRows)
    {
        const TCHAR C = Rows[Keyboard.Row][Keyboard.Col];
        TypeChar(Keyboard.Shift ? FChar::ToUpper(C) : C, User);
        Keyboard.Shift = false;
        return;
    }
    switch (Keyboard.Col)
    {
    case 0:
        Keyboard.Shift = !Keyboard.Shift;
        break;
    case 1:
        TypeChar(TEXT(' '), User);
        break;
    case 2:
        TypeKey(EKeys::BackSpace, User);
        break;
    default:
        Keyboard.Root.Reset();
        break;
    }
}

// Returns true when the key belonged to the open keyboard.
bool KeyboardKey(const FKey &K, uint32 User)
{
    const int32 OldLen = RowLength(Keyboard.Row);
    auto MoveRow = [&](int32 Delta) {
        Keyboard.Row = FMath::Clamp(Keyboard.Row + Delta, 0, CharRows);
        // Keep roughly the same horizontal position between the 10-key and 4-key rows.
        Keyboard.Col = FMath::Clamp(Keyboard.Col * RowLength(Keyboard.Row) / OldLen, 0, RowLength(Keyboard.Row) - 1);
    };
    if (K == EKeys::Gamepad_DPad_Up || K == EKeys::Gamepad_LeftStick_Up)
        MoveRow(-1);
    else if (K == EKeys::Gamepad_DPad_Down || K == EKeys::Gamepad_LeftStick_Down)
        MoveRow(1);
    else if (K == EKeys::Gamepad_DPad_Left || K == EKeys::Gamepad_LeftStick_Left)
        Keyboard.Col = (Keyboard.Col + OldLen - 1) % OldLen;
    else if (K == EKeys::Gamepad_DPad_Right || K == EKeys::Gamepad_LeftStick_Right)
        Keyboard.Col = (Keyboard.Col + 1) % OldLen;
    else if (K == EKeys::Gamepad_FaceButton_Bottom)
        KeyboardPress(User);
    else if (K == EKeys::Gamepad_FaceButton_Left)
        TypeKey(EKeys::BackSpace, User);
    else if (K == EKeys::Gamepad_FaceButton_Top)
        TypeChar(TEXT(' '), User);
    else if (K == EKeys::Gamepad_FaceButton_Right || K == EKeys::Gamepad_Special_Right)
        Keyboard.Root.Reset();
    else
        return false;
    return true;
}

int32 PaintKeyboard(const TSharedPtr<SWidget> &Root, FSlateWindowElementList &Out, int32 Layer)
{
    const FGeometry &G = Root->GetPaintSpaceGeometry();
    const FVector2D Area = G.GetLocalSize();
    const float Width = FMath::Min(820.f, float(Area.X) - 40.f);
    const float Key = Width / 10.f, Height = Key * 0.8f, Gap = 4.f;
    const float Total = (CharRows + 1) * (Height + Gap) + 40.f;
    const FVector2D Origin((Area.X - Width) * 0.5f, Area.Y - Total - 20.f);
    const FSlateBrush *White = FCoreStyle::Get().GetBrush(TEXT("GenericWhiteBox"));
    const FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 18);
    const FSlateFontInfo Small = FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 12);
    const TSharedRef<FSlateFontMeasure> Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
    auto Box = [&](FVector2D Pos, FVector2D Size, FLinearColor Color, int32 L) {
        FSlateDrawElement::MakeBox(Out, L, G.ToPaintGeometry(FVector2f(Size), FSlateLayoutTransform(FVector2f(Pos))),
                                   White, ESlateDrawEffect::None, Color);
    };
    auto Label = [&](FVector2D Pos, FVector2D Size, const FString &Text, const FSlateFontInfo &F, int32 L) {
        const FVector2D TextSize = Measure->Measure(Text, F);
        FSlateDrawElement::MakeText(
            Out, L,
            G.ToPaintGeometry(FVector2f(TextSize), FSlateLayoutTransform(FVector2f(Pos + (Size - TextSize) * 0.5f))),
            Text, F, ESlateDrawEffect::None, FLinearColor::White);
    };
    Box(Origin - FVector2D(12, 12), FVector2D(Width + 24, Total + 4), FLinearColor(0.02f, 0.02f, 0.02f, 0.96f),
        Layer + 1);
    for (int32 Row = 0; Row <= CharRows; ++Row)
    {
        const int32 Len = RowLength(Row);
        const float W = Width / Len;
        for (int32 Col = 0; Col < Len; ++Col)
        {
            const FVector2D Pos = Origin + FVector2D(Col * W, Row * (Height + Gap));
            const FVector2D Size(W - Gap, Height);
            const bool Selected = Row == Keyboard.Row && Col == Keyboard.Col;
            const bool ShiftOn = Row == CharRows && Col == 0 && Keyboard.Shift;
            Box(Pos, Size,
                Selected  ? FLinearColor(0.98f, 0.58f, 0.13f)
                : ShiftOn ? FLinearColor(0.35f, 0.25f, 0.1f)
                          : FLinearColor(0.2f, 0.2f, 0.2f),
                Layer + 2);
            const FString Text =
                Row < CharRows ? FString::Chr(Keyboard.Shift ? FChar::ToUpper(Rows[Row][Col]) : Rows[Row][Col])
                               : FString(Specials[Col]);
            Label(Pos, Size, Text, Font, Layer + 3);
        }
    }
    Label(Origin + FVector2D(0, (CharRows + 1) * (Height + Gap)), FVector2D(Width, 24),
          TEXT("A type    X delete    Y space    B or Menu: done"), Small, Layer + 3);
    return Layer + 3;
}

bool Direction(const FKey &Key, EUINavigation &Out)
{
    if (Key == EKeys::Gamepad_DPad_Up || Key == EKeys::Gamepad_LeftStick_Up)
        Out = EUINavigation::Up;
    else if (Key == EKeys::Gamepad_DPad_Down || Key == EKeys::Gamepad_LeftStick_Down)
        Out = EUINavigation::Down;
    else if (Key == EKeys::Gamepad_DPad_Left || Key == EKeys::Gamepad_LeftStick_Left)
        Out = EUINavigation::Left;
    else if (Key == EKeys::Gamepad_DPad_Right || Key == EKeys::Gamepad_LeftStick_Right)
        Out = EUINavigation::Right;
    else
        return false;
    return true;
}
} // namespace

bool TeleportLogisticsGamepad::IsGamepadKey(const FKeyEvent &Key)
{
    return Key.GetKey().IsGamepadKey();
}

TeleportLogisticsGamepad::EResult TeleportLogisticsGamepad::HandleKey(const TSharedPtr<SWidget> &Root,
                                                                      const FKeyEvent &Key)
{
    const FKey K = Key.GetKey();
    if (KeyboardOpenFor(Root))
        return KeyboardKey(K, Key.GetUserIndex()) ? EResult::Handled : EResult::Unhandled;
    if (K == EKeys::Gamepad_FaceButton_Right)
        return EResult::Close;
    EUINavigation Nav;
    const bool Move = Direction(K, Nav);
    const bool Accept = K == EKeys::Gamepad_FaceButton_Bottom;
    if (!Root || (!Move && !Accept))
        return EResult::Unhandled;

    TArray<TSharedRef<SWidget>> Candidates;
    Collect(Root.ToSharedRef(), Candidates);
    if (Candidates.IsEmpty())
        return Move ? EResult::Handled : EResult::Unhandled;
    const uint32 User = Key.GetUserIndex();
    const TSharedPtr<SWidget> At = Current(Candidates, User);

    if (Accept)
    {
        if (At && At->GetType() == ButtonType)
        {
            FButtonAccess::Click(*StaticCastSharedPtr<SButton>(At));
            return EResult::Handled;
        }
        if (!At)
        {
            Focus(First(Candidates), User);
            return EResult::Handled;
        }
        // A on a text box starts editing, as a click would, and opens Steam's keyboard.
        const TSharedPtr<SWidget> Editor = FindType(At.ToSharedRef(), TEXT("SEditableText"));
        FSlateApplication::Get().SetUserFocus(User, Editor ? Editor : At, EFocusCause::Mouse);
        const TCHAR *Failure = ShowSteamKeyboard(At->GetPaintSpaceGeometry());
        UE_LOG(LogTeleportLogistics, Log, TEXT("Gamepad: editing text field, Steam keyboard %s%s"),
               Failure ? TEXT("unavailable, using the built-in keyboard: ") : TEXT("requested"),
               Failure ? Failure : TEXT(""));
        if (Failure)
        {
            Keyboard = FKeyboard();
            Keyboard.Root = Root;
        }
        return EResult::Handled;
    }

    if (!At)
        Focus(First(Candidates), User);
    else if (const TSharedPtr<SWidget> Next = Nearest(Candidates, At.ToSharedRef(), Nav))
        Focus(Next, User);
    return EResult::Handled;
}

int32 TeleportLogisticsGamepad::PaintFocus(const TSharedPtr<SWidget> &Root, FSlateWindowElementList &OutDrawElements,
                                           int32 LayerId)
{
    if (!Root || !FSlateApplication::IsInitialized())
        return LayerId;
    TArray<TSharedRef<SWidget>> Candidates;
    Collect(Root.ToSharedRef(), Candidates);
    const TSharedPtr<SWidget> At = Current(Candidates, 0);
    if (!At)
    {
        Keyboard.Root.Reset();
        return LayerId;
    }
    static const FSlateRoundedBoxBrush Outline(FLinearColor::Transparent, 3.f, FLinearColor(0.98f, 0.58f, 0.13f),
                                               2.f);
    FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 1, At->GetPaintSpaceGeometry().ToPaintGeometry(), &Outline);
    return KeyboardOpenFor(Root) ? PaintKeyboard(Root, OutDrawElements, LayerId + 1) : LayerId + 1;
}
