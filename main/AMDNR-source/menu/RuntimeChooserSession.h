// Copyright (c) 2026 3zwr1 (AMDNR). Part of AMDNR (GPL-3.0; see Licenses/AMDNR_NOTICE.txt).
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
// AMDNR 0.3.4.2 (Assetto Corsa): the rules of the first-launch neural runtime chooser (menu_common.cpp,
// RenderNeuralRuntimeChooser) as a state machine without ImGui or Windows, so they are unit tested
// (tests\034\runtime_chooser_session_test.cpp). In AC the chooser opened by itself while the game was loading, "Use
// this runtime" stayed greyed until a row was clicked, clicks were lost at 3-10 fps, and hiding the menu with the menu
// key left the modal in ImGui's popup stack, so it came back on every open. Now:
// - closing the menu while the chooser is open means "Decide later" for this session;
// - keys work at any frame rate (read from OptiInput by MenuCommon::UpdateManualInput): 1 / 2 (or Numpad 1 / 2) pick
//   a row, Enter uses the picked row, Escape decides later; a key pressed before the chooser appeared is ignored, and
//   so is the menu key (that one closes the menu);
// - the row of the runtime running now is preselected until the player picks, so "Use this runtime" works at once;
// - the title bar X decides later as well.
// NrBackend is still written by the caller through Config::DlssNrBackend + SaveIni, exactly as before.
// (H2) The chooser no longer opens the menu by itself at boot: RuntimeChooserBoot below posts one notification per
// process instead, and the chooser appears the first time the player opens the menu.

#include "input/KeyPressLatch.h" // MenuInput::TickBefore

#include <cstdint>
#include <string>

namespace MenuUi
{
enum class ChooserKey : uint8_t
{
    None,
    Row1,  // 1 or Numpad 1: the first row (danielblnc)
    Row2,  // 2 or Numpad 2: the second row (lmxxf)
    Enter, // "Use this runtime"
    Escape // "Decide later"
};

enum class ChooserEvent : uint8_t
{
    None,         // nothing to do (chooser not on screen, row missing, nothing usable picked)
    Stale,        // the key was pressed before the chooser appeared
    Picked,       // a row was picked
    UseRequested, // the next frame of the chooser writes NrBackend for the picked row
    Later,        // decided later: the next frame closes the chooser
};

enum class LaterReason : uint8_t
{
    Button,
    Escape,
    TitleBarX,
    MenuClosed,
};

inline const char* LaterReasonName(LaterReason r)
{
    switch (r)
    {
    case LaterReason::Button:
        return "Decide later button";
    case LaterReason::Escape:
        return "Esc key";
    case LaterReason::TitleBarX:
        return "title bar X";
    case LaterReason::MenuClosed:
        return "menu closed";
    }
    return "?";
}

// Virtual-key codes of winuser.h, repeated so this header needs no Windows.h.
inline constexpr int kVk1 = 0x31;
inline constexpr int kVk2 = 0x32;
inline constexpr int kVkNumpad1 = 0x61;
inline constexpr int kVkNumpad2 = 0x62;
inline constexpr int kVkReturn = 0x0D;
inline constexpr int kVkEscape = 0x1B;
inline constexpr int kChooserKeyVks[] = { kVk1, kVk2, kVkNumpad1, kVkNumpad2, kVkReturn, kVkEscape };

// The chooser key of a virtual key; the menu key is never one (it closes the menu, which decides later).
inline ChooserKey ChooserKeyFromVk(int vk, int menuKeyVk)
{
    if (vk == menuKeyVk)
        return ChooserKey::None;
    switch (vk)
    {
    case kVk1:
    case kVkNumpad1:
        return ChooserKey::Row1;
    case kVk2:
    case kVkNumpad2:
        return ChooserKey::Row2;
    case kVkReturn:
        return ChooserKey::Enter;
    case kVkEscape:
        return ChooserKey::Escape;
    default:
        return ChooserKey::None;
    }
}

struct RuntimeChooserSession
{
    static constexpr int kMaxRows = 4;

    bool opened = false;          // the popup was opened (once per process)
    bool later = false;           // "Decide later" for this session: never shown again
    bool used = false;            // "Use this runtime" done (NrBackend written)
    bool usePending = false;      // Enter: the next frame of the chooser writes NrBackend and closes it
    bool closePending = false;    // Esc: the next frame of the chooser closes it
    int pick = -1;                // the row the player picked (-1 = none yet)
    bool userPicked = false;      // the player picked (by key or click): the preselection no longer moves
    int preselect = -1;           // row of the runtime running now (AmdBridge::ActiveRuntime), until the player picks
    uint32_t shownAtMs = 0;       // tick the chooser appeared (this menu open)
    bool visibleLastFrame = false;
    int rowCount = 0;             // rows of the last frame drawn, and which can be used
    bool rowInstalled[kMaxRows] = {};

    // The chooser is to be shown this frame: a choice is still needed and the player has not answered.
    bool Wanted(bool choiceNeeded) const { return choiceNeeded && !later && !used; }
    // The chooser's render has to run: shown, or a key answer to apply inside its popup.
    bool NeedsRender(bool choiceNeeded) const { return Wanted(choiceNeeded) || usePending || closePending; }

    // true once: the caller opens the popup.
    bool OpenOnce()
    {
        if (opened)
            return false;
        opened = true;
        return true;
    }

    int EffectivePick() const { return userPicked ? pick : preselect; }
    bool CanUse() const
    {
        const int p = EffectivePick();
        return p >= 0 && p < rowCount && p < kMaxRows && rowInstalled[p];
    }

    void SetRows(int count, const bool* installed)
    {
        rowCount = count < 0 ? 0 : (count > kMaxRows ? kMaxRows : count);
        for (int i = 0; i < kMaxRows; ++i)
            rowInstalled[i] = i < rowCount && installed != nullptr && installed[i];
    }
    // The row of the runtime running now (-1 = none yet); it matters only until the player picks.
    void SetPreselect(int row) { preselect = row >= 0 && row < rowCount ? row : -1; }
    // A row picked by click or key; false when there is no such row.
    bool PickRow(int row)
    {
        if (row < 0 || row >= rowCount)
            return false;
        pick = row;
        userPicked = true;
        return true;
    }

    // Once per frame: the chooser was drawn (true) or not. The first visible frame of an open is shownAtMs.
    void OnFrame(bool visible, uint32_t nowMs)
    {
        if (visible && !visibleLastFrame)
            shownAtMs = nowMs;
        visibleLastFrame = visible;
    }

    // A press of a chooser key, with its event time, read while the chooser was on screen last frame.
    ChooserEvent OnKey(ChooserKey key, uint32_t eventMs)
    {
        if (key == ChooserKey::None || !visibleLastFrame || later || used || usePending)
            return ChooserEvent::None;
        if (MenuInput::TickBefore(eventMs, shownAtMs))
            return ChooserEvent::Stale;
        switch (key)
        {
        case ChooserKey::Row1:
            return PickRow(0) ? ChooserEvent::Picked : ChooserEvent::None;
        case ChooserKey::Row2:
            return PickRow(1) ? ChooserEvent::Picked : ChooserEvent::None;
        case ChooserKey::Enter:
            if (!CanUse())
                return ChooserEvent::None;
            usePending = true;
            return ChooserEvent::UseRequested;
        case ChooserKey::Escape:
            later = true;
            closePending = true;
            return ChooserEvent::Later;
        default:
            return ChooserEvent::None;
        }
    }

    // Inside the chooser's popup: the row whose NrBackend is to be written now (-1 = none); the caller writes it and
    // closes the popup. From Enter (a pending use) or the "Use this runtime" button (`byButton`).
    int TakeUse(bool byButton)
    {
        if (!byButton && !usePending)
            return -1;
        usePending = false;
        if (!CanUse())
            return -1;
        used = true;
        return EffectivePick();
    }

    // Inside the popup: an Esc answer to apply (the caller closes the popup). One-shot.
    bool TakeClose()
    {
        const bool c = closePending;
        closePending = false;
        return c;
    }

    // "Decide later" by the button or the title bar X (true = the answer is new; log it).
    bool DecideLater()
    {
        if (later || used)
            return false;
        later = true;
        return true;
    }

    // The menu was hidden (menu key, Close button) while the chooser was open and still needed: "Decide later" for this
    // session. true = changed (log it); the caller also trims ImGui's popup stack.
    bool OnMenuHidden(bool choiceNeeded)
    {
        usePending = false;
        closePending = false;
        visibleLastFrame = false;
        if (!opened || later || used || !choiceNeeded)
            return false;
        later = true;
        return true;
    }
};

// AMDNR 0.3.4.2 (H2): what the menu does at boot while the choice is still open. Until 0.3.4.1 RenderMenu opened the
// menu once by itself so the chooser was the first thing on screen; over a loading game at a few frames per second
// that trapped players (Assetto Corsa, F1 25). Now the menu stays closed and one notification per process says which
// runtime runs until the player picks and how to open the menu; the chooser appears the first time the player opens
// the menu, as before. Nothing here writes NrBackend: what runs while unchosen is the bridge's own rule (AmdBridge
// LmxxfWanted: danielblnc's runtime while its files are there and lmxxf was not chosen), exactly as in 0.3.4.1.
enum class BootAction : uint8_t
{
    None,
    Notify, // post the notification (once per process); there is no "open the menu" action
};

struct RuntimeChooserBoot
{
    bool notified = false;

    // Once per RenderMenu, before anything is drawn. Notify exactly once: a choice is open and the menu is hidden
    // (with the menu open the chooser itself is on screen; after a "Decide later" there the notice follows when the
    // menu closes, so the player still learns what runs).
    BootAction OnFrame(bool choiceNeeded, bool menuVisible)
    {
        if (notified || !choiceNeeded || menuVisible)
            return BootAction::None;
        notified = true;
        return BootAction::Notify;
    }
};

// The notification's text. `runtime` is the runtime that runs, named by the Neural tab's own rule
// (RuntimeCaps::Menu().name); `menuKey` is the configured menu key's name (MenuCommon::KeyName).
// The first line speaks of the choice, not of an install state: the bridge asks for a choice as soon as
// danielblnc's runtime and lmxxf's assets are there (AmdBridge::RuntimeChoiceNeeded), which is also true for a
// player who has the lmxxf assets but no LmxxfNrRuntime.dll -- the chooser itself then shows lmxxf as not installed,
// so a notice claiming two installed runtimes would be wrong.
inline std::string BootNoticeText(const char* runtime, const char* menuKey)
{
    return std::string("A second neural runtime was found.\n") + runtime + " runs until you pick one: open the menu (" +
           menuKey + ") > Neural";
}
} // namespace MenuUi
