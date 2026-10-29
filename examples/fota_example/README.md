# FOTA examples: stale, they do not build

These four examples were written against an API that no longer exists. Measured, not guessed:

    symbols called by example_*.c : 17
    of those defined by components/fota : 0

So every example fails to link (`undefined reference to xy_fota_rollback`, `xy_fota_start_update`,
`xy_fota_deinit`, ...). `FOTA_SRC_DIR` is set and printed but no target ever compiles those sources, and adding
them would not help - the functions they call are not there under any name.

The tree is kept because deleting somebody's material is a decision, not a cleanup. It is also not part of the
root build (`project(fota_examples C)` with no `add_subdirectory` pointing here), so it cannot turn a gate red
by accident.

Two things that do work, so nobody has to rediscover them:

- `-DBUILD_EXAMPLES=OFF` now actually skips the examples. Before, the option was read by nothing and OFF built
  all four anyway.
- `install()` names only the targets that were really created, so a missing `example_*.c` is a skipped example
  rather than an install of a target that does not exist.

What to do with this directory, in order of preference:

1. Rewrite against the current `components/fota` API.
2. Delete it, since a runnable example of an API that does not exist teaches the wrong thing.
3. Leave it here, stale and labelled, as it is now.
