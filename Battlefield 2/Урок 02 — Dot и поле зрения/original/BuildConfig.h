// [CHOSEN, not recovered] Compile-time image contract; force-include this header in engine TUs.
// Define dedicated explicitly as 1 for the dedicated server or 0 for the client and client DLLs.
// Client-only names belong under #if !dedicated; runtime isDedicated() branches remain runtime
// branches in the client image. There is deliberately no fallback image define.

#ifndef CORE_BUILDCONFIG_H          // [CHOSEN] guard style, matching the tree's other Core headers
#define CORE_BUILDCONFIG_H

#if !defined(DEDICATED)
#  error "DEDICATED is not defined. Every compile of Code/BF2 must pass /DDEDICATED=1 (bf2_w32ded.exe and every server-side gate) or /DDEDICATED=0 (BF2.exe and the client-side DLL images). See docs/building.md."
#elif (DEDICATED != 0) && (DEDICATED != 1)
#  error "DEDICATED must be 1 (the dedicated server image) or 0 (the client image)."
#endif

#endif // CORE_BUILDCONFIG_H
