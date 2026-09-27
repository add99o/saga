#include "nu2api/nucore/nuhtml.h"

#include <stdio.h>

static f32 curx;
static f32 cury;
static f32 nextx;
static f32 dx;
i32 size;

extern "C" {
    void NuHtmlHBarGraph(void) {
        STUBBED();
    }

    void NuHtmlVBarGraph(void) {
        STUBBED();
    }
}

void setpoint(f32 x) {
    curx = x;
    cury = 0.0f;
}

void setnextpoint(f32 x, f32 y) {
    nextx = x;
    dx = (x - curx) / y;
    size = static_cast<i32>(y);
}

i32 getnextdatapoint(f32 *value, i32 *delta) {
    const f32 next_y = cury + 1.0f;
    const f32 old_x = curx;
    *value = old_x;
    const i32 old_value = static_cast<i32>(old_x);
    cury = next_y;
    --size;
    const f32 next_x = old_x + dx;
    curx = next_x;
    *delta = static_cast<i32>(next_x) - old_value;
    if (size != 0) {
        return 0;
    }
    curx = nextx;
    return -1;
}

extern "C" {
    void NuHtmlHLineGraph(char *title, i32 width, i32 height, i32 *values, i32 count, i32 maximum, char **labels) {
        char text[256];
        sprintf(text,
                "<TABLE bgColor=white height=16 width=%d cellSpacing=0 cellPadding=\"1\" border=1>"
                "<TR><TD><Center>%s</Center></TD></TR></TABLE>\r\n",
                width, title);
        NuHtmlWrite(text);
        sprintf(text,
                "<TABLE bgColor=white height=%d width=%d cellSpacing=0 cellPadding=0 border=1>"
                "<tr><TD VAlign=top>\r\n",
                height, width);
        NuHtmlWrite(text);
        sprintf(text, "<TABLE height=10 width=\"100%%%%\" cellSpacing=0 cellPadding=0 border=0 >\r\n");
        NuHtmlWrite(text);

        i32 quarter_width = static_cast<i32>(static_cast<f32>(width) * 0.88f * 0.25f);
        i32 label_width = static_cast<i32>(static_cast<f32>(width) * 0.12f);
        i32 tick_width = static_cast<i32>(static_cast<f32>(quarter_width) / 600.0f * 100.0f);
        f32 scale = static_cast<f32>(quarter_width * 4) / static_cast<f32>(maximum);
        const char *tick = "<TD width=\"%d%%%%\" bgColor=gray Align=right>"
                           "<DIV style=\"FONT-SIZE: 10pt; WIDTH: 10pt;\">%d</DIV></TD>\r\n";
        sprintf(text, tick, 12, 0);
        NuHtmlWrite(text);
        setpoint(static_cast<f32>(values[0]) * scale);
        setnextpoint(static_cast<f32>(values[0]) * scale, 8.0f);
        i32 quarter_maximum = maximum / 4;
        sprintf(text, tick, tick_width, quarter_maximum);
        NuHtmlWrite(text);
        sprintf(text, tick, tick_width, quarter_maximum * 2);
        NuHtmlWrite(text);
        sprintf(text, tick, tick_width, quarter_maximum * 3);
        NuHtmlWrite(text);
        sprintf(text, tick, tick_width, quarter_maximum * 4);
        NuHtmlWrite(text);
        sprintf(text, "</TD></tr></TABLE>");
        NuHtmlWrite(text);

        i32 next_value = 1;
        for (i32 row = 0; row < count; ++row) {
            char *label = labels != NULL ? labels[row] : NULL;
            sprintf(text,
                    "<TABLE  BORDER=0 CELLPADDING=0 CELLSPACING=0><TD width=\"12%%%%\" "
                    "height=30 bgColor=lightgrey> <DIV style=\"FONT-SIZE: 10pt\">%s</TD>\r\n",
                    label != NULL ? label : " ");
            NuHtmlWrite(text);
            sprintf(text, "<TD width=\"88%%%%\" bgColor=green>\r\n");
            NuHtmlWrite(text);
            for (i32 line = 0; line < 16; ++line) {
                f32 value;
                i32 delta;
                if (getnextdatapoint(&value, &delta) < 0) {
                    setnextpoint(static_cast<f32>(values[next_value++]) * scale, 17.0f);
                    getnextdatapoint(&value, &delta);
                    if (row + 1 == count) {
                        dx = 0.0f;
                    }
                }
                if (delta < 0) {
                    value += static_cast<f32>(delta);
                    delta = -delta;
                } else if (delta == 0) {
                    delta = 1;
                }
                i32 before = static_cast<i32>(value);
                i32 after = width - (label_width + before) - delta - 4;
                sprintf(text,
                        "  <TABLE width=\"100%%%%\" BORDER=0 CELLPADDING=0 CELLSPACING=0> "
                        "<TD width=%d height=2 bgColor=white></TD> <TD width=%d bgColor=black> </TD> "
                        "<TD width=%d  bgColor=white> </TD></TABLE>\r\n",
                        before, delta, after);
                NuHtmlWrite(text);
            }
            sprintf(text, "<TR></TD> </TD></TABLE> \r\n\r\n");
            NuHtmlWrite(text);
        }
        sprintf(text, "</TABLE>");
        NuHtmlWrite(text);
    }
}
