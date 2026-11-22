// InitScript.h — preloaded first-boot provisioning. The build embeds an
// initial command list (see scripts/gen_init_cmds.py: init_cmds.txt ->
// include/init_cmds_data.h, INIT_CMDS_DATA, empty when no init_cmds.txt is
// present). On the first loop pass the list runs once through the same
// serial-privilege CLI dispatch the fleet scripts use. The list is divided
// into sections by `#! <name>` headers closed by `#! end`; /init_cmds.done
// stores the name of the last completed section, so on later boots the list
// is read up to that delimiter and continues from there — appended sections
// run once, finished ones never re-run. A list with no sections is one
// implicit section (sentinel name "all"). Fork-owned component; see
// INIT_SCRIPT.md for format, semantics and the trust model.
//
// Trust model: the command list is part of the firmware image itself. It is
// authored in the repo at build time and cannot be influenced at runtime.
//
// Memory discipline: one fixed line buffer + one reply buffer, both static
// (the Arduino loop stack overflowed at 8 KB in load(); do not stack
// allocate). Lines are applied and discarded as they are read from the
// flash-resident string — nothing accumulates, because state lands in the
// config stores, not here.
//
// Relationship to /filter_cfg: the filter loads its own binary config from
// flash in its constructor. Do not keep filter rules in BOTH places — two
// sources of truth for one filter is how duplicate rules happen.

#ifndef _INIT_SCRIPT_H
#define _INIT_SCRIPT_H

#include <Arduino.h>
#include <helpers/IdentityStore.h>   // FILESYSTEM typedef

class MyMesh;

// Runs the embedded INIT_CMDS_DATA once per boot if present, resuming after
// the last completed section (version-name resume via /init_cmds.done). Safe
// to call from loop().
void runInitScript(MyMesh& mesh, FILESYSTEM* fs);

#endif
