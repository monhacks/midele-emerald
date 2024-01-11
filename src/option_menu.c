#include "global.h"
#include "option_menu.h"
#include "main.h"
#include "menu.h"
#include "scanline_effect.h"
#include "palette.h"
#include "sprite.h"
#include "task.h"
#include "bg.h"
#include "gpu_regs.h"
#include "window.h"
#include "text.h"
#include "text_window.h"
#include "international_string_util.h"
#include "strings.h"
#include "gba/m4a_internal.h"
#include "constants/rgb.h"
#include "string_util.h"
#include "event_data.h"

// Task data
enum
{
    TD_MENUSELECTION,
    TD_TEXTSPEED,
    TD_BATTLESCENE,
    TD_SOUND,
    TD_BUTTONMODE,
    TD_FRAMETYPE,
    TD_AUTORUN,
    TD_MAIN_UNIT_SYSTEM,
};

// Menu items Pg1
enum
{
    MENUITEM_TEXTSPEED,
    MENUITEM_BATTLESCENE,
    MENUITEM_SOUND,
    MENUITEM_BUTTONMODE,
    MENUITEM_FRAMETYPE,
    MENUITEM_AUTORUN,
    MENUITEM_CANCEL,
    MENUITEM_COUNT,
};

// Menu items Pg2
enum
{
    MENUITEM_MAIN_UNIT_SYSTEM,
    MENUITEM_CANCEL_PG2,
    MENUITEM_COUNT_PG2,
};

// Window Ids
enum
{
    WIN_TEXT_OPTION,
    WIN_OPTIONS
};

//Pg 1
#define YPOS_TEXTSPEED    (MENUITEM_TEXTSPEED * 16)
#define YPOS_BATTLESCENE  (MENUITEM_BATTLESCENE * 16)
#define YPOS_BATTLESTYLE  (MENUITEM_BATTLESTYLE * 16)
#define YPOS_SOUND        (MENUITEM_SOUND * 16)
#define YPOS_BUTTONMODE   (MENUITEM_BUTTONMODE * 16)
#define YPOS_FRAMETYPE    (MENUITEM_FRAMETYPE * 16)
#define YPOS_AUTORUN      (MENUITEM_AUTORUN * 16)

//Pg2
#define YPOS_MAIN_UNIT_SYSTEM      (MENUITEM_MAIN_UNIT_SYSTEM * 16)
#define PAGE_COUNT  2

// this file's functions
static void Task_OptionMenuFadeIn(u8 taskId);
static void Task_OptionMenuProcessInput(u8 taskId);
static void Task_OptionMenuFadeIn_Pg2(u8 taskId);
static void Task_OptionMenuProcessInput_Pg2(u8 taskId);
static void Task_OptionMenuSave(u8 taskId);
static void Task_OptionMenuFadeOut(u8 taskId);
static void HighlightOptionMenuItem(u8 selection);
static u8   TextSpeed_ProcessInput(u8 selection);
static void TextSpeed_DrawChoices(u8 selection);
static u8   BattleScene_ProcessInput(u8 selection);
static void BattleScene_DrawChoices(u8 selection);
static u8   Sound_ProcessInput(u8 selection);
static void Sound_DrawChoices(u8 selection);
static u8   FrameType_ProcessInput(u8 selection);
static void FrameType_DrawChoices(u8 selection);
static u8   ButtonMode_ProcessInput(u8 selection);
static void ButtonMode_DrawChoices(u8 selection);
static u8 Autorun_ProcessInput(u8 selection);
static void Autorun_DrawChoices(u8 selection);
static void DrawTextOption(void);
static void DrawOptionMenuTexts(void);
static void sub_80BB154(void);
static void DrawChoices_UnitSystem(u8 selection);

// EWRAM vars
EWRAM_DATA static bool8 sArrowPressed = FALSE;
EWRAM_DATA static u8 sCurrPage = 0;

// const rom data
static const u16 sUnknown_0855C604[] = INCBIN_U16("graphics/misc/option_menu_text.gbapal");
// note: this is only used in the Japanese release
static const u8 sEqualSignGfx[] = INCBIN_U8("graphics/misc/option_menu_equals_sign.4bpp");
static const u8 sText_UnitSystem[]  = _("UNIT SYSTEM");

static const u8 *const sOptionMenuItemsNames[MENUITEM_COUNT] =
{
    [MENUITEM_TEXTSPEED]   = gText_TextSpeed,
    [MENUITEM_BATTLESCENE] = gText_BattleScene,
    [MENUITEM_SOUND]       = gText_Sound,
    [MENUITEM_BUTTONMODE]  = gText_ButtonMode,
    [MENUITEM_FRAMETYPE]   = gText_Frame,
    [MENUITEM_AUTORUN]   = gText_Autorun,
    [MENUITEM_CANCEL]      = gText_OptionMenuCancel,
};

static const u8 *const sOptionMenuItemsNames_Pg2[MENUITEM_COUNT_PG2] =
{
    [MENUITEM_MAIN_UNIT_SYSTEM] = sText_UnitSystem,
    [MENUITEM_CANCEL_PG2]      = gText_OptionMenuCancel,
};

static const struct WindowTemplate sOptionMenuWinTemplates[] =
{
    {
        .bg = 1,
        .tilemapLeft = 2,
        .tilemapTop = 1,
        .width = 26,
        .height = 2,
        .paletteNum = 1,
        .baseBlock = 2
    },
    {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 5,
        .width = 26,
        .height = 14,
        .paletteNum = 1,
        .baseBlock = 0x36
    },
    DUMMY_WIN_TEMPLATE
};

static const struct BgTemplate sOptionMenuBgTemplates[] =
{
   {
       .bg = 1,
       .charBaseIndex = 1,
       .mapBaseIndex = 30,
       .screenSize = 0,
       .paletteMode = 0,
       .priority = 0,
       .baseTile = 0
   },
   {
       .bg = 0,
       .charBaseIndex = 1,
       .mapBaseIndex = 31,
       .screenSize = 0,
       .paletteMode = 0,
       .priority = 1,
       .baseTile = 0
   }
};

static const u16 sUnknown_0855C6A0[] = {0x7E51};

// code
static void MainCB2(void)
{
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
}

static void VBlankCB(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

static void ReadAllCurrentSettings(u8 taskId)
{
    gTasks[taskId].data[TD_MENUSELECTION] = 0;
    gTasks[taskId].data[TD_TEXTSPEED] = gSaveBlock2Ptr->optionsTextSpeed;
    gTasks[taskId].data[TD_BATTLESCENE] = gSaveBlock2Ptr->optionsBattleSceneOff;
    gTasks[taskId].data[TD_SOUND] = gSaveBlock2Ptr->optionsSound;
    gTasks[taskId].data[TD_BUTTONMODE] = gSaveBlock2Ptr->optionsButtonMode;
    gTasks[taskId].data[TD_FRAMETYPE] = gSaveBlock2Ptr->optionsWindowFrameType;
	gTasks[taskId].data[TD_AUTORUN] = gSaveBlock2Ptr->autoRun;
    gTasks[taskId].data[TD_MAIN_UNIT_SYSTEM] = FlagGet(FLAG_UNIT_SYSTEM);
}

static void DrawOptionsPg1(u8 taskId)
{  
    ReadAllCurrentSettings(taskId);
    TextSpeed_DrawChoices(gTasks[taskId].data[TD_TEXTSPEED]);
    BattleScene_DrawChoices(gTasks[taskId].data[TD_BATTLESCENE]);
    Sound_DrawChoices(gTasks[taskId].data[TD_SOUND]);
    ButtonMode_DrawChoices(gTasks[taskId].data[TD_BUTTONMODE]);
    FrameType_DrawChoices(gTasks[taskId].data[TD_FRAMETYPE]);
    Autorun_DrawChoices(gTasks[taskId].data[TD_AUTORUN]);
    HighlightOptionMenuItem(gTasks[taskId].data[TD_MENUSELECTION]);
    CopyWindowToVram(WIN_OPTIONS, 3);
}

static void DrawOptionsPg2(u8 taskId)
{
    ReadAllCurrentSettings(taskId);
    DrawChoices_UnitSystem(gTasks[taskId].data[TD_MAIN_UNIT_SYSTEM]);
    HighlightOptionMenuItem(gTasks[taskId].data[TD_MENUSELECTION]);
    CopyWindowToVram(WIN_OPTIONS, 3);
}

void CB2_InitOptionMenu(void)
{
	u8 taskId;
    switch (gMain.state)
    {
    default:
    case 0:
        SetVBlankCallback(NULL);
        gMain.state++;
        break;
    case 1:
        DmaClearLarge16(3, (void*)(VRAM), VRAM_SIZE, 0x1000);
        DmaClear32(3, OAM, OAM_SIZE);
        DmaClear16(3, PLTT, PLTT_SIZE);
        SetGpuReg(REG_OFFSET_DISPCNT, 0);
        ResetBgsAndClearDma3BusyFlags(0);
        InitBgsFromTemplates(0, sOptionMenuBgTemplates, ARRAY_COUNT(sOptionMenuBgTemplates));
        ChangeBgX(0, 0, 0);
        ChangeBgY(0, 0, 0);
        ChangeBgX(1, 0, 0);
        ChangeBgY(1, 0, 0);
        ChangeBgX(2, 0, 0);
        ChangeBgY(2, 0, 0);
        ChangeBgX(3, 0, 0);
        ChangeBgY(3, 0, 0);
        InitWindows(sOptionMenuWinTemplates);
        DeactivateAllTextPrinters();
        SetGpuReg(REG_OFFSET_WIN0H, 0);
        SetGpuReg(REG_OFFSET_WIN0V, 0);
        SetGpuReg(REG_OFFSET_WININ, 1);
        SetGpuReg(REG_OFFSET_WINOUT, 35);
        SetGpuReg(REG_OFFSET_BLDCNT, 193);
        SetGpuReg(REG_OFFSET_BLDALPHA, 0);
        SetGpuReg(REG_OFFSET_BLDY, 4);
        SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_WIN0_ON | DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP);
        ShowBg(0);
        ShowBg(1);
        gMain.state++;
        break;
    case 2:
        ResetPaletteFade();
        ScanlineEffect_Stop();
        ResetTasks();
        ResetSpriteData();
        gMain.state++;
        break;
    case 3:
        LoadBgTiles(1, GetWindowFrameTilesPal(gSaveBlock2Ptr->optionsWindowFrameType)->tiles, 0x120, 0x1A2);
        gMain.state++;
        break;
    case 4:
        LoadPalette(sUnknown_0855C6A0, 0, sizeof(sUnknown_0855C6A0));
        LoadPalette(GetWindowFrameTilesPal(gSaveBlock2Ptr->optionsWindowFrameType)->pal, 0x70, 0x20);
        gMain.state++;
        break;
    case 5:
        LoadPalette(sUnknown_0855C604, 0x10, sizeof(sUnknown_0855C604));
        gMain.state++;
        break;
    case 6:
        PutWindowTilemap(0);
        DrawTextOption();
        gMain.state++;
        break;
    case 7:
        gMain.state++;
        break;
    case 8:
        PutWindowTilemap(1);
        DrawOptionMenuTexts();
        gMain.state++;
    case 9:
        sub_80BB154();
        gMain.state++;
        break;
    case 10:
    {
      switch(sCurrPage)
        {
        case 0:
            taskId = CreateTask(Task_OptionMenuFadeIn, 0);
            DrawOptionsPg1(taskId);
            break;
        case 1:
            taskId = CreateTask(Task_OptionMenuFadeIn_Pg2, 0);
            DrawOptionsPg2(taskId);
            break;            
        }
        gMain.state++;
        break;
    }
    case 11:
        BeginNormalPaletteFade(0xFFFFFFFF, 0, 0x10, 0, RGB_BLACK);
        SetVBlankCallback(VBlankCB);
        SetMainCallback2(MainCB2);
        return;
    }
}

static u8 Process_ChangePage(u8 CurrentPage)
{
    if (gMain.newKeys & R_BUTTON)
    {
        if (CurrentPage < PAGE_COUNT - 1)
            CurrentPage++;
        else
            CurrentPage = 0;
    }
    if (gMain.newKeys & L_BUTTON)
    {
        if (CurrentPage != 0)
            CurrentPage--;
        else
            CurrentPage = PAGE_COUNT - 1;
    }
    return CurrentPage;
}

static void Task_ChangePage(u8 taskId)
{
    DrawTextOption();
    PutWindowTilemap(1);
    DrawOptionMenuTexts();
    switch(sCurrPage)
    {
    case 0:
        DrawOptionsPg1(taskId);
        gTasks[taskId].func = Task_OptionMenuFadeIn;
        break;
    case 1:
        DrawOptionsPg2(taskId);
        gTasks[taskId].func = Task_OptionMenuFadeIn_Pg2;
        break;
    }
}

static void Task_OptionMenuFadeIn(u8 taskId)
{
    if (!gPaletteFade.active)
        gTasks[taskId].func = Task_OptionMenuProcessInput;
}

static void Task_OptionMenuProcessInput(u8 taskId)
{
    if ((gMain.newKeys & L_BUTTON) || (gMain.newKeys & R_BUTTON))
    {
        FillWindowPixelBuffer(WIN_OPTIONS, PIXEL_FILL(1));
        ClearStdWindowAndFrame(WIN_OPTIONS, FALSE);
        sCurrPage = Process_ChangePage(sCurrPage);
        gTasks[taskId].func = Task_ChangePage;
    }
    else if (gMain.newKeys & A_BUTTON)
    {
        if (gTasks[taskId].data[TD_MENUSELECTION] == MENUITEM_CANCEL)
            gTasks[taskId].func = Task_OptionMenuSave;
    }
    else if (gMain.newKeys & B_BUTTON)
    {
        gTasks[taskId].func = Task_OptionMenuSave;
    }
    else if (gMain.newKeys & DPAD_UP)
    {
        if (gTasks[taskId].data[TD_MENUSELECTION] > 0)
            gTasks[taskId].data[TD_MENUSELECTION]--;
        else
            gTasks[taskId].data[TD_MENUSELECTION] = MENUITEM_CANCEL;
        HighlightOptionMenuItem(gTasks[taskId].data[TD_MENUSELECTION]);
    }
    else if (gMain.newKeys & DPAD_DOWN)
    {
        if (gTasks[taskId].data[TD_MENUSELECTION] < MENUITEM_CANCEL)
            gTasks[taskId].data[TD_MENUSELECTION]++;
        else
            gTasks[taskId].data[TD_MENUSELECTION] = 0;
        HighlightOptionMenuItem(gTasks[taskId].data[TD_MENUSELECTION]);
    }
    else
    {
        u8 previousOption;

        switch (gTasks[taskId].data[TD_MENUSELECTION])
        {
        case MENUITEM_TEXTSPEED:
            previousOption = gTasks[taskId].data[TD_TEXTSPEED];
            gTasks[taskId].data[TD_TEXTSPEED] = TextSpeed_ProcessInput(gTasks[taskId].data[TD_TEXTSPEED]);

            if (previousOption != gTasks[taskId].data[TD_TEXTSPEED])
                TextSpeed_DrawChoices(gTasks[taskId].data[TD_TEXTSPEED]);
            break;
        case MENUITEM_BATTLESCENE:
            previousOption = gTasks[taskId].data[TD_BATTLESCENE];
            gTasks[taskId].data[TD_BATTLESCENE] = BattleScene_ProcessInput(gTasks[taskId].data[TD_BATTLESCENE]);

            if (previousOption != gTasks[taskId].data[TD_BATTLESCENE])
                BattleScene_DrawChoices(gTasks[taskId].data[TD_BATTLESCENE]);
            break;
        case MENUITEM_SOUND:
            previousOption = gTasks[taskId].data[TD_SOUND];
            gTasks[taskId].data[TD_SOUND] = Sound_ProcessInput(gTasks[taskId].data[TD_SOUND]);

            if (previousOption != gTasks[taskId].data[TD_SOUND])
                Sound_DrawChoices(gTasks[taskId].data[TD_SOUND]);
            break;
        case MENUITEM_BUTTONMODE:
            previousOption = gTasks[taskId].data[TD_BUTTONMODE];
            gTasks[taskId].data[TD_BUTTONMODE] = ButtonMode_ProcessInput(gTasks[taskId].data[TD_BUTTONMODE]);

            if (previousOption != gTasks[taskId].data[TD_BUTTONMODE])
                ButtonMode_DrawChoices(gTasks[taskId].data[TD_BUTTONMODE]);
            break;
        case MENUITEM_FRAMETYPE:
            previousOption = gTasks[taskId].data[TD_FRAMETYPE];
            gTasks[taskId].data[TD_FRAMETYPE] = FrameType_ProcessInput(gTasks[taskId].data[TD_FRAMETYPE]);

            if (previousOption != gTasks[taskId].data[TD_FRAMETYPE])
                FrameType_DrawChoices(gTasks[taskId].data[TD_FRAMETYPE]);
            break;
        case MENUITEM_AUTORUN:
            previousOption = gTasks[taskId].data[TD_AUTORUN];
            gTasks[taskId].data[TD_AUTORUN] = Autorun_ProcessInput(gTasks[taskId].data[TD_AUTORUN]);

            if (previousOption != gTasks[taskId].data[TD_AUTORUN])
                Autorun_DrawChoices(gTasks[taskId].data[TD_AUTORUN]);
            break;
        default:
            return;
        }

        if (sArrowPressed)
        {
            sArrowPressed = FALSE;
            CopyWindowToVram(WIN_OPTIONS, 2);
        }
    }
}

static void Task_OptionMenuFadeIn_Pg2(u8 taskId)
{
    if (!gPaletteFade.active)
        gTasks[taskId].func = Task_OptionMenuProcessInput_Pg2;
}

static void Task_OptionMenuProcessInput_Pg2(u8 taskId)
{
    if ((gMain.newKeys & L_BUTTON) || (gMain.newKeys & R_BUTTON))
    {
        FillWindowPixelBuffer(WIN_OPTIONS, PIXEL_FILL(1));
        ClearStdWindowAndFrame(WIN_OPTIONS, FALSE);
        sCurrPage = Process_ChangePage(sCurrPage);
        gTasks[taskId].func = Task_ChangePage;
    }
    else if (gMain.newKeys & A_BUTTON)
    {
        if (gTasks[taskId].data[TD_MENUSELECTION] == MENUITEM_CANCEL_PG2)
            gTasks[taskId].func = Task_OptionMenuSave;
    }
    else if (gMain.newKeys & B_BUTTON)
    {
        gTasks[taskId].func = Task_OptionMenuSave;
    }
    else if (gMain.newKeys & DPAD_UP)
    {
        if (gTasks[taskId].data[TD_MENUSELECTION] > 0)
            gTasks[taskId].data[TD_MENUSELECTION]--;
        else
            gTasks[taskId].data[TD_MENUSELECTION] = MENUITEM_CANCEL_PG2;
        HighlightOptionMenuItem(gTasks[taskId].data[TD_MENUSELECTION]);
    }
    else if (gMain.newKeys & DPAD_DOWN)
    {
        if (gTasks[taskId].data[TD_MENUSELECTION] < MENUITEM_CANCEL_PG2)
            gTasks[taskId].data[TD_MENUSELECTION]++;
        else
            gTasks[taskId].data[TD_MENUSELECTION] = 0;
        HighlightOptionMenuItem(gTasks[taskId].data[TD_MENUSELECTION]);
    }
    else
    {
        u8 previousOption;

        switch (gTasks[taskId].data[TD_MENUSELECTION])
        {
        case MENUITEM_MAIN_UNIT_SYSTEM:
            previousOption = gTasks[taskId].data[TD_MAIN_UNIT_SYSTEM];
            gTasks[taskId].data[TD_MAIN_UNIT_SYSTEM] = BattleScene_ProcessInput(gTasks[taskId].data[TD_MAIN_UNIT_SYSTEM]);

            if (previousOption != gTasks[taskId].data[TD_MAIN_UNIT_SYSTEM])
                DrawChoices_UnitSystem(gTasks[taskId].data[TD_MAIN_UNIT_SYSTEM]);
            break;
        default:
            return;
        }

        if (sArrowPressed)
        {
            sArrowPressed = FALSE;
            CopyWindowToVram(WIN_OPTIONS, 3);
        }
    }
}

static void Task_OptionMenuSave(u8 taskId)
{
    gSaveBlock2Ptr->optionsTextSpeed = gTasks[taskId].data[TD_TEXTSPEED];
    gSaveBlock2Ptr->optionsBattleSceneOff = gTasks[taskId].data[TD_BATTLESCENE];
    gSaveBlock2Ptr->optionsSound = gTasks[taskId].data[TD_SOUND];
    gSaveBlock2Ptr->optionsButtonMode = gTasks[taskId].data[TD_BUTTONMODE];
    gSaveBlock2Ptr->optionsWindowFrameType = gTasks[taskId].data[TD_FRAMETYPE];
    gSaveBlock2Ptr->autoRun = gTasks[taskId].data[TD_AUTORUN];
	gTasks[taskId].data[TD_MAIN_UNIT_SYSTEM] == 0 ? FlagClear(FLAG_UNIT_SYSTEM) : FlagSet(FLAG_UNIT_SYSTEM);

    BeginNormalPaletteFade(0xFFFFFFFF, 0, 0, 0x10, RGB_BLACK);
    gTasks[taskId].func = Task_OptionMenuFadeOut;
}

static void Task_OptionMenuFadeOut(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        DestroyTask(taskId);
        FreeAllWindowBuffers();
        SetMainCallback2(gMain.savedCallback);
    }
}

static void HighlightOptionMenuItem(u8 index)
{
    SetGpuReg(REG_OFFSET_WIN0H, WIN_RANGE(16, 224));
    SetGpuReg(REG_OFFSET_WIN0V, WIN_RANGE(index * 16 + 40, index * 16 + 56));
}

static void DrawOptionMenuChoice(const u8 *text, u8 x, u8 y, u8 style)
{
    u8 dst[16];
    u16 i;

    for (i = 0; *text != EOS && i <= 14; i++)
        dst[i] = *(text++);

    if (style != 0)
    {
        dst[2] = 4;
        dst[5] = 5;
    }

    dst[i] = EOS;
    AddTextPrinterParameterized(WIN_OPTIONS, 1, dst, x, y + 1, TEXT_SPEED_FF, NULL);
}

static u8 TextSpeed_ProcessInput(u8 selection)
{
    if (gMain.newKeys & DPAD_RIGHT)
    {
        if (selection <= 1)
            selection++;
        else
            selection = 0;

        sArrowPressed = TRUE;
    }
    if (gMain.newKeys & DPAD_LEFT)
    {
        if (selection != 0)
            selection--;
        else
            selection = 2;

        sArrowPressed = TRUE;
    }
    return selection;
}

static void TextSpeed_DrawChoices(u8 selection)
{
    u8 styles[3];
    s32 widthSlow, widthMid, widthFast, xMid;

    styles[0] = 0;
    styles[1] = 0;
    styles[2] = 0;
    styles[selection] = 1;

    DrawOptionMenuChoice(gText_TextSpeedSlow, 104, YPOS_TEXTSPEED, styles[0]);

    widthSlow = GetStringWidth(1, gText_TextSpeedSlow, 0);
    widthMid = GetStringWidth(1, gText_TextSpeedMid, 0);
    widthFast = GetStringWidth(1, gText_TextSpeedFast, 0);

    widthMid -= 94;
    xMid = (widthSlow - widthMid - widthFast) / 2 + 104;
    DrawOptionMenuChoice(gText_TextSpeedMid, xMid, YPOS_TEXTSPEED, styles[1]);

    DrawOptionMenuChoice(gText_TextSpeedFast, GetStringRightAlignXOffset(1, gText_TextSpeedFast, 198), YPOS_TEXTSPEED, styles[2]);
}

static u8 BattleScene_ProcessInput(u8 selection)
{
    if (gMain.newKeys & (DPAD_LEFT | DPAD_RIGHT))
    {
        selection ^= 1;
        sArrowPressed = TRUE;
    }

    return selection;
}

static void DrawChoices_UnitSystem(u8 selection)
{
    u8 styles[2];

    styles[0] = 0;
    styles[1] = 0;
    styles[selection] = 1;

    DrawOptionMenuChoice(gText_UnitSystemMetric, 104, YPOS_MAIN_UNIT_SYSTEM, styles[0]);
    DrawOptionMenuChoice(gText_UnitSystemImperial, GetStringRightAlignXOffset(1, gText_UnitSystemMetric, 198), YPOS_MAIN_UNIT_SYSTEM, styles[1]);
}

static void BattleScene_DrawChoices(u8 selection)
{
    u8 styles[2];

    styles[0] = 0;
    styles[1] = 0;
    styles[selection] = 1;

    DrawOptionMenuChoice(gText_BattleSceneOn, 104, YPOS_BATTLESCENE, styles[0]);
    DrawOptionMenuChoice(gText_BattleSceneOff, GetStringRightAlignXOffset(1, gText_BattleSceneOff, 198), YPOS_BATTLESCENE, styles[1]);
}

static u8 Sound_ProcessInput(u8 selection)
{
    if (gMain.newKeys & (DPAD_LEFT | DPAD_RIGHT))
    {
        selection ^= 1;
        SetPokemonCryStereo(selection);
        sArrowPressed = TRUE;
    }

    return selection;
}

static void Sound_DrawChoices(u8 selection)
{
    u8 styles[2];

    styles[0] = 0;
    styles[1] = 0;
    styles[selection] = 1;

    DrawOptionMenuChoice(gText_SoundMono, 104, YPOS_SOUND, styles[0]);
    DrawOptionMenuChoice(gText_SoundStereo, GetStringRightAlignXOffset(1, gText_SoundStereo, 198), YPOS_SOUND, styles[1]);
}

static u8 FrameType_ProcessInput(u8 selection)
{
    if (gMain.newKeys & DPAD_RIGHT)
    {
        if (selection < WINDOW_FRAMES_COUNT - 1)
            selection++;
        else
            selection = 0;

        LoadBgTiles(1, GetWindowFrameTilesPal(selection)->tiles, 0x120, 0x1A2);
        LoadPalette(GetWindowFrameTilesPal(selection)->pal, 0x70, 0x20);
        sArrowPressed = TRUE;
    }
    if (gMain.newKeys & DPAD_LEFT)
    {
        if (selection != 0)
            selection--;
        else
            selection = WINDOW_FRAMES_COUNT - 1;

        LoadBgTiles(1, GetWindowFrameTilesPal(selection)->tiles, 0x120, 0x1A2);
        LoadPalette(GetWindowFrameTilesPal(selection)->pal, 0x70, 0x20);
        sArrowPressed = TRUE;
    }
    return selection;
}

static void FrameType_DrawChoices(u8 selection)
{
    u8 text[16];
    u8 n = selection + 1;
    u16 i;

    for (i = 0; gText_FrameTypeNumber[i] != EOS && i <= 5; i++)
        text[i] = gText_FrameTypeNumber[i];

    // Convert a number to decimal string
    if (n / 10 != 0)
    {
        text[i] = n / 10 + CHAR_0;
        i++;
        text[i] = n % 10 + CHAR_0;
        i++;
    }
    else
    {
        text[i] = n % 10 + CHAR_0;
        i++;
        text[i] = 0x77;
        i++;
    }

    text[i] = EOS;

    DrawOptionMenuChoice(gText_FrameType, 104, YPOS_FRAMETYPE, 0);
    DrawOptionMenuChoice(text, 128, YPOS_FRAMETYPE, 1);
}

static u8 ButtonMode_ProcessInput(u8 selection)
{
    if (gMain.newKeys & DPAD_RIGHT)
    {
        if (selection <= 1)
            selection++;
        else
            selection = 0;

        sArrowPressed = TRUE;
    }
    if (gMain.newKeys & DPAD_LEFT)
    {
        if (selection != 0)
            selection--;
        else
            selection = 2;

        sArrowPressed = TRUE;
    }
    return selection;
}

static void ButtonMode_DrawChoices(u8 selection)
{
    s32 widthNormal, widthLR, widthLA, xLR;
    u8 styles[3];

    styles[0] = 0;
    styles[1] = 0;
    styles[2] = 0;
    styles[selection] = 1;

    DrawOptionMenuChoice(gText_ButtonTypeNormal, 104, YPOS_BUTTONMODE, styles[0]);

    widthNormal = GetStringWidth(1, gText_ButtonTypeNormal, 0);
    widthLR = GetStringWidth(1, gText_ButtonTypeLR, 0);
    widthLA = GetStringWidth(1, gText_ButtonTypeLEqualsA, 0);

    widthLR -= 94;
    xLR = (widthNormal - widthLR - widthLA) / 2 + 104;
    DrawOptionMenuChoice(gText_ButtonTypeLR, xLR, YPOS_BUTTONMODE, styles[1]);

    DrawOptionMenuChoice(gText_ButtonTypeLEqualsA, GetStringRightAlignXOffset(1, gText_ButtonTypeLEqualsA, 198), YPOS_BUTTONMODE, styles[2]);
}

static u8 Autorun_ProcessInput(u8 selection)
{
  if (gMain.newKeys & (DPAD_LEFT | DPAD_RIGHT))
  {
      selection ^= 1;
      SetPokemonCryStereo(selection);
      sArrowPressed = TRUE;
  }

  return selection;
}

static void Autorun_DrawChoices(u8 selection)
{
  u8 styles[2];

  styles[0] = 0;
  styles[1] = 0;
  styles[selection] = 1;

  DrawOptionMenuChoice(gText_AutorunNormal, 104, YPOS_AUTORUN, styles[0]);
  DrawOptionMenuChoice(gText_AutorunAuto, GetStringRightAlignXOffset(1, gText_AutorunAuto, 198), YPOS_AUTORUN, styles[1]);
}

static void DrawTextOption(void)
{
    u32 i, widthOptions, xMid;
    u8 pageDots[9] = _("");  // Array size should be at least (2 * PAGE_COUNT) -1
    widthOptions = GetStringWidth(1, gText_Option, 0);

    for (i = 0; i < PAGE_COUNT; i++)
    {
        if (i == sCurrPage)
            StringAppend(pageDots, gText_LargeDot);
        else
            StringAppend(pageDots, gText_SmallDot);
        if (i < PAGE_COUNT - 1)
            StringAppend(pageDots, gText_Space);            
    }
    xMid = (8 + widthOptions + 5);
    FillWindowPixelBuffer(WIN_TEXT_OPTION, PIXEL_FILL(1));
    AddTextPrinterParameterized(WIN_TEXT_OPTION, 1, gText_Option, 8, 1, TEXT_SPEED_FF, NULL);
    AddTextPrinterParameterized(WIN_TEXT_OPTION, 1, pageDots, xMid, 1, TEXT_SPEED_FF, NULL);
    AddTextPrinterParameterized(WIN_TEXT_OPTION, 1, gText_PageNav, GetStringRightAlignXOffset(1, gText_PageNav, 198), 1, TEXT_SPEED_FF, NULL);
    CopyWindowToVram(WIN_TEXT_OPTION, 3);
}

static void DrawOptionMenuTexts(void)
{
    u8 i, items;
    const u8* const* menu = NULL;

    switch (sCurrPage){
    case 0:
        items = MENUITEM_COUNT;
        menu = sOptionMenuItemsNames;
        break;
    case 1:
        items = MENUITEM_COUNT_PG2;
        menu = sOptionMenuItemsNames_Pg2;
        break;    
    }

    FillWindowPixelBuffer(WIN_OPTIONS, PIXEL_FILL(1));
    for (i = 0; i < items; i++)
        AddTextPrinterParameterized(WIN_OPTIONS, 1, menu[i], 8, (i * 16) + 1, TEXT_SPEED_FF, NULL);
    CopyWindowToVram(WIN_OPTIONS, 3);
}

static void sub_80BB154(void)
{
    //                   bg, tileNum, x,    y,    width, height,  pal
    FillBgTilemapBufferRect(1, 0x1A2, 1,    0,      1,      1,      7);
    FillBgTilemapBufferRect(1, 0x1A3, 2,    0,      0x1B,   1,      7);
    FillBgTilemapBufferRect(1, 0x1A4, 28,   0,      1,      1,      7);
    FillBgTilemapBufferRect(1, 0x1A5, 1,    1,      1,      2,      7);
    FillBgTilemapBufferRect(1, 0x1A7, 28,   1,      1,      2,      7);
    FillBgTilemapBufferRect(1, 0x1A8, 1,    3,      1,      1,      7);
    FillBgTilemapBufferRect(1, 0x1A9, 2,    3,      0x1B,   1,      7);
    FillBgTilemapBufferRect(1, 0x1AA, 28,   3,      1,      1,      7);
    FillBgTilemapBufferRect(1, 0x1A2, 1,    4,      1,      1,      7);
    FillBgTilemapBufferRect(1, 0x1A3, 2,    4,      0x1A,   1,      7);
    FillBgTilemapBufferRect(1, 0x1A4, 28,   4,      1,      1,      7);
    FillBgTilemapBufferRect(1, 0x1A5, 1,    5,      1,      0x12,   7);
    FillBgTilemapBufferRect(1, 0x1A7, 28,   5,      1,      0x12,   7);
    FillBgTilemapBufferRect(1, 0x1A8, 1,    19,     1,      1,      7);
    FillBgTilemapBufferRect(1, 0x1A9, 2,    19,     0x1A,   1,      7);
    FillBgTilemapBufferRect(1, 0x1AA, 28,   19,     1,      1,      7);

    CopyBgTilemapBufferToVram(1);
}
