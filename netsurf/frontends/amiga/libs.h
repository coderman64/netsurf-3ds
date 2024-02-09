/*
 * Copyright 2014 Chris Young <chris@unsatisfactorysoftware.co.uk>
 *
 * This file is part of NetSurf, http://www.netsurf-browser.org/
 *
 * NetSurf is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * NetSurf is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef AMIGA_LIBS_H
#include <stdbool.h>
#include <intuition/classes.h>

/* BOOPSI classes */
extern Class *ARexxClass;
extern Class *BevelClass;
extern Class *BitMapClass;
extern Class *ButtonClass;
extern Class *CheckBoxClass;
extern Class *ChooserClass;
extern Class *ClickTabClass;
extern Class *FuelGaugeClass;
extern Class *GetFileClass;
extern Class *GetFontClass;
extern Class *GetScreenModeClass;
extern Class *IntegerClass;
extern Class *LabelClass;
extern Class *LayoutClass;
extern Class *ListBrowserClass;
#ifndef __amigaos4__
/* OS4 uses a public class name instead */
extern Class *PageClass;
#endif
extern Class *RadioButtonClass;
extern Class *ScrollerClass;
extern Class *SpaceClass;
extern Class *SpeedBarClass;
extern Class *StringClass;
extern Class *WindowClass;

/* New improved ReAction macros! */
#define ARexxObj			NewObject(ARexxClass, NULL
#define BevelObj			NewObject(BevelClass, NULL
#define BitMapObj			NewObject(BitMapClass, NULL
#define ButtonObj			NewObject(ButtonClass, NULL
#define CheckBoxObj			NewObject(CheckBoxClass, NULL
#define ChooserObj			NewObject(ChooserClass, NULL
#define ClickTabObj			NewObject(ClickTabClass, NULL
#define FuelGaugeObj		NewObject(FuelGaugeClass, NULL
#define GetFileObj			NewObject(GetFileClass, NULL
#define GetFontObj			NewObject(GetFontClass, NULL
#define GetScreenModeObj	NewObject(GetScreenModeClass, NULL
#define IntegerObj			NewObject(IntegerClass, NULL
#define LabelObj			NewObject(LabelClass, NULL
#define LayoutHObj			NewObject(LayoutClass, NULL, LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ
#define LayoutVObj			NewObject(LayoutClass, NULL, LAYOUT_Orientation, LAYOUT_ORIENT_VERT
#define ListBrowserObj			NewObject(ListBrowserClass, NULL
#ifdef __amigaos4__
#define PageObj				NewObject(NULL, "page.gadget"
#else
#define PageObj				NewObject(PageClass, NULL
#endif
#define RadioButtonObj		NewObject(RadioButtonClass, NULL
#define ScrollerObj			NewObject(ScrollerClass, NULL
#define SpaceObj			NewObject(SpaceClass, NULL
#define SpeedBarObj			NewObject(SpeedBarClass, NULL
#define StringObj			NewObject(StringClass, NULL
#define WindowObj			NewObject(WindowClass, NULL

/* Functions */
bool ami_libs_open(void);
void ami_libs_close(void);
#endif

