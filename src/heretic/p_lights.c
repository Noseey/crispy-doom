//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 1993-2008 Raven Software
// Copyright(C) 2005-2014 Simon Howard
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
#include "doomdef.h"
#include "m_random.h"
#include "p_local.h"
#include "v_video.h"

//==================================================================
//==================================================================
//
//                                                      BROKEN LIGHT FLASHING
//
//==================================================================
//==================================================================

//==================================================================
//
//      T_LightFlash
//
//      After the map has been loaded, scan each sector for specials
//      that spawn thinkers
//
//==================================================================
void T_LightFlash(thinker_t *thinker)
{
    lightflash_t *flash = (lightflash_t *) thinker;

    if (--flash->count) 
        return;

    if (flash->toggle)
    {
        if(!crispy->strobelights)
            flash->sector->lightlevel = flash->maxlight;
        else
            flash->sector->lightlevel = flash->minlight;

        flash->toggle = 0;
        flash->count = (P_Random() & flash->mintime) + 1;
    }
    else
    {
        flash->sector->lightlevel = flash->maxlight;
        flash->toggle = 1;
        flash->count = (P_Random() & flash->maxtime) + 1;
    } 

}


//==================================================================
//
//      P_SpawnLightFlash
//
//      After the map has been loaded, scan each sector for specials that spawn thinkers
//
//==================================================================
void P_SpawnLightFlash(sector_t * sector)
{
    lightflash_t *flash;

    sector->special = 0;        // nothing special about it during gameplay

    flash = Z_Malloc(sizeof(*flash), PU_LEVSPEC, 0);
    P_AddThinker(&flash->thinker);
    flash->thinker.function = T_LightFlash;
    flash->sector = sector;
    flash->maxlight = sector->lightlevel;
    flash->toggle = 1; // [crispy] Start toggling with Maxlight
    flash->minlight = P_FindMinSurroundingLight(sector, sector->lightlevel);
    flash->maxtime = 64;
    flash->mintime = 7;
    flash->count = (P_Random() & flash->maxtime) + 1;
}

//==================================================================
//
//                                                      STROBE LIGHT FLASHING
//
//==================================================================

//==================================================================
//
//      T_StrobeFlash
//
//      After the map has been loaded, scan each sector for specials that spawn thinkers
//
//==================================================================
void T_StrobeFlash(thinker_t *thinker)
{
    strobe_t *flash = (strobe_t *) thinker;

    if (--flash->count)
        return;

    if (!flash->toggle)
    {
        flash->sector->lightlevel = flash->maxlight;
        flash->count = flash->brighttime;
    }
    else
    {
        if(!crispy->strobelights)
            flash->sector->lightlevel = flash->maxlight;
        else
            flash->sector->lightlevel = flash->minlight;

        flash->count = flash->darktime;
    }

}

//==================================================================
//
//      P_SpawnLightFlash
//
//      After the map has been loaded, scan each sector for specials that spawn thinkers
//
//==================================================================
void P_SpawnStrobeFlash(sector_t * sector, int fastOrSlow, int inSync)
{
    strobe_t *flash;

    flash = Z_Malloc(sizeof(*flash), PU_LEVSPEC, 0);
    P_AddThinker(&flash->thinker);
    flash->sector = sector;
    flash->darktime = fastOrSlow;
    flash->brighttime = STROBEBRIGHT;
    flash->thinker.function = T_StrobeFlash;
    flash->maxlight = sector->lightlevel;
    flash->toggle = 1; // [crispy] Start toggling with Maxlight
    flash->minlight = P_FindMinSurroundingLight(sector, sector->lightlevel);

    if (flash->minlight == flash->maxlight)
        flash->minlight = 0;
    sector->special = 0;        // nothing special about it during gameplay

    if (!inSync)
        flash->count = (P_Random() & 7) + 1;
    else
        flash->count = 1;
}

//==================================================================
//
//      Start strobing lights (usually from a trigger)
//
//==================================================================
void EV_StartLightStrobing(line_t * line)
{
    int secnum;
    sector_t *sec;

    secnum = -1;
    while ((secnum = P_FindSectorFromLineTag(line, secnum)) >= 0)
    {
        sec = &sectors[secnum];
        if (sec->specialdata)
            continue;

        P_SpawnStrobeFlash(sec, SLOWDARK, 0);
    }
}

//==================================================================
//
//      TURN LINE'S TAG LIGHTS OFF
//
//==================================================================
void EV_TurnTagLightsOff(line_t * line)
{
    int i;
    int j;
    int min;
    sector_t *sector;
    sector_t *tsec;
    line_t *templine;

    sector = sectors;
    for (j = 0; j < numsectors; j++, sector++)
        if (sector->tag == line->tag)
        {
            min = sector->lightlevel;
            for (i = 0; i < sector->linecount; i++)
            {
                templine = sector->lines[i];
                tsec = getNextSector(templine, sector);
                if (!tsec)
                    continue;
                if (tsec->lightlevel < min)
                    min = tsec->lightlevel;
            }
            sector->lightlevel = min;
        }
}

//==================================================================
//
//      TURN LINE'S TAG LIGHTS ON
//
//==================================================================
void EV_LightTurnOn(line_t * line, int bright)
{
    int i;
    int j;
    sector_t *sector;
    sector_t *temp;
    line_t *templine;

    sector = sectors;

    for (i = 0; i < numsectors; i++, sector++)
        if (sector->tag == line->tag)
        {
            //
            // bright = 0 means to search for highest
            // light level surrounding sector
            //
            if (!bright)
            {
                for (j = 0; j < sector->linecount; j++)
                {
                    templine = sector->lines[j];
                    temp = getNextSector(templine, sector);
                    if (!temp)
                        continue;
                    if (temp->lightlevel > bright)
                        bright = temp->lightlevel;
                }
            }
            sector->lightlevel = bright;
        }
}

//==================================================================
//
//      Spawn glowing light
//
//==================================================================
void T_Glow(thinker_t *thinker)
{
    glow_t *g = (glow_t *) thinker;

    switch (g->direction)
    {
        case -1:               // DOWN
            g->tmplight -= GLOWSPEED;
            if (g->tmplight <= g->minlight)
            {
                g->tmplight += GLOWSPEED;
                g->direction = 1;
            }
            break;
        case 1:                // UP
            g->tmplight += GLOWSPEED;
            if (g->tmplight >= g->maxlight)
            {
                g->tmplight -= GLOWSPEED;
                g->direction = -1;
            }
            break;
    }

    if(!crispy->strobelights)
        g->sector->lightlevel = g->maxlight;
    else
        g->sector->lightlevel = g->tmplight;

}

void P_SpawnGlowingLight(sector_t * sector)
{
    glow_t *g;

    g = Z_Malloc(sizeof(*g), PU_LEVSPEC, 0);
    P_AddThinker(&g->thinker);
    g->sector = sector;
    g->minlight = P_FindMinSurroundingLight(sector, sector->lightlevel);
    g->maxlight = sector->lightlevel;
    g->tmplight = g->maxlight; // [crispy] Start with Maxlight
    g->thinker.function = T_Glow;
    g->direction = -1;

    sector->special = 0;
}
