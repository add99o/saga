#include "decomp.h"
#include "globals.h"
#include "legoapi/legoapi_types.h"
#include "legoapi/menus/screens/movies.h"
#include "nu2api/nucore/nustring.h"
#include "nu2api/nu3d/android/nufmv_android.h"
#include "nu2api/nu3d/nutex.h"
#include "nu2api/nufile/nufpar.h"

struct AIROW_s;
struct nuqthdr_s;
struct nunativegscene_s;
struct SHOPINPUT;

extern "C" void NuSound3KillAllAudio(void);
i32 GamePads_SkipMovie();

static char (*MovieList)[32];
static i32 MovieCount;
static i32 movie_skipped;
static f32 MoviePlayTime;
static f32 MovieFrameTime;
static i32 (*MovieInputFn)();
static i32 Movie_CallBack();

__attribute__((force_align_arg_pointer)) i32 Movie_Play(char *movie, variptr_u *bufferStart, variptr_u *bufferEnd,
               float frameTime, i32 (*inputFn)(),
               float volume) {
    movie_skipped = 0;
    char moviePath[256];
    char subtitlePath[256];
    NuStrCpy(moviePath, const_cast<char *>("movies\\"));
    if (PAL != 0) {
        NuStrCat(moviePath, const_cast<char *>("pal\\"));
    } else {
        NuStrCat(moviePath, const_cast<char *>("ntsc\\"));
    }
    NuStrCat(moviePath, movie);
    NuStrCpy(subtitlePath, moviePath);
    NuStrCat(subtitlePath, const_cast<char *>(".sub"));
    NuStrCat(moviePath, const_cast<char *>(".pss"));
    if (NOSOUND == 0) {
        NuSound3KillAllAudio();
    }
    MovieInputFn = inputFn != NULL ? inputFn : GamePads_SkipMovie;
    MoviePlayTime = 0.0f;
    MovieFrameTime = frameTime;
    if (bufferStart == NULL || bufferEnd == NULL) {
        return 0;
    }
    i32 result = NuFmvPlayV(2, moviePath, 3, 4, 2, 7, Movie_CallBack, 10, 0, 14, &volume, 8, bufferStart,
                            bufferEnd->addr, 5, subtitlePath, 1);
    return result != 0 ? (movie_skipped == 1 ? 1 : 2) : 0;
}

void Movies_ConfigureList(char *path, variptr_u *buf, variptr_u *buf_end) {
    (void)buf_end;
    NUFPAR *parser = NuFParCreate(path);
    if (parser == NULL) {
        return;
    }

    MovieCount = 0;
    buf->addr = ALIGN(buf->addr, 4);
    MovieList = reinterpret_cast<char (*)[32]>(buf->addr);
    char *destination = reinterpret_cast<char *>(buf->addr);
    while (NuFParGetLine(parser) != 0) {
        NuFParGetWord(parser);
        while (NuStrICmp(parser->word_buf, const_cast<char *>("movie")) == 0) {
            if (NuFParGetWord(parser) == 0 || NuStrLen(parser->word_buf) > 31) {
                goto next_line;
            }
            NuStrCpy(destination, parser->word_buf);
            destination += 32;
            ++MovieCount;
            if (NuFParGetLine(parser) == 0) {
                goto done;
            }
            NuFParGetWord(parser);
        }
    next_line:;
    }
done:
    NuFParDestroy(parser);
    if (MovieCount == 0) {
        MovieList = NULL;
    } else {
        buf->void_ptr = destination;
    }
}

static __used__ i32 Movie_CallBack() {
    i32 skip = 0;
    if (MovieInputFn != NULL && MovieInputFn() != 0 && MoviePlayTime >= 0.2f) {
        movie_skipped = 1;
        skip = 1;
    }
    MoviePlayTime += MovieFrameTime;
    return skip;
}
