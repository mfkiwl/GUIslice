// =======================================================================
// GUIslice library (extensions)
// - Calvin Hass
// - https://www.impulseadventure.com/elec/guislice-gui.html
// - https://github.com/ImpulseAdventure/GUIslice
// =======================================================================
//
// The MIT License
//
// Copyright 2016-2019 Calvin Hass
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// =======================================================================
/// \file GUIslice_ex.c



// GUIslice library
#include "GUIslice.h"
#include "GUIslice_drv.h"

#include "extra/XTextbox.h"

#include <stdio.h>

#if (GSLC_USE_PROGMEM)
    #include <avr/pgmspace.h>
#endif

// ----------------------------------------------------------------------------
// Error Messages
// ----------------------------------------------------------------------------

extern const char GSLC_PMEM ERRSTR_NULL[];
extern const char GSLC_PMEM ERRSTR_PXD_NULL[];


// ----------------------------------------------------------------------------
// Extended element definitions
// ----------------------------------------------------------------------------
//
// - This file extends the core GUIslice functionality with
//   additional widget types
// - After adding any widgets to GUIslice_ex, a unique
//   enumeration (GSLC_TYPEX_*) should be added to "GUIslice.h"
//
//   TODO: Consider whether we should remove the need to update
//         these enumerations in "GUIslice.h"; we could instead
//         define a single "GSLC_TYPEX" in GUIslice.h but then
//         allow "GUIslice_ex.h" to create a new set of unique
//         enumerations. This way extended elements could be created
//         in GUIslice_ex and no changes at all would be required
//         in GUIslice.

// ----------------------------------------------------------------------------


// ============================================================================

gslc_tsElemRef* gslc_ElemXTextboxCreate(gslc_tsGui* pGui,int16_t nElemId,int16_t nPage,
  gslc_tsXTextbox* pXData,gslc_tsRect rElem,int16_t nFontId,char* pBuf,
    uint16_t nBufRows,uint16_t nBufCols)
{
  if ((pGui == NULL) || (pXData == NULL)) {
    static const char GSLC_PMEM FUNCSTR[] = "ElemXTextboxCreate";
    GSLC_DEBUG_PRINT_CONST(ERRSTR_NULL,FUNCSTR);
    return NULL;
  }
  gslc_tsElem     sElem;
  gslc_tsElemRef* pElemRef = NULL;
  sElem = gslc_ElemCreate(pGui,nElemId,nPage,GSLC_TYPEX_TEXTBOX,rElem,NULL,0,nFontId);
  sElem.nFeatures        |= GSLC_ELEM_FEA_FRAME_EN;
  sElem.nFeatures        |= GSLC_ELEM_FEA_FILL_EN;
  sElem.nFeatures        &= ~GSLC_ELEM_FEA_CLICK_EN;
  sElem.nFeatures        &= ~GSLC_ELEM_FEA_GLOW_EN;
  // Default group assignment. Can override later with ElemSetGroup()
  sElem.nGroup            = GSLC_GROUP_ID_NONE;

  // Define other extended data
  pXData->pBuf            = pBuf;
  pXData->nMarginX        = 5;
  pXData->nMarginY        = 5;
  pXData->bWrapEn         = true;
  pXData->nCurPosX        = 0;
  pXData->nCurPosY        = 0;

  pXData->nBufRows        = nBufRows;
  pXData->nBufCols        = nBufCols;
  pXData->nBufPosX        = 0;
  pXData->nBufPosY        = 0;
  pXData->nWndRowStart    = 0;


  // Clear the buffer
  memset(pBuf,0,nBufRows*nBufCols*sizeof(char));

  // Precalculate certain parameters
  // Determine the maximum size of a character
  // - For now, assume we are using a monospaced font and derive
  //   text pixel coords from the size of a worst-case character.
  // - TODO: Update to determine worst-case character (might not be "%")
  int16_t       nChOffsetX, nChOffsetY, nChOffsetTmp;
  uint16_t      nChSzW,nChSzH,nChSzTmp;

  // Fetch X & Y sizing and offsets independently, based on characters that
  // are likely to maximize the ascenders / descenders / width attributes
  char          acMonoH[3] = "p$";
  char          acMonoW[2] = "w";

  gslc_DrvGetTxtSize(pGui, sElem.pTxtFont, (char*)&acMonoH, sElem.eTxtFlags, &nChOffsetTmp, &nChOffsetY, &nChSzTmp, &nChSzH);
  gslc_DrvGetTxtSize(pGui, sElem.pTxtFont, (char*)&acMonoW, sElem.eTxtFlags, &nChOffsetX, &nChOffsetTmp, &nChSzW, &nChSzTmp);

  pXData->nWndCols = (rElem.w - (2*pXData->nMarginX)) / nChSzW;
  pXData->nWndRows = (rElem.h - (2*pXData->nMarginY)) / nChSzH;
  
  // Adjust margin to correct for character offsets
  pXData->nMarginX -= nChOffsetX;
  pXData->nMarginY -= nChOffsetY;

  pXData->nChSizeX = nChSzW;
  pXData->nChSizeY = nChSzH;

  // Determine if scrollbar should be enabled
  if (pXData->nWndRows >= pXData->nBufRows) {
    // Disable scrollbar as the window is larger
    // than the number of rows in the buffer
    pXData->bScrollEn   = false;
    pXData->nScrollPos  = 0;
  } else {
    // Scrollbar is enabled
    pXData->bScrollEn   = true;
    pXData->nScrollPos  = 0;
  }

  sElem.pXData            = (void*)(pXData);

  // Specify the custom drawing callback
  sElem.pfuncXDraw        = &gslc_ElemXTextboxDraw;
  sElem.pfuncXTouch       = NULL;
  sElem.colElemFill       = GSLC_COL_BLACK;
  sElem.colElemFillGlow   = GSLC_COL_BLACK;
  sElem.colElemFrame      = GSLC_COL_GRAY;
  sElem.colElemFrameGlow  = GSLC_COL_WHITE;
  if (nPage != GSLC_PAGE_NONE) {
    pElemRef = gslc_ElemAdd(pGui,nPage,&sElem,GSLC_ELEMREF_DEFAULT);
    return pElemRef;
#if (GSLC_FEATURE_COMPOUND)
  } else {
    // Save as temporary element
    pGui->sElemTmp = sElem;
    pGui->sElemRefTmp.pElem = &(pGui->sElemTmp);
    pGui->sElemRefTmp.eElemFlags = GSLC_ELEMREF_DEFAULT | GSLC_ELEMREF_REDRAW_FULL;
    return &(pGui->sElemRefTmp);
#endif
  }
  return NULL;
}


void gslc_ElemXTextboxReset(gslc_tsGui* pGui,gslc_tsElemRef* pElemRef)
{
  if (pElemRef == NULL) {
    static const char GSLC_PMEM FUNCSTR[] = "ElemXTextboxReset";
    GSLC_DEBUG_PRINT_CONST(ERRSTR_NULL,FUNCSTR);
    return;
  }
  gslc_tsXTextbox*  pBox;
  gslc_tsElem*      pElem = gslc_GetElemFromRef(pGui,pElemRef);
  pBox = (gslc_tsXTextbox*)(pElem->pXData);

  // Reset the positional state
  pBox->nCurPosX        = 0;
  pBox->nCurPosY        = 0;
  pBox->nBufPosX        = 0;
  pBox->nBufPosY        = 0;
  pBox->nWndRowStart    = 0;

  // Clear the buffer
  memset(pBox->pBuf,0,pBox->nBufRows*pBox->nBufCols*sizeof(char));

  // Set the redraw flag
  // - Only need incremental redraw
  gslc_ElemSetRedraw(pGui,pElemRef,GSLC_REDRAW_INC);
}

// Advance the buffer writer to the next line
// The window is also shifted if we are eating the first row
void gslc_ElemXTextboxLineWrAdv(gslc_tsGui* pGui,gslc_tsXTextbox* pBox)
{
  pBox->nBufPosX = 0;
  pBox->nBufPosY++;

  // Wrap the pointers around end of buffer
  pBox->nBufPosY      = pBox->nBufPosY % pBox->nBufRows;

  // Did the buffer write pointer start to encroach upon
  // the visible window region? If so, shift the window
  if (pBox->nBufPosY == pBox->nWndRowStart) {
    // Advance the window (with wrap if needed)
    pBox->nWndRowStart = (pBox->nWndRowStart + 1) % pBox->nBufRows;
  }
}

void gslc_ElemXTextboxScrollSet(gslc_tsGui* pGui,gslc_tsElemRef* pElemRef,uint8_t nScrollPos,uint8_t nScrollMax)
{

  gslc_tsXTextbox*  pBox;
  gslc_tsElem*      pElem = gslc_GetElemFromRef(pGui,pElemRef);
  pBox = (gslc_tsXTextbox*)(pElem->pXData);

  // Ensure scrollbar is enabled
  if (!pBox->bScrollEn) {
    // Scrollbar is disabled, so ignore
    return;
  }

  // Assign proportional value based on visible window region
  uint16_t nScrollPosOld = pBox->nScrollPos;
  pBox->nScrollPos = nScrollPos * (pBox->nBufRows - pBox->nWndRows) / nScrollMax;

  // Set the redraw flag
  // - Only need incremental redraw
  // - Only redraw if changed actual scroll row
  if (pBox->nScrollPos != nScrollPosOld) {
    gslc_ElemSetRedraw(pGui,pElemRef,GSLC_REDRAW_INC);
  }
}

// Write a character to the buffer
// - Advance the write ptr, wrap if needed
// - If encroach upon buffer read ptr, then drop the oldest line from the buffer
// NOTE: This should not be called with newline char!
void gslc_ElemXTextboxBufAdd(gslc_tsGui* pGui,gslc_tsXTextbox* pBox,unsigned char chNew,bool bAdvance)
{
  // Ensure that we haven't gone past end of line
  // - Note that we have to leave one extra byte for the line terminator (NULL)
  if ((pBox->nBufPosX+1) >= pBox->nBufCols) {
    if (pBox->bWrapEn) {
      // Perform line wrap
      // - Force a null at the end of the current line first
      pBox->pBuf[pBox->nBufPosY * pBox->nBufCols + (pBox->nBufCols-1)] = 0;
      gslc_ElemXTextboxLineWrAdv(pGui,pBox);
    } else {
      // Ignore the write
      return;
    }
  }

  uint16_t    nBufPos = pBox->nBufPosY * pBox->nBufCols + pBox->nBufPosX;

  // Add the character
  pBox->pBuf[nBufPos] = chNew;

  // Optionally advance the pointer
  // - The only time we don't advance is if we added NULL
  //   but note that in some special commands there may be
  //   zero values added, so we still need to advance these
  if (bAdvance) {
    pBox->nBufPosX++;
  }

}

void gslc_ElemXTextboxColSet(gslc_tsGui* pGui,gslc_tsElemRef* pElemRef,gslc_tsColor nCol)
{
#if (GSLC_FEATURE_XTEXTBOX_EMBED == 0)
  GSLC_DEBUG_PRINT("ERROR: gslc_ElemXTextboxColSet() not enabled. Requires GSLC_FEATURE_XTEXTBOX_EMBED=1 %s\n","");
  return;
#else
  gslc_tsXTextbox*  pBox = NULL;
  gslc_tsElem*      pElem = gslc_GetElemFromRef(pGui,pElemRef);
  pBox = (gslc_tsXTextbox*)(pElem->pXData);

  // Ensure that there are enough free columns in current
  // buffer row to accommodate the color code (4 bytes)
  if (pBox->nBufPosX +4 >= pBox->nBufCols) {
    // Not enough space for the code, so ignore it
    GSLC_DEBUG_PRINT("ERROR: gslc_ElemXTextboxColSet() not enough cols [Pos=%u Cols=%u]\n",pBox->nBufPosX,pBox->nBufCols);
    return;
  }

  gslc_ElemXTextboxBufAdd(pGui,pBox,GSLC_XTEXTBOX_CODE_COL_SET,true);
  gslc_ElemXTextboxBufAdd(pGui,pBox,nCol.r,true);
  gslc_ElemXTextboxBufAdd(pGui,pBox,nCol.g,true);
  gslc_ElemXTextboxBufAdd(pGui,pBox,nCol.b,true);
#endif
}

void gslc_ElemXTextboxColReset(gslc_tsGui* pGui,gslc_tsElemRef* pElemRef)
{
#if (GSLC_FEATURE_XTEXTBOX_EMBED == 0)
  GSLC_DEBUG_PRINT("ERROR: gslc_ElemXTextboxColReset() not enabled. Requires GSLC_FEATURE_XTEXTBOX_EMBED=1 %s\n","");
  return;
#else
  gslc_tsXTextbox*  pBox = NULL;
  gslc_tsElem*      pElem = gslc_GetElemFromRef(pGui,pElemRef);
  pBox = (gslc_tsXTextbox*)(pElem->pXData);
  gslc_ElemXTextboxBufAdd(pGui,pBox,GSLC_XTEXTBOX_CODE_COL_RESET,true);
#endif
}

void gslc_ElemXTextboxWrapSet(gslc_tsGui* pGui,gslc_tsElemRef* pElemRef,bool bWrapEn)
{
  gslc_tsXTextbox*  pBox = NULL;
  gslc_tsElem*      pElem = gslc_GetElemFromRef(pGui,pElemRef);
  pBox = (gslc_tsXTextbox*)(pElem->pXData);
  pBox->bWrapEn = bWrapEn;
}


void gslc_ElemXTextboxAdd(gslc_tsGui* pGui,gslc_tsElemRef* pElemRef,char* pTxt)
{
  gslc_tsXTextbox*  pBox = NULL;
  gslc_tsElem*      pElem = gslc_GetElemFromRef(pGui,pElemRef);
  pBox = (gslc_tsXTextbox*)(pElem->pXData);

  // Warn the user about mode compatibility
#if (GSLC_FEATURE_XTEXTBOX_EMBED)
  static bool bWarned = false;  // Warn only once
  bool bEncUtf8 = ((pElem->eTxtFlags & GSLC_TXT_ENC) == GSLC_TXT_ENC_UTF8);
  if ((!bWarned) && (bEncUtf8)) {
    // Continue to render the text, but issue warning to the user
    GSLC_DEBUG_PRINT("WARNING: ElemXTextboxAdd(%s) UTF-8 encoding not supported in GSLC_FEATURE_XTEXTBOX_EMBED=1 mode\n","");
    bWarned = true;
  }
#endif

  // Add null-terminated string to the bottom of the buffer
  // If the string exceeds the buffer length then it will wrap
  // back to the beginning.
  // TODO: Ensure that buffer wrap doesn't encroach upon visible region!
  // TODO: Assert (pBox)
  bool            bDone = false;
  uint16_t        nTxtPos = 0;
  unsigned char   chNext;

  if (pTxt == NULL) { bDone = true; }
  while (!bDone) {
    chNext = pTxt[nTxtPos];
    nTxtPos++;
    if (chNext == 0) {
      // Reached terminator character
      // Add terminator to buffer but don't advance write pointer
      // since we want next write to overwrite this
      gslc_ElemXTextboxBufAdd(pGui,pBox,0,false);

      bDone = true;
      continue;
    }

    // FIXME: It is possible that the following check may no longer be
    // appropriate when using UTF-8 encoding mode.
    if (chNext == '\n') {
      // Terminate the line
      gslc_ElemXTextboxBufAdd(pGui,pBox,0,false);
      // Advance the writer by one line
      gslc_ElemXTextboxLineWrAdv(pGui,pBox);
    } else {
      // TODO: Check to see if we are in mask/truncate state
      // Note that this routine also handles line wrap
      gslc_ElemXTextboxBufAdd(pGui,pBox,chNext,true);
    }
  }

  // Set the redraw flag
  // - Only need incremental redraw
  // - TODO: Detect case of single line-update and limit redraw
  //         to line instead of redrawing entire control. Whenever
  //         the line is advanced, the full control content should
  //         be redrawn.
  gslc_ElemSetRedraw(pGui,pElemRef,GSLC_REDRAW_INC);
}

bool gslc_ElemXTextboxDraw(void* pvGui,void* pvElemRef,gslc_teRedrawType eRedraw)
{
  if ((pvGui == NULL) || (pvElemRef == NULL)) {
    static const char GSLC_PMEM FUNCSTR[] = "ElemXTextboxDraw";
    GSLC_DEBUG_PRINT_CONST(ERRSTR_NULL,FUNCSTR);
    return false;
  }
  // Typecast the parameters to match the GUI and element types
  gslc_tsGui*     pGui      = (gslc_tsGui*)(pvGui);
  gslc_tsElemRef* pElemRef  = (gslc_tsElemRef*)(pvElemRef);
  gslc_tsElem*    pElem     = gslc_GetElemFromRef(pGui,pElemRef);
  gslc_tsColor colBg = GSLC_COL_BLACK;
  // Fetch the element's extended data structure
  gslc_tsXTextbox* pBox;
  pBox = (gslc_tsXTextbox*)(pElem->pXData);
  if (pBox == NULL) {
    GSLC_DEBUG_PRINT("ERROR: ElemXTextboxDraw(%s) pXData is NULL\n","");
    return false;
  }

  bool     bGlow     = (pElem->nFeatures & GSLC_ELEM_FEA_GLOW_EN) && gslc_ElemGetGlow(pGui,pElemRef);
  bool     bFrameEn  = (pElem->nFeatures & GSLC_ELEM_FEA_FRAME_EN);

  // Draw the frame
  if (eRedraw == GSLC_REDRAW_FULL) {
    if (bFrameEn) {
      gslc_DrawFrameRect(pGui,pElem->rElem,pElem->colElemFrame);
    }
  }

  // Clear the background (inset from frame)
  gslc_tsRect rInner = gslc_ExpandRect(pElem->rElem,-1,-1);
  colBg = (bGlow)?pElem->colElemFillGlow:pElem->colElemFill;
  gslc_DrawFillRect(pGui,rInner,colBg);

  uint16_t          nBufPos = 0;

  uint16_t          nTxtPixX;
  uint16_t          nTxtPixY;
  gslc_tsColor      colTxt;
  //bool            bEncUtf8;

  // Determine what encoding method is used for text
  // Not used at the moment
  //bEncUtf8 = ((pElem->eTxtFlags & GSLC_TXT_ENC) == GSLC_TXT_ENC_UTF8);

  // Initialize color state
  colTxt = pElem->colElemText;

  // Calculate the starting row for the window
  uint16_t nWndRowStartScr = pBox->nWndRowStart;

  // Only correct for scrollbar position if enabled
  if (pBox->bScrollEn) {
    nWndRowStartScr = (pBox->nWndRowStart + pBox->nScrollPos) % pBox->nBufRows;
  }

#if (GSLC_FEATURE_XTEXTBOX_EMBED == 0)

  // Normal mode support (no embedded text color)
  // - This mode is much faster and is able to support UTF-8 text encoding

  uint8_t nCurY = 0;

  uint8_t nOutRow = 0;
  uint8_t nMaxRow = 0;

  nMaxRow = (pBox->nBufRows < pBox->nWndRows)? pBox->nBufRows : pBox->nWndRows;
  for (nOutRow=0;nOutRow<nMaxRow;nOutRow++) {

    // Calculate row offset after accounting for buffer wrap
    // and current window starting offset
    uint16_t nRowCur = nWndRowStartScr + nOutRow;
    nRowCur = nRowCur % pBox->nBufRows;

    // NOTE: At the start of buffer fill where we have
    // only written a couple rows, we don't stop reading
    // across all of the rows. We are dependent upon
    // the reset to initialize all rows with NULL terminator
    // so that we don't show garbage.

    nBufPos = nRowCur * pBox->nBufCols;

    nTxtPixX = pElem->rElem.x + pBox->nMarginX + 0 * pBox->nChSizeX;
    nTxtPixY = pElem->rElem.y + pBox->nMarginY + nCurY * pBox->nChSizeY;
    gslc_DrvDrawTxt(pGui,nTxtPixX,nTxtPixY,pElem->pTxtFont,(char*)&(pBox->pBuf[nBufPos]),pElem->eTxtFlags,colTxt,colBg);

    nCurY++;
  } // nOutRow

#else

  // Embedded color mode support
  // - This mode supports inline changing of text color
  // - However, it does not support UTF-8 character encoding
  // - It is also slower since rendering is per-character

  enum              {TBOX_NORM, TBOX_COL_SET};
  int16_t           eTBoxState = TBOX_NORM;
  uint16_t          nTBoxStateCnt = 0;

  unsigned char     chNext;
  uint8_t           nCurX = 0;
  uint8_t           nCurY = 0;

  uint8_t nOutRow = 0;
  uint8_t nOutCol = 0;
  uint8_t nMaxCol = 0;
  uint8_t nMaxRow = 0;
  bool    bRowDone = false;
  nMaxCol = (pBox->nBufCols < pBox->nWndCols)? pBox->nBufCols : pBox->nWndCols;
  nMaxRow = (pBox->nBufRows < pBox->nWndRows)? pBox->nBufRows : pBox->nWndRows;
  for (nOutRow=0;nOutRow<nMaxRow;nOutRow++) {
    bRowDone = false;
    nCurX = 0;
    for (nOutCol=0;(!bRowDone)&&(nOutCol<nMaxCol);nOutCol++) {

      // Calculate row offset after accounting for buffer wrap
      // and current window starting offset
      uint16_t nRowCur = nWndRowStartScr + nOutRow;
      nRowCur = nRowCur % pBox->nBufRows;

      // NOTE: At the start of buffer fill where we have
      // only written a couple rows, we don't stop reading
      // across all of the rows. We are dependent upon
      // the reset to initialize all rows with NULL terminator
      // so that we don't show garbage.

      nBufPos = nRowCur * pBox->nBufCols + nOutCol;
      chNext = pBox->pBuf[nBufPos];

      if (eTBoxState == TBOX_NORM) {
        if (chNext == 0) {
          // Reached early terminator
          bRowDone = true;
          continue;
        } else if (chNext == GSLC_XTEXTBOX_CODE_COL_SET) {
          // Set color (enter FSM)
          eTBoxState = TBOX_COL_SET;
          nTBoxStateCnt = 0;
        } else if (chNext == GSLC_XTEXTBOX_CODE_COL_RESET) {
          // Reset color
          colTxt = pElem->colElemText;
        } else {

          // Render the character
          // TODO: Optimize by coalescing all characters in row before calling DrvDrawTxt
          //       - Note that this would make it harder to change aspects (such as color)
          //         in mid-line.
          char  acChToDraw[2] = "";
          acChToDraw[0] = chNext;
          acChToDraw[1] = 0;
          nTxtPixX = pElem->rElem.x + pBox->nMargin + nCurX * pBox->nChSizeX;
          nTxtPixY = pElem->rElem.y + pBox->nMargin + nCurY * pBox->nChSizeY;
          gslc_DrvDrawTxt(pGui,nTxtPixX,nTxtPixY,pElem->pTxtFont,(char*)&acChToDraw,pElem->eTxtFlags,colTxt,colBg);

          nCurX++;

        }

      } else if (eTBoxState == TBOX_COL_SET) {
        nTBoxStateCnt++;
        if      (nTBoxStateCnt == 1) { colTxt.r = chNext; }
        else if (nTBoxStateCnt == 2) { colTxt.g = chNext; }
        else if (nTBoxStateCnt == 3) {
          colTxt.b = chNext;
          eTBoxState = TBOX_NORM;
        }
      } // eTBoxState

    } // nOutCol
    nCurY++;
  } // nOutRow

#endif // GSLC_FEATURE_XTEXTBOX_EMBED

  // Clear the redraw flag
  gslc_ElemSetRedraw(pGui,pElemRef,GSLC_REDRAW_NONE);

  // Mark page as needing flip
  gslc_PageFlipSet(pGui,true);

  return true;
}


// ============================================================================
