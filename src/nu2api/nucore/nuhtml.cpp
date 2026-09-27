#include "nu2api/nucore/nuhtml.h"
#include "nu2api/nu3d/nudlist.h"

#include <stdio.h>
#include <string.h>

static f32 curx;
static f32 cury;
static f32 nextx;
static f32 dx;
i32 size;

extern "C" {
    void NuHtmlHBarGraph(void) {
        STUBBED();
    }

    void NuHtmlVBarGraph(const char *title, i32 width, i32 height, const i32 *values, i32 count,
                          i32 maximum, const char *const *labels, const u32 *colors, i32 color_count) {
        char text[256];
        const i32 bar_width = static_cast<i32>(static_cast<f32>(width - 50) / static_cast<f32>(count));
        const f32 scale = 80.0f / static_cast<f32>(maximum);

        sprintf(text, "<TABLE bgColor=white height=16 width=%d cellSpacing=0 cellPadding=\"1\" border=1><TR><TD><Center>%s</Center></TD></TR></TABLE>\r\n",
                width + 5, title);
        NuHtmlWrite(text);
        sprintf(text, "<TABLE bgColor=white height=%d width=%d cellSpacing=0 cellPadding=\"1\" border=1 VAlign=bottom><TR><TD>\r\n",
                height, width);
        NuHtmlWrite(text);
        sprintf(text, "<TABLE height=\"100%%%%\" width=%d cellSpacing=1 cellPadding=0 border=0 Align=left>\r\n", 50);
        NuHtmlWrite(text);

        const i32 quarter = maximum / 4;
        for (i32 tick = 0; tick < 4; ++tick) {
            sprintf(text, "<TR><TD valign=top align=right Height=\"%d%%%%\" bgColor=gray><DIV style=\"FONT-SIZE: 10pt\">%d</TD></TR>\r\n",
                    20, maximum - quarter * tick);
            NuHtmlWrite(text);
        }
        sprintf(text, "<TR><TD valign=top align=right Height=\"%d%%%%\" bgColor=gray><DIV style=\"FONT-SIZE: 10pt\">%d</TD></TR>\r\n",
                20, 0);
        NuHtmlWrite(text);
        NuHtmlWrite("</CENTER></TABLE>\r\n");

        i32 color_index = 0;
        for (i32 index = 0; index < count; ++index) {
            const i32 bar_height = static_cast<i32>(static_cast<f32>(values[index]) * scale);
            const u32 color = colors != NULL && color_count != 0 ? colors[color_index] : 0;
            if (colors != NULL && color_count != 0 && ++color_index >= color_count)
                color_index = 0;
            const char *label = labels != NULL && labels[index] != NULL ? labels[index] : " ";

            sprintf(text, "<TABLE height=\"100%%%%\" width=%d cellSpacing=1 cellPadding=0 border=0 Align=left><TR><TD></TD></TR>\r\n",
                    bar_width);
            NuHtmlWrite(text);
            sprintf(text, "<TR><TD Height=\"%d%%%%\" bgColor=%06X></TD></TR>\r\n",
                    bar_height <= 80 ? bar_height : 80, color);
            NuHtmlWrite(text);
            sprintf(text, "<TR><TD Height=\"%d%%%%\" bgColor=lightgrey><DIV style=\"FONT-SIZE: 10pt; WIDTH: 10pt; WRITING-MODE: tb-rl\">%s</DIV></TD></TR></TABLE>\r\n",
                    20, label);
            NuHtmlWrite(text);
        }
        NuHtmlWrite("</TABLE></TD></tr></TABLE>\r\n");
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
    void NuHtmlHLineGraph(const char *title, i32 width, i32 height, const i32 *data, i32 row_count,
                          i32 sample_count, const char *const *labels) {
        char text[256];
        static const char *axis_cell =
            "<TD width=\"%d%%%%\" bgColor=gray Align=right><DIV style=\"FONT-SIZE: 10pt; WIDTH: 10pt;\">%d</DIV></TD>\r\n";
        static const char *graph_cell =
            "  <TABLE width=\"100%%%%\" BORDER=0 CELLPADDING=0 CELLSPACING=0> <TD width=%d height=2 bgColor=white></TD> <TD width=%d bgColor=black> </TD> <TD width=%d  bgColor=white> </TD></TABLE>\r\n";

        sprintf(text, "<TABLE bgColor=white height=16 width=%d cellSpacing=0 cellPadding=\"1\" border=1><TR><TD><Center>%s</Center></TD></TR></TABLE>\r\n",
                width, title);
        NuHtmlWrite(text);
        sprintf(text, "<TABLE bgColor=white height=%d width=%d cellSpacing=0 cellPadding=0 border=1><tr><TD VAlign=top>\r\n",
                height, width);
        NuHtmlWrite(text);
        sprintf(text, "<TABLE height=10 width=\"100%%%%\" cellSpacing=0 cellPadding=0 border=0 >\r\n");
        NuHtmlWrite(text);

        const i32 graph_width = static_cast<i32>(static_cast<f32>(width) * 0.88f * 0.25f);
        const i32 axis_width = static_cast<i32>(static_cast<f32>(width) * 0.12f);
        const i32 label_width = static_cast<i32>(static_cast<f32>(graph_width) / 600.0f * 100.0f);
        const f32 scale = static_cast<f32>(graph_width * 4) / static_cast<f32>(sample_count);
        sprintf(text, axis_cell, 12, 0);
        NuHtmlWrite(text);

        setpoint(static_cast<f32>(data[0]) * scale);
        setnextpoint(static_cast<f32>(data[0]) * scale, 8.0f);
        const i32 quarter = sample_count / 4;
        for (i32 tick = 1; tick <= 4; ++tick) {
            sprintf(text, axis_cell, label_width, quarter * tick);
            NuHtmlWrite(text);
        }
        strcpy(text, "</TD></tr></TABLE>");
        NuHtmlWrite(text);

        i32 point = 1;
        i32 row = 0;
        while (row < row_count) {
            const char *label = labels != NULL && labels[row] != NULL ? labels[row] : " ";
            sprintf(text, "<TABLE  BORDER=0 CELLPADDING=0 CELLSPACING=0><TD width=\"12%%%%\" height=30 bgColor=lightgrey> <DIV style=\"FONT-SIZE: 10pt\">%s</TD>\r\n",
                    label);
            NuHtmlWrite(text);
            sprintf(text, "<TD width=\"88%%%%\" bgColor=green>\r\n");
            NuHtmlWrite(text);
            ++row;

            i32 remaining = 16;
            do {
                f32 value;
                i32 delta;
                if (getnextdatapoint(&value, &delta) < 0) {
                    setnextpoint(static_cast<f32>(data[point++]) * scale, 17.0f);
                    getnextdatapoint(&value, &delta);
                    if (row == row_count)
                        size = 0;
                }
                if (delta < 0) {
                    value += static_cast<f32>(delta);
                    delta = -delta;
                }
                if (delta == 0)
                    delta = 1;
                sprintf(text, graph_cell, static_cast<i32>(value), delta,
                        width - 4 - axis_width - static_cast<i32>(value) - delta);
                NuHtmlWrite(text);
            } while (--remaining != 0);
            strcpy(text, "<TR></TD> </TD></TABLE> \r\n\r\n");
            NuHtmlWrite(text);
        }
        strcpy(text, "</TABLE>");
        NuHtmlWrite(text);
    }
}
