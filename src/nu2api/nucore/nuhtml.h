#pragma once

#include "decomp.h"

extern "C" {
    void NuHtmlHBarGraph(void);
    void NuHtmlVBarGraph(void);
    // Internal diagnostic HTML: title/labels must fit the 256-byte formatting
    // buffer and contain no format directives. Values include a look-ahead
    // sample after the displayed rows; even an empty graph reads values[0].
    void NuHtmlHLineGraph(char *title, i32 width, i32 height, i32 *values, i32 count, i32 maximum, char **labels);
    void NuHtmlWrite(const char *text, ...);
}

void setpoint(f32 x);
void setnextpoint(f32 x, f32 y);
i32 getnextdatapoint(f32 *value, i32 *delta);
