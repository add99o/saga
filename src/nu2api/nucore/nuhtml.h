#pragma once

#include "decomp.h"

extern "C" {
    // Count must not be -1; maximum must be nonzero. Arithmetic/conversions
    // and trusted title/labels follow the contracts documented below.
    void NuHtmlHBarGraph(char *title, i32 width, i32 height, i32 *values, i32 count, i32 maximum, char **labels,
                         u32 *colours, i32 colour_count);
    // Nonzero count/maximum and representable arithmetic are required.
    // Title/labels follow the same trusted-string contract as the line graph.
    void NuHtmlVBarGraph(char *title, i32 width, i32 height, i32 *values, i32 count, i32 maximum, char **labels,
                         u32 *colours, i32 colour_count);
    // Internal diagnostic HTML: title/labels must fit the 256-byte formatting
    // buffer and contain no format directives. Values include a look-ahead
    // sample after the displayed rows; even an empty graph reads values[0].
    void NuHtmlHLineGraph(char *title, i32 width, i32 height, i32 *values, i32 count, i32 maximum, char **labels);
    void NuHtmlWrite(const char *text, ...);
}

void setpoint(f32 x);
void setnextpoint(f32 x, f32 y);
i32 getnextdatapoint(f32 *value, i32 *delta);
