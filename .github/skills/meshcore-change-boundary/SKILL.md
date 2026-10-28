---
name: meshcore-change-boundary
description: "Use when working in MeshCore and deciding where to implement fixes; prefer build/workflow/config changes and avoid core firmware code edits unless explicitly requested by the user."
---

# MeshCore Change Boundary

## Scope

This skill applies to repository-level change strategy in MeshCore.

## Rule

- Prefer minimal changes to build/workflow/configuration surfaces first.
- Treat core firmware code edits as a last resort.
- Only modify core firmware code when the user explicitly asks for it.

## Practical Guidance

- Preferred surfaces:
  - `.github/workflows/**`
  - build orchestration and target selection logic
  - project config and CI matrix shaping
- Avoid touching runtime firmware paths unless explicitly requested:
  - `src/**`
  - `examples/**` firmware runtime logic

## Rationale

This keeps risk low, limits behavioral drift in upstream-sensitive firmware code, and makes CI/build policy changes easier to reason about and review.
