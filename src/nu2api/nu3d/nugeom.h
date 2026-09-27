#pragma once

// Recovered prefix of the legacy geometry record. Its remaining fields have
// not been reconstructed; consumers here only traverse the linked list.
struct nugeom_s {
    nugeom_s *next;
};
