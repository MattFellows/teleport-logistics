#include "TeleportLogisticsGamepad.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Framework/Application/SlateApplication.h"
#include "InputCoreTypes.h"
#include "Layout/Children.h"
#include "Rendering/DrawElements.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SWidget.h"

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
    FSlateApplication::Get().SetUserFocus(UserIndex, Target, EFocusCause::Navigation);
    for (TSharedPtr<SWidget> P = Target->GetParentWidget(); P; P = P->GetParentWidget())
        if (P->GetType() == ScrollBoxType)
            StaticCastSharedPtr<SScrollBox>(P)->ScrollDescendantIntoView(Target, true,
                                                                         EDescendantScrollDestination::IntoView, 8);
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
        // Text boxes keep A for their own editing.
        return EResult::Unhandled;
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
        return LayerId;
    static const FSlateRoundedBoxBrush Outline(FLinearColor::Transparent, 3.f, FLinearColor(0.98f, 0.58f, 0.13f),
                                               2.f);
    FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 1, At->GetPaintSpaceGeometry().ToPaintGeometry(), &Outline);
    return LayerId + 1;
}
