/******************************************************************************
 *                               FREXXWARE
 * ----------------------------------------------------------------------------
 *
 * Project: Frexx C Preprocessor
 * $Source: /cvsroot/silgraphite/silgraphite/src/GrCompiler/GdlPp/FPPBase.h,v $
 * $Revision: 1.2 $
 * $Date: 2003/07/17 10:16:34 $
 * $Author: mhosken $
 * $State: Exp $
 * $Locker:  $
 *
 * ----------------------------------------------------------------------------
 * $Log: FPPBase.h,v $
 * Revision 1.2  2003/07/17 10:16:34  mhosken
 * Main linux port checkin
 *
 * Revision 1.1  2003/04/21 21:24:18  wardak
 * Add files for the GDL pre-processor (gdlpp.exe).
 *
 * Revision 1.3  1993/12/06  13:51:20  start
 * A lot of new stuff (too much to mention)
 *
 * Revision 1.2  1993/11/11  07:16:39  start
 * New stuff
 *
 * Revision 1.2  1993/11/11  07:16:39  start
 * New stuff
 *
 * Revision 1.1  1993/11/03  09:15:59  start
 * Initial revision
 *
 *
 *****************************************************************************/
#ifndef FPP_BASE_H
#define FPP_BASE_H

/*
**   $Filename: libraries/FPPbase.h $
**   $Release: 1.0 $
**   $Date: 2003/07/17 10:16:34 $
**
**   (C) Copyright 1992, 1993 by FrexxWare
**       All Rights Reserved
*/

#include <exec/types.h>
#include <exec/libraries.h>

struct FPPBase {
  struct Library LibNode;
  Ubyte Flags;
  Ubyte pad;
  /* long word aligned */
  ULONG SysLib;
  ULONG DosLib;
  ULONG SegList;
};

#define FPPNAME "fpp.library"

#endif
