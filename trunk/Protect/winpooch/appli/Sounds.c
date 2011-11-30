/******************************************************************/
/*                                                                */
/*  Winpooch : Windows Watchdog                                   */
/*  Copyright (C) 2004-2006  Benoit Blanchon                      */
/*                                                                */
/*  This program is free software; you can redistribute it        */
/*  and/or modify it under the terms of the GNU General Public    */
/*  License as published by the Free Software Foundation; either  */
/*  version 2 of the License, or (at your option) any later       */
/*  version.                                                      */
/*                                                                */
/*  This program is distributed in the hope that it will be       */
/*  useful, but WITHOUT ANY WARRANTY; without even the implied    */
/*  warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR       */
/*  PURPOSE.  See the GNU General Public License for more         */
/*  details.                                                      */
/*                                                                */
/*  You should have received a copy of the GNU General Public     */
/*  License along with this program; if not, write to the Free    */
/*  Software Foundation, Inc.,                                    */
/*  675 Mass Ave, Cambridge, MA 02139, USA.                       */
/*                                                                */
/******************************************************************/

/******************************************************************/
/* Includes                                                       */
/******************************************************************/

// module's interface
#include "Sounds.h"

// project's headers
#include "Config.h"
#include "Resources.h"


/******************************************************************/
/* Internal data types                                            */
/******************************************************************/

typedef struct {
  HINSTANCE	hInstance ;
  BOOL		bEnableSounds ;
  BOOL		bUseDefaultSounds ;
  TCHAR		aFiles[_SOUND_COUNT][MAX_PATH] ;
  INT		aResources[_SOUND_COUNT] ;
} INTERNALDATA ;


/******************************************************************/
/* Internal data                                                  */
/******************************************************************/

static INTERNALDATA g_data ;


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

BOOL Sounds_Init (HINSTANCE hInstance)
{
  g_data.hInstance = hInstance ;
  
  g_data.aResources[SOUND_ALERT] = IDW_BARK ;
  g_data.aResources[SOUND_ASK] = IDW_BARK ;
  g_data.aResources[SOUND_VIRUS] = IDW_BARK ;

  Sounds_ReloadConfig () ;

  return TRUE ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

BOOL Sounds_Uninit ()
{

  return TRUE ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

BOOL Sounds_ReloadConfig () 
{
  INT		iSoundConfig = Config_GetInteger(CFGINT_SOUND) ;
  LPCTSTR	szFile ;

  g_data.bEnableSounds = iSoundConfig > 0 ;
  
  g_data.bUseDefaultSounds = iSoundConfig == 1 ;
  
  szFile = Config_GetString(CFGSTR_ALERT_SOUND) ;
  if( szFile != NULL )
    wcscpy (g_data.aFiles[SOUND_ALERT], szFile) ;
  else
    g_data.aFiles[SOUND_ALERT][0] = 0 ;

  szFile = Config_GetString(CFGSTR_ASK_SOUND) ;
  if( szFile != NULL )
    wcscpy (g_data.aFiles[SOUND_ASK], szFile) ;
  else
    g_data.aFiles[SOUND_ASK][0] = 0 ;

  szFile = Config_GetString(CFGSTR_VIRUS_SOUND);
  if( szFile != NULL )
    wcscpy (g_data.aFiles[SOUND_VIRUS], szFile) ;
  else
    g_data.aFiles[SOUND_VIRUS][0] = 0 ;

  return TRUE ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

BOOL Sounds_Play (UINT nSound)
{
  if( ! g_data.bEnableSounds ) return TRUE ;

  if( nSound >= _SOUND_COUNT ) return FALSE ;

  if( g_data.bUseDefaultSounds )
    PlaySound (MAKEINTRESOURCE(g_data.aResources[nSound]), 
	       g_data.hInstance, SND_ASYNC|SND_RESOURCE) ;
  else if( g_data.aFiles[nSound][0] ) 
    PlaySound (g_data.aFiles[nSound], NULL, SND_ASYNC|SND_FILENAME) ;

  return TRUE ;
}
