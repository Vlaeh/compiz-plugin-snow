#include <composite/composite.h>
#include <core/core.h>
#include <opengl/opengl.h>

#include "snow_options.h"


class SnowTexture
{
public:
    SnowTexture();
    ~SnowTexture();
    GLTexture::List tex;
    CompSize size;
    GLuint dList;
};

class SnowFlake
{
public:
    float x, y, z;
    float xs, ys, zs;
    float ra; /* rotation angle */
    float rs; /* rotation speed */
    SnowTexture *tex;
    float texCenterX;
    float texCenterY;
    CompPoint pos;
};

class SnowScreen:
    public PluginClassHandler <SnowScreen, CompScreen>,
    public CompositeScreenInterface,
    public GLScreenInterface,
    public SnowOptions
{
public:
    SnowScreen (CompScreen *);
    ~SnowScreen ();

    CompScreen      *s;
    CompositeScreen *cScreen;
    GLScreen        *gScreen;

    /**
     * Composite Screen Interface
     */

    /**
     * GL Screen Interface
     */
    bool glPaintOutput (const GLScreenPaintAttrib &,
                        const GLMatrix &, const CompRegion &,
                        CompOutput *, unsigned int);

    /**
     *  Options Interface
     */
    int             numFlakes;
    float           snowSize;
    int             snowSpeed;
    int             snowDirection;
    int             screenUpdateDelay;
    int             screenBoxing;
    int             screenDepth;
    int             snowDisplayMode;
    Bool            snowDisplayOverFocusedWindows;
    Bool            snowDisplayOverTopWindows;
    Bool            snowDisplayOverWindowsVieableOnAllWorkspaces;
    CompMatch       snowForcedMatch;
    Bool            snowRotation;
    Bool            useBlending;
    Bool            useTextures;
    Bool            active;

    void optionChanged (CompOption                *options,
                        SnowOptions::Options num);
    void beginRendering (const CompWindow *w, const CompRegion &r);
    bool isWindowDrawable (const CompWindow *w);

private:
    bool snowToggle ();
    bool snowToggleDisplayMode ();
    bool snowActivate (bool state);
    void setupDisplayList ();
    void initiateSnowFlake (SnowFlake  *sf);
    void setSnowflakeTexture (SnowFlake  *sf);
    Bool stepSnowPositions ();
    void snowThink (SnowFlake  *sf);
    void snowMove (SnowFlake   *sf);
    void clearSnowTextures ();
    void updateSnowTextures ();
    /*
     * Runtime
     */
    bool              displayListNeedsUpdate;
    GLuint            displayList;
    int               snowTexNFiles;
    CompString        *snowTexFiles;
    int               snowTexturesLoaded;
    SnowTexture       *snowTex;
    SnowFlake         *allSnowFlakes;
    CompTimer         mTimer;
    bool              snowColorForced;
    float             snowColorRed;
    float             snowColorGreen;
    float             snowColorBlue;
    float             snowColorOpacity;
};

class SnowWindow:
    public PluginClassHandler <SnowWindow, CompWindow>,
    public GLWindowInterface
{
public:
    SnowWindow (CompWindow *window);

    SnowScreen      *sScreen;
    CompWindow      *Window;
    CompositeWindow *cWindow;
    GLWindow        *gWindow;

    // GLWindowInterface methods
    bool glDraw (const GLMatrix           &transform,
                 const GLWindowPaintAttrib &attrib,
                 const CompRegion          &region,
                 unsigned int              mask);
};


class SnowPluginVTable:
public CompPlugin::VTableForScreenAndWindow<SnowScreen, SnowWindow>
{
public:
    bool init ();
};

#define SNOW_SCREEN(s)                                \
    SnowScreen *ms = SnowScreen::get (s)
#define SNOW_WINDOW(w)                                \
    SnowWindow *sw = SnowWindow::get (w)

