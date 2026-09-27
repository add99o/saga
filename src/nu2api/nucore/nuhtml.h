#pragma once

#include "decomp.h"

extern "C" {
    void NuHtmlHBarGraph(void);
    void NuHtmlVBarGraph(void);
    void NuHtmlHLineGraph(const char *title, i32 width, i32 height, const i32 *data, i32 row_count,
                          i32 sample_count, const char *const *labels);
}

void setpoint(f32 x);
void setnextpoint(f32 x, f32 y);
i32 getnextdatapoint(f32 *value, i32 *delta);
