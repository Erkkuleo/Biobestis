#pragma once
#include <pgmspace.h>

#define EYE_W 128
#define EYE_H 32

#include "heartEyes.h"    // level 0 - Great!
#include "happyEyes.h"     // level 1 - Good
#include "hoodedEyes.h"   // level 2 - OK
#include "sadEyes.h"   // level 3 - Meh
#include "cryingEyes.h"      // level 4 - Bad
#include "angryEyes.h"    // level 5 - Critical!

static const unsigned char* const statusBitmaps[6] PROGMEM = {
    heartEyesBitmap,
    happyEyesBitmap,
    hoodedEyesBitmap,
    sadEyesBitmap,
    cryingEyesBitmap,
    angryEyesBitmap
};
