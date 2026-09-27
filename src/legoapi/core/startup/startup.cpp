#include "decomp.h"
#include "legoapi/legoapi_types.h"
#include "nu2api/nu3d/nutex.h"
#include "nu2api/nucore/nuapi.h"
#include "nu2api/nucore/nustring.h"

struct AIROW_s;
struct nuqthdr_s;
struct nunativegscene_s;
struct SHOPINPUT;

static i32 argc;
static char **argv;

void ParseCommandLine() {
    while (argc > 0) {
        if (*argv == NULL)
            return;
        if (NuStrICmp(*argv, "PADRECORD") == 0) {
            ++argv;
            --argc;
            nuapi.pad_record.filepath = *argv;
            nuapi.pad_record.mode = NUPAD_RECORD;
        } else if (NuStrICmp(*argv, "PADPLAY") == 0) {
            ++argv;
            --argc;
            nuapi.pad_record.filepath = *argv;
            nuapi.pad_record.mode = NUPAD_PLAY;
        }
        ++argv;
        --argc;
    }
}
