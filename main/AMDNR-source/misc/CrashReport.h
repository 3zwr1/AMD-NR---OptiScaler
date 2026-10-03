// Copyright (c) 2026 3zwr1 (AMDNR). Part of AMDNR (GPL-3.0; see Licenses/AMDNR_NOTICE.txt).
#pragma once
// In-process crash note ([Log] CrashHandler, off by default; [Log] CrashDump adds a small minidump). Passive: it
// records and passes the exception on (chained), filters to faults worth a note, and allocates nothing inside the
// handler (files and buffers are prepared at Install). Only dllmain.cpp calls it, and only when CrashHandler=true.
// Output: amdnr_crash.log beside OptiScaler.log (one START line per process, capped EXCEPTION lines with a short
// stack, one UNHANDLED line, END on a clean exit); with CrashDump, one AMDNR-crash-<exe>-<time>.dmp per process
// (the newest 3 are kept). The "Save report" zip collects amdnr_crash.log (misc/AmdnrReport.cpp).
//
// Signatures fixed by W0-2 (a change is a gate request); bodies W1-F (CrashReport.cpp).

namespace CrashReport
{
// Installs the handler. `withDump` = [Log] CrashDump. False when it could not be installed (nothing changed then).
bool Install(bool withDump);

// Removes what Install added; safe when Install never ran or failed. DLL_PROCESS_DETACH, before the logger closes.
void Uninstall();
} // namespace CrashReport
