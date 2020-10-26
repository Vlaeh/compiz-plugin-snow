/**
 *
 * Compiz snow plugin
 *
 * snow.c
 *
 * Copyright (c) 2006 Eckhart P. <beryl@cornergraf.net>
 * Copyright (c) 2006 Brian Jørgensen <qte@fundanemt.com>
 * Maintained by Danny Baumann <maniac@opencompositing.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 **/

/*
 * Many thanks to Atie H. <atie.at.matrix@gmail.com> for providing
 * a clean plugin template
 * Also thanks to the folks from #beryl-dev, especially Quinn_Storm
 * for helping me make this possible
 */

#include <math.h>
#include "snow.h"
#include <time.h>

static CompString pname ("snow");

COMPIZ_PLUGIN_20090315 (snow, SnowPluginVTable);

#define WINDOW_TYPE_DRAW_MASK (CompWindowTypeNormalMask|CompWindowTypeDialogMask|CompWindowTypeModalDialogMask|CompWindowTypeMenuMask|CompWindowTypeUtilMask|CompWindowTypeToolbarMask|CompWindowTypeDesktopMask|CompWindowTypeFullscreenMask|CompWindowTypeUnknownMask)

/* #define DEBUG_WINDOWS */

/* --------------------  HELPER FUNCTIONS ------------------------ */

#ifdef DEBUG_WINDOWS
static inline void debugWindow(CompScreen *s, CompWindow * w, int all) {
    if ((! all) && ((! w->isMapped()) || (! w->isViewable())) && (! strstr(w->resName().c_str(), "screensaver")))
        return;
    fprintf(stderr, "New snowWindow: %s (%0x)\n", w->resName().c_str(), w->type());
    fprintf(stderr, " -- %d.%d %d.%d\n", w->outputRect().x1(), w->outputRect().y1(), w->outputRect().x2(), w->outputRect().y2());
    fprintf(stderr, " -- window_draw: %0x\n", w->type() & WINDOW_TYPE_DRAW_MASK);
    fprintf(stderr, " --       alive: %d\n", w->alive());
    fprintf(stderr, " --      mapped: %d\n", w->isMapped());
    fprintf(stderr, " --    viewable: %d\n", w->isViewable());
    fprintf(stderr, " --     focused: %d\n", w->focused());
    fprintf(stderr, " --   intersect: %d\n", w->outputRect().intersects(CompRect(0,0,s->width(),s->height())));
}
#endif

static inline bool
isForcedWindow(const CompMatch &m, const CompWindow *w)
{
	return m.evaluate(w);
    //return strstr(w->resName().c_str(), "screensaver") != NULL;
}

static inline int
getRand (int min,
     int max)
{
    return (rand() % (max - min + 1) + min);
}

static inline float
mmRand (int   min,
    int   max,
    float divisor)
{
    return ((float) getRand(min, max)) / divisor;
};

SnowTexture::SnowTexture():
    dList(0)
{
}

SnowTexture::~SnowTexture()
{
    tex.clear();
    if (dList)
        glDeleteLists(dList, 1);
}

void
SnowScreen::initiateSnowFlake (SnowFlake  *sf)
{
    /* TODO: possibly place snowflakes based on FOV, instead of a cube. */

    switch (snowDirection)
    {
    case SnowDirectionTopToBottom:
        sf->x  = mmRand (-screenBoxing, s->width() + screenBoxing, 1);
        sf->xs = mmRand (-1, 1, 500);
        sf->y  = mmRand (-300, 0, 1);
        sf->ys = mmRand (1, 3, 1);
    break;
    case SnowDirectionBottomToTop:
        sf->x  = mmRand (-screenBoxing, s->width() + screenBoxing, 1);
        sf->xs = mmRand (-1, 1, 500);
        sf->y  = mmRand (s->height(), s->height() + 300, 1);
        sf->ys = -mmRand (1, 3, 1);
    break;
    case SnowDirectionRightToLeft:
        sf->x  = mmRand (s->width(), s->width() + 300, 1);
        sf->xs = -mmRand (1, 3, 1);
        sf->y  = mmRand (-screenBoxing, s->height() + screenBoxing, 1);
        sf->ys = mmRand (-1, 1, 500);
    break;
    case SnowDirectionLeftToRight:
        sf->x  = mmRand (-300, 0, 1);
        sf->xs = mmRand (1, 3, 1);
        sf->y  = mmRand (-screenBoxing, s->height() + screenBoxing, 1);
        sf->ys = mmRand (-1, 1, 500);
    break;
    default:
    break;
    }

    sf->z  = mmRand (-screenDepth, 0.1, 5000);
    sf->zs = mmRand (-1000, 1000, 500000);
    sf->ra = mmRand (-1000, 1000, 50);
    sf->rs = mmRand (-1000, 1000, 1000);
    sf->pos.setX(floor(sf->x));
    sf->pos.setY(floor(sf->y));
}

void
SnowScreen::snowThink (SnowFlake  *sf)
{
    if (sf->y >= s->height() + screenBoxing ||
        sf->x <= -screenBoxing ||
        sf->y >= s->width() + screenBoxing ||
        sf->z <= -((float) screenDepth / 500.0) ||
        sf->z >= 1)
    {
        initiateSnowFlake (sf);
    }
    snowMove (sf);
}

void
SnowScreen::snowMove (SnowFlake   *sf)
{
    float tmp = 1.0f / (101.0f - snowSpeed);
    int   snowUpdateDelay = screenUpdateDelay;
    
    sf->x += (sf->xs * (float) snowUpdateDelay) * tmp;
    sf->y += (sf->ys * (float) snowUpdateDelay) * tmp;
    sf->z += (sf->zs * (float) snowUpdateDelay) * tmp;
    sf->ra += ((float) snowUpdateDelay) / (10.0f - sf->rs);
    sf->pos.setX(floor(sf->x));
    sf->pos.setY(floor(sf->y));
}

Bool
SnowScreen::stepSnowPositions ()
{
    if (!active)
        return TRUE;

    for (int i = 0; i < numFlakes; i++)
        snowThink(&allSnowFlakes[i]);

    switch(snowDisplayMode) 
    {
    case SnowOptions::SnowDisplayModeOnDesktop: 
    {
        foreach (CompWindow *w, s->windows())
            if ( (w->type() & CompWindowTypeDesktopMask)
                 || isForcedWindow(snowForcedMatch, w) )
            {
                SNOW_WINDOW(w);
                 if (sw != NULL) 
                    sw->cWindow->addDamage ();
                break;
            }
    }
    break;
    case SnowOptions::SnowDisplayModeOnScreen:
        cScreen->damageScreen ();
        break;
    case SnowOptions::SnowDisplayModeOverWindows:
    {
        foreach (CompWindow *w, s->windows())
        {
            if (isWindowDrawable(w))
            {
                SNOW_WINDOW(w);
                if (sw != NULL) 
                    sw->cWindow->addDamage ();
            }
        }
    }
    break;
    }
    return TRUE;
}

/* --------------------------- RENDERING ------------------------- */

void
SnowScreen::setupDisplayList ()
{
    if (! displayList)
        displayList = glGenLists (1);

    glNewList (displayList, GL_COMPILE);
    glBegin (GL_QUADS);

    glColor4f (snowColorRed, snowColorGreen, snowColorBlue, snowColorOpacity);
    glVertex3f (0, 0, -0.0);
    glColor4f (snowColorRed, snowColorGreen, snowColorBlue, snowColorOpacity);
    glVertex3f (0, snowSize, -0.0);
    glColor4f (snowColorRed, snowColorGreen, snowColorBlue, snowColorOpacity);
    glVertex3f (snowSize, snowSize, -0.0);
    glColor4f (snowColorRed, snowColorGreen, snowColorBlue, snowColorOpacity);
    glVertex3f (snowSize, 0, -0.0);

    glEnd ();
    glEndList ();
}

void
SnowScreen::beginRendering (const CompWindow *win, const CompRegion &region)
{
    if (useBlending) {
        if (! snowColorForced)
            glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable (GL_BLEND);
    }

    glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    if (displayListNeedsUpdate)
    {
        setupDisplayList ();
        displayListNeedsUpdate = false;
    }

    if (snowColorForced)
        glColor4f (snowColorRed, snowColorGreen, snowColorBlue, snowColorOpacity);

    if (snowTexturesLoaded && useTextures)
    {
        for (int j = 0; j < snowTexturesLoaded; j++)
        {
            if (snowTex[j].tex.empty())
                continue;
            GLTexture *tex = snowTex[j].tex[0];
            tex->enable (GLTexture::Good);

            for (int i = 0; i < numFlakes; i++)
            {
                SnowFlake *snowFlake = &allSnowFlakes[i];
                if (snowFlake->tex != &snowTex[j])
                    continue;

                if ((win != NULL) && (! win->borderRect().contains(snowFlake->pos)))
                    continue;

                if (! region.contains(snowFlake->pos))
                    continue;

                if (snowRotation) {
                    float x =  snowFlake->x + snowFlake->texCenterX;
                    float y = snowFlake->y + snowFlake->texCenterY;
                    glTranslatef (x, y, snowFlake->z);
                    glRotatef (snowFlake->ra, 0, 0, 1);
                    glCallList (snowTex[j].dList);
                    glRotatef (-snowFlake->ra, 0, 0, 1);
                    glTranslatef (-x, -y, -snowFlake->z);
                } else  {
                    glTranslatef (snowFlake->x, snowFlake->y, snowFlake->z);
                    glCallList (snowTex[j].dList);
                    glTranslatef (-snowFlake->x, -snowFlake->y, -snowFlake->z);
                }
            }
            tex->disable ();
        }
    }
    else
    {
        for (int i = 0; i < numFlakes; i++)
        {
            SnowFlake *snowFlake = &allSnowFlakes[i];

            if ((win != NULL) && (! win->borderRect().contains(snowFlake->pos)))
                continue;

            if (! region.contains(snowFlake->pos))
                continue;

            glTranslatef (snowFlake->x, snowFlake->y, snowFlake->z);
            if (snowRotation)
                glRotatef (snowFlake->ra, 0, 0, 1);
            glCallList (displayList);
            if (snowRotation)
                glRotatef (-snowFlake->ra, 0, 0, 1);
            glTranslatef (-snowFlake->x, -snowFlake->y, -snowFlake->z);
        }
    }

    glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    if (useBlending)
    {
        glDisable (GL_BLEND);
        glBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    }
}

/* ------------------------  WINDOW FUNCTIONS -------------------- */
bool SnowWindow::glDraw (const GLMatrix           &transform,
                         const GLWindowPaintAttrib &attrib,
                         const CompRegion          &region,
                         unsigned int              mask)
{
    bool status = gWindow->glDraw (transform, attrib, region, mask);

    switch(sScreen->snowDisplayMode)
     {
    case SnowOptions::SnowDisplayModeOnDesktop:
        if (! (Window->type() & CompWindowTypeDesktopMask))
            break;
    case SnowOptions::SnowDisplayModeOverWindows:
        if (sScreen->active
            && (sScreen->isWindowDrawable(Window))
            // try to keep performance when transparant cube is used
            && ((mask == 0) || (mask & PAINT_WINDOW_ON_TRANSFORMED_SCREEN_MASK))
            && (sScreen->cScreen->windowPaintOffset().x() == 0)
            && (sScreen->cScreen->windowPaintOffset().y() == 0)
            )
        {
            glPushMatrix ();
            glLoadMatrixf (transform.getMatrix());
            sScreen->beginRendering (Window, region);
            glPopMatrix ();
        }
        break;
    default:
        break;
    }

    return status;
}

SnowWindow::SnowWindow(CompWindow *w) :
    PluginClassHandler<SnowWindow, CompWindow> (w),
    sScreen (SnowScreen::get (::screen)),
    Window (w),
    cWindow (CompositeWindow::get (w)),
    gWindow (GLWindow::get (w))
{
    GLWindowInterface::setHandler (gWindow, false);
    SNOW_WINDOW (w);
    sw->gWindow->glDrawSetEnabled (sw, sScreen->active);
#ifdef DEBUG_WINDOWS
    debugWindow(sw->sScreen->s, w, 1);
#endif
}

/* ------------------------  SCREEN FUNCTIONS -------------------- */

bool
SnowScreen::isWindowDrawable(const CompWindow *w)
{
    const unsigned int type = w->type();
    if (! (type & WINDOW_TYPE_DRAW_MASK))
        return false;
    if ((!w->alive()) || (!w->isMapped()) || (!w->isViewable()))
        return false;
    if (! (type & CompWindowTypeDesktopMask))
    {
        if (isForcedWindow(snowForcedMatch, w))
            return true;
        if ((! snowDisplayOverFocusedWindows)
            && w->focused())
            return false;
        if ((! snowDisplayOverWindowsVieableOnAllWorkspaces)
            && w->onAllViewports ())
            return false;
        if ((! snowDisplayOverTopWindows)
            && (w->state() & CompWindowStateAboveMask))
            return false;
    }
    if (! w->outputRect().intersects(CompRect(0,0,s->width(),s->height())))
        return false;
    return true;
}



bool
SnowScreen::glPaintOutput (const GLScreenPaintAttrib &sAttrib,
                const GLMatrix            &transform,
                const CompRegion          &region,
                CompOutput                *output,
                unsigned int              mask)
{
    bool status;
    GLMatrix sTransform (transform);

    if (active && (snowDisplayMode != SnowOptions::SnowDisplayModeOnScreen))
        mask |= PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS_MASK;

    status = gScreen->glPaintOutput (sAttrib, transform, region, output, mask);

    if (active && (snowDisplayMode == SnowOptions::SnowDisplayModeOnScreen))
    {
        sTransform.toScreenSpace (output, -DEFAULT_Z_CAMERA);
        glPushMatrix ();
        glLoadMatrixf (sTransform.getMatrix ());
        beginRendering (NULL, region);
        glPopMatrix ();
    }

    return status;
}

void
SnowScreen::setSnowflakeTexture (SnowFlake  *sf)
{
    if (snowTexturesLoaded) {
        sf->tex = &snowTex[rand () % snowTexturesLoaded];
        sf->texCenterX = snowSize / 2;
        sf->texCenterY = (snowSize * sf->tex->size.height() / sf->tex->size.width()) / 2;
    }
}

void
SnowScreen::clearSnowTextures ()
{
    if (snowTex == NULL)
        return;

    delete [] snowTex;
    snowTex = NULL;
    snowTexturesLoaded = 0;
}

void
SnowScreen::updateSnowTextures ()
{
    int i, count = 0;

    clearSnowTextures();
    if (snowTexNFiles > 0)
        snowTex = new SnowTexture[snowTexNFiles];

    for (i = 0; i < snowTexNFiles; i++)
    {
        SnowTexture *sTex = &snowTex[count];

        sTex->tex =
            GLTexture::readImageToTexture (snowTexFiles[i], pname, sTex->size);
        if (sTex->tex.empty ())
        {
            compLogMessage ("snow", CompLogLevelWarn,
                            "Texture not found : %s", snowTexFiles[i].c_str());
            continue;
        }
        compLogMessage ("snow", CompLogLevelInfo,
                        "Loaded Texture %s", snowTexFiles[i].c_str());

        GLTexture::Matrix mat = sTex->tex[0]->matrix ();
        sTex->dList = glGenLists (1);
        glNewList (sTex->dList, GL_COMPILE);

        glBegin (GL_QUADS);
        glTexCoord2f (COMP_TEX_COORD_X (mat, 0), COMP_TEX_COORD_Y (mat, 0));
        glVertex2f (0, 0);
        glTexCoord2f (COMP_TEX_COORD_X (mat, 0),
                      COMP_TEX_COORD_Y (mat, sTex->size.height()));
        glVertex2f (0, snowSize * sTex->size.height() / sTex->size.width());
        glTexCoord2f (COMP_TEX_COORD_X (mat, sTex->size.width()),
                      COMP_TEX_COORD_Y (mat, sTex->size.height()));
        glVertex2f (snowSize, snowSize * sTex->size.height() / sTex->size.width());
        glTexCoord2f (COMP_TEX_COORD_X (mat, sTex->size.width()),
                      COMP_TEX_COORD_Y (mat, 0));
        glVertex2f (snowSize, 0);

        glEnd ();
        glEndList ();
        
        count++;
    }

    snowTexturesLoaded = count;

    for (i = 0; i < numFlakes; i++)
        setSnowflakeTexture (&allSnowFlakes[i]);
}

bool
SnowScreen::snowToggle() 
{
    return snowActivate (! active);
}

bool
SnowScreen::snowToggleDisplayMode() 
{
    switch(snowDisplayMode) {
    case SnowOptions::SnowDisplayModeOnScreen:
        snowDisplayMode = SnowOptions::SnowDisplayModeOnDesktop;
        break;
    case SnowOptions::SnowDisplayModeOnDesktop:
        snowDisplayMode = SnowOptions::SnowDisplayModeOverWindows;
        break;
    case SnowOptions::SnowDisplayModeOverWindows:
    default:
        snowDisplayMode = SnowOptions::SnowDisplayModeOnScreen;
        break;
    }
    return true;
}

bool
SnowScreen::snowActivate(bool status) 
{
    gScreen->glPaintOutputSetEnabled (this, status);
    foreach (CompWindow *w, s->windows ())
    {
        SNOW_WINDOW (w);
        sw->gWindow->glDrawSetEnabled (sw, status);
    }
    if (active && (!status))
        cScreen->damageScreen ();
    active = status;
    if (status)
        mTimer.start();
    else
        mTimer.stop();
    return true;
}

void
SnowScreen::optionChanged (CompOption            *options,
                           SnowOptions::Options  num)
{
    switch (num)
    {
    case SnowOptions::SnowSize:
    {
        snowSize = optionGetSnowSize();
        updateSnowTextures ();
        displayListNeedsUpdate = true;
    }
    break;
    case SnowOptions::SnowTextures:
    {
        CompOption::Value::Vector texOpt = optionGetSnowTextures ();
        snowTexNFiles = texOpt.size();
        snowTexFiles = snowTexNFiles > 0 ? new CompString[snowTexNFiles] : NULL;
        for (int i = 0; i < snowTexNFiles; i++) 
            snowTexFiles[i] = texOpt.at(i).s();

        updateSnowTextures ();
        displayListNeedsUpdate = true;
    }
    break;
    case SnowOptions::NumSnowflakes:
    {
        numFlakes = optionGetNumSnowflakes();
        int        i;

        delete [] allSnowFlakes;
        allSnowFlakes = numFlakes > 0 ? new SnowFlake[numFlakes] : NULL;

        for (i = 0; i < numFlakes; i++)
        {
            SnowFlake  *snowFlake = &allSnowFlakes[i];
            initiateSnowFlake (snowFlake);    
            setSnowflakeTexture (snowFlake);
            snowFlake++;
        }
    }
    break;
    case SnowOptions::SnowSpeed:
    {
        snowSpeed = optionGetSnowSpeed();
    }
    break;
    case SnowOptions::SnowDirection:
    {
        snowDirection = optionGetSnowDirection();
    }
    break;
    case SnowOptions::SnowUpdateDelay:
    {
        screenUpdateDelay = optionGetSnowUpdateDelay();
        mTimer.setTimes (screenUpdateDelay);
    }
    break;
    case SnowOptions::ScreenBoxing:
    {
        screenBoxing = optionGetScreenBoxing();
    }
    break;
    case SnowOptions::ScreenDepth:
    {
        screenDepth = optionGetScreenDepth();
    }
    break;
    case SnowOptions::SnowDisplayOverFocusedWindows:
    {
        snowDisplayOverFocusedWindows = optionGetSnowDisplayOverFocusedWindows();
    }
    break;
    case SnowOptions::SnowDisplayOverTopWindows:
    {
        snowDisplayOverTopWindows = optionGetSnowDisplayOverTopWindows();
    }
    break;
    case SnowOptions::SnowDisplayOverWindowsVieableOnAllWorkspaces:
    {
        snowDisplayOverWindowsVieableOnAllWorkspaces = optionGetSnowDisplayOverWindowsVieableOnAllWorkspaces();
    }
    break;
    case SnowOptions::SnowDisplayMode:
    {
        snowDisplayMode = optionGetSnowDisplayMode();
    }
    break;
    case SnowOptions::SnowRotation:
    {
        snowRotation = optionGetSnowRotation();
    }
    break;
    case SnowOptions::UseBlending:
    {
        useBlending = optionGetUseBlending();
    }
    break;
    case SnowOptions::UseTextures:
    {
        useTextures = optionGetUseTextures();
    }
    break;
    case SnowOptions::SnowColorForced:
    {
        snowColorForced = optionGetSnowColorForced();
        updateSnowTextures ();
        displayListNeedsUpdate = true;
    }
    break;
    case SnowOptions::SnowColor:
    {
        unsigned short* snowColor = optionGetSnowColor();
        snowColorRed = ((float) snowColor[0])/0xffff;
        snowColorGreen = ((float) snowColor[1])/0xffff; 
        snowColorBlue = ((float) snowColor[2])/0xffff;
        snowColorOpacity = ((float) snowColor[3])/0xffff;
    }
    break;
    case SnowOptions::SnowEnabled:
    {
        active = optionGetSnowEnabled();
        snowActivate(active);
    }
    break;
    case SnowOptions::SnowForcedOverWindow:
    {
        snowForcedMatch = optionGetSnowForcedOverWindow();
    }
    break;
    case SnowOptions::SnowToggleKey:
    case SnowOptions::SnowToggleDisplayMode:
    case SnowOptions::OptionNum:
    break;
    }
}

SnowScreen::SnowScreen (CompScreen *screen) :
    PluginClassHandler <SnowScreen, CompScreen> (screen), 
    s (screen),
    cScreen (CompositeScreen::get (screen)),
    gScreen (GLScreen::get (screen)),
    // options
    numFlakes (optionGetNumSnowflakes ()),
    snowSize (optionGetSnowSize ()),
    snowSpeed (optionGetSnowSpeed ()),
    snowDirection (optionGetSnowDirection ()),
    screenUpdateDelay (optionGetSnowUpdateDelay ()),
    screenBoxing (optionGetScreenBoxing ()),
    screenDepth (optionGetScreenDepth ()),
    snowDisplayMode (optionGetSnowDisplayMode ()),
    snowDisplayOverFocusedWindows (optionGetSnowDisplayOverFocusedWindows()),
    snowDisplayOverTopWindows (optionGetSnowDisplayOverTopWindows()),
    snowDisplayOverWindowsVieableOnAllWorkspaces (optionGetSnowDisplayOverWindowsVieableOnAllWorkspaces()),
    snowForcedMatch (optionGetSnowForcedOverWindow()),
    snowRotation (optionGetSnowRotation ()),
    useBlending (optionGetUseBlending ()),
    useTextures (optionGetUseTextures ()),
    active (optionGetSnowEnabled ()),
    snowTexNFiles (0),
    snowTexturesLoaded (0),
    snowTex(NULL),
    snowColorForced (optionGetSnowColorForced ()),
    snowColorRed (0),
    snowColorGreen (0),
    snowColorBlue (0),
    snowColorOpacity (0)
{
    // Init
    optionChanged(NULL, SnowOptions::SnowColor);

    CompOption::Value::Vector texOpt = optionGetSnowTextures ();
    snowTexNFiles = texOpt.size();
    snowTexFiles =  snowTexNFiles > 0 ? new CompString[snowTexNFiles] : NULL;
    //(CompString *) calloc(1, sizeof(CompString) * (snowTexNFiles+1));
    for (int i = 0; i < snowTexNFiles; i++)  {
        texOpt.at(i);
        snowTexFiles[i] = texOpt.at(i).s(); // to duplicate ?
    }

    displayList = 0;
    displayListNeedsUpdate = false;

    allSnowFlakes = numFlakes > 0 ? new SnowFlake[numFlakes] : NULL;
    for (int i = 0; i < numFlakes; i++)
    {
        SnowFlake  *snowFlake = &allSnowFlakes[i];
        initiateSnowFlake (snowFlake);
        setSnowflakeTexture (snowFlake);
        snowFlake++;
    }
    updateSnowTextures ();
    setupDisplayList ();

    // setup runtime evnts
    CompositeScreenInterface::setHandler (cScreen, false);
    GLScreenInterface::setHandler (gScreen, false);

    mTimer.setTimes (screenUpdateDelay);
    mTimer.setCallback (boost::bind (&SnowScreen::stepSnowPositions, this));

    // setup external bindings
    optionSetSnowEnabledNotify (boost::bind (&SnowScreen::optionChanged, this, _1, _2));
    optionSetNumSnowflakesNotify (boost::bind (&SnowScreen::optionChanged, this, _1, _2));
    optionSetSnowSizeNotify (boost::bind (&SnowScreen::optionChanged, this, _1, _2));
    optionSetSnowSpeedNotify (boost::bind (&SnowScreen::optionChanged, this, _1, _2));
    optionSetSnowDirectionNotify (boost::bind (&SnowScreen::optionChanged, this, _1, _2));
    optionSetSnowUpdateDelayNotify (boost::bind (&SnowScreen::optionChanged, this, _1, _2));
    optionSetScreenBoxingNotify (boost::bind (&SnowScreen::optionChanged, this, _1, _2));
    optionSetScreenDepthNotify (boost::bind (&SnowScreen::optionChanged, this, _1, _2));
    optionSetSnowDisplayModeNotify (boost::bind (&SnowScreen::optionChanged, this, _1, _2));
    optionSetSnowDisplayOverFocusedWindowsNotify (boost::bind (&SnowScreen::optionChanged, this, _1, _2));
    optionSetSnowDisplayOverTopWindowsNotify (boost::bind (&SnowScreen::optionChanged, this, _1, _2));
    optionSetSnowDisplayOverWindowsVieableOnAllWorkspacesNotify (boost::bind (&SnowScreen::optionChanged, this, _1, _2));
    optionSetSnowRotationNotify (boost::bind (&SnowScreen::optionChanged, this, _1, _2));
    optionSetUseBlendingNotify (boost::bind (&SnowScreen::optionChanged, this, _1, _2));
    optionSetUseTexturesNotify (boost::bind (&SnowScreen::optionChanged, this, _1, _2));
    optionSetSnowTexturesNotify(boost::bind (&SnowScreen::optionChanged, this, _1, _2));
    optionSetSnowColorNotify(boost::bind (&SnowScreen::optionChanged, this, _1, _2));
    optionSetSnowColorForcedNotify(boost::bind (&SnowScreen::optionChanged, this, _1, _2));
    optionSetSnowToggleKeyInitiate (boost::bind (&SnowScreen::snowToggle, this));
    optionSetSnowToggleDisplayModeInitiate (boost::bind (&SnowScreen::snowToggleDisplayMode, this));
    optionSetSnowForcedOverWindowNotify(boost::bind (&SnowScreen::optionChanged, this, _1, _2));

    snowActivate(active);
}

SnowScreen::~SnowScreen ()
{
    clearSnowTextures();
    delete [] allSnowFlakes;
    delete [] snowTexFiles;
    if (displayList)
        glDeleteLists (displayList, 1);
}




// Plugin part
bool
SnowPluginVTable::init ()
{
    if (CompPlugin::checkPluginABI ("core", CORE_ABIVERSION)        &&
        CompPlugin::checkPluginABI ("composite", COMPIZ_COMPOSITE_ABI)    &&
        CompPlugin::checkPluginABI ("opengl", COMPIZ_OPENGL_ABI))
        return true;

    return false;
}
