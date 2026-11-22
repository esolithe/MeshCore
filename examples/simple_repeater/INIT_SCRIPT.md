# First-boot provisioning (`init_cmds.txt` → embedded command list)

The firmware can carry a **preloaded initial command list**: put an
`init_cmds.txt` in the project root before building, and the build embeds it
(generated into `include/init_cmds_data.h` by `scripts/gen_init_cmds.py`, a
pre-build hook on `[arduino_base]`). On the node's first boot the list runs
once through the same serial-privilege CLI dispatch the fleet scripts use —
no serial console, no companion app, no data image, no second build step.

The list is organized into **versioned sections**. A section starts with a
`#! <name>` header and ends with a `#! end` line; the node records the name
of the last completed section in `/init_cmds.done`. On later boots the
embedded list is read up to that point and **only newer sections run** — so
growing the list over time (new channels, new filters) never re-applies old
commands.

```text
#! setup-base
# provisioning for repeater-42
set adm.pass "s3cret-hunter2"
set node.lat 41.2
set node.lon -73.9
#! end

#! channels-v2
filter add chan=#wardriving hops=[2,*]
filter on
filter ratelimit advert 48
#! end
```

Later, ship a firmware with the list **extended** (never rewrite old
sections — they will not re-run):

```text
# ... setup-base and channels-v2 unchanged ...
#! channels-v3-east
filter add chan=#east hops=[2,*]
#! end
```

Blank lines and lines starting with `#` (but not `#!`) are comments. Values
with spaces go in double quotes, exactly as at the console. There is no
pattern or wildcard expansion — what you write is what the dispatcher gets.

## Semantics

| Aspect | Behavior |
|---|---|
| When | First `loop()` pass after boot — after all components (filter, fleet, clock sync) constructed, same deferred context the fleet scripts run in |
| Privilege | Serial-console privilege: `handleCommand(0, ...)` — the same dispatch `execCliLine` uses for fleet scripts |
| **Once** | `/init_cmds.done` stores the **name of the last completed section**. On boot the embedded list is read to that delimiter and execution continues from the next section: finished sections never re-run, appended ones run exactly once |
| Fresh node | No marker file (or empty): everything runs — any lines before the first `#!` header included |
| Section names | Must be unique; the marker matches by exact name. If the marker names a section that no longer exists, the list was rewritten wholesale and **nothing runs** (log a warning; add a new `#!` section to apply anything) |
| Errors | **Stop on first error.** The CLI refuses invalid input whole (no half-applied command). The section that errored is recorded as completed anyway — a half-configured node never boot-loops or retries; fix the list by appending a new section and reflash |
| No sections | A list without any `#!` headers is one implicit section: it runs once on a fresh node, then never again (recorded under the sentinel name `all`) |
| Truncation | A final section missing its `#! end` marker was never completed: its commands run but it is not recorded, so it retries on the next boot |
| Trailing lines | Command lines after the final `#! end` are ignored (they have no section) |
| Memory | One fixed 196-byte line buffer + one reply buffer, both static; lines are applied and discarded as read from the flash-resident string. Over-long lines are refused whole (the CLI cap is ~180 chars) |
| Logging | Each boot's run is visible on the serial console; the `filter`/fleet counters reflect what the commands did |

## Build integration

```bash
# project root
edit init_cmds.txt
FIRMWARE_VERSION=v1.0.0 sh build.sh build-firmware heltec_v4_repeater_observer
```

The pre-build hook regenerates the header on every build; with no
`init_cmds.txt` present it generates an empty list and the hook is a no-op,
so one firmware tree serves nodes with and without provisioning.

Per-node identity that is compile-time by nature (advert name, admin
password, lat/lon `-D` defaults) stays in `platformio.local.ini` or per-node
env overrides — runtime-settable state (channels, filter rules, ratelimit,
clock, region defs) belongs in `init_cmds.txt`.

## Per-area builds (overlaying `init_cmds_<Area>.txt`)

One provisioning list per area, selected at build time — the firmware is
unchanged; only the embedded file differs.

- **pio env**: The hook embeds `init_cmds_<Area>.txt` from the project root. Build with
  `sh build.sh build-firmware heltec_v4_repeater_observer_manchester`.
- **OS env**: `INIT_CMDS_AREA=Manchester sh build.sh build-firmware <target>`
  selects the file for any target without a dedicated env.
- **Resolution order**: `custom_init_cmds_file` option →
  `custom_init_cmds_area` option → `INIT_CMDS_AREA` env var → default
  `init_cmds.txt`. A selected area whose file does not exist is a **build
  error**, never a silent fallback to the generic list.
- **CI**: the Heltec V4 workflow builds a matrix of the generic target plus
  the per-area envs; artifacts are named per target.
- To add an area: create `init_cmds_<Area>.txt`. Sections
  (`#! … #! end`) work per area file exactly as in the main list, so each
  area's list still grows over time with per-node resume.

## Upgrading from the hash-marker format

Older builds wrote a bare SHA-256 (or `<sha> <label>`) into
`/init_cmds.done`. That string will never match a section name, so the first
boot on this firmware applies nothing — which is the safe default: your node
is already provisioned. To apply new provisioning, add a section with a fresh
name and reflash; the new section runs once. Deleting `/init_cmds.done`
re-runs the whole current list (use only with care: repeated `filter add`
lines create duplicate rules).

## Relationship to /filter_cfg

The filter loads its own binary config from flash in its constructor. Do not
keep filter rules in BOTH `init_cmds.txt` and a separately written
`/filter_cfg` — two sources of truth for one filter is how duplicate rules
happen. With this provisioning path, `init_cmds.txt` is the intended source.

## Security model

The command list is part of the firmware image. It is authored in the repo
and compiled in; it cannot be influenced at runtime or over the air. Running
it is exactly as privileged as the serial console — which is what it is for.

## Testing without hardware

`pio run -e native_packet_filter` style native envs do not execute the
init hook; validate a new list by dry-running the same lines through a real
node's console (`filter list`, `filter get <idx>`) or against the fleet
script path. The generation step is covered by the format rules above:
everything after `#` stripping is handed to the dispatcher verbatim.
