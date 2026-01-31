## Agent Guide

- Use English for every README and comment.
- Keep directory nesting to a maximum of 3 levels; prefer flat structures.
- Limit code nesting to 3 blocks; after that, extract logic into a public/helper function or use early returns to flatten control flow.
- Prioritize performance and stability: measure hot paths, avoid unnecessary allocations, and guard concurrency with clear ownership rules.
- Design for modularity: small, single-purpose components with explicit interfaces and no hidden side effects.
- Apply modern threading practices: prefer tasks/futures over raw threads, avoid shared mutable state, and document synchronization points.
- Write modern, readable code: consistent naming, clean syntax, and self-documenting structures over comments when possible.
- Avoid duplicate code: extract reusable logic into shared helpers.
- Reference materials inside this folder: `README.md` (AI rules), `dev-docs.md` (developer + git workflow), `user-docs.md` (user guide).

### IDE visibility (Rider)
- After new files/folders are created, trigger `File -> Synchronize` (`Ctrl+Alt+Y`) so Rider refreshes the project view.
- If the project model seems stale, use `File -> Reload All from Disk` to rescan without restarting.
- `AiDocs` is added to the solution as a Solution Folder so docs show in the Solution view; if it ever disappears, right-click the solution -> `Add` -> `Add Solution Folder` -> include the markdown files.
- Keep `AiDocs` included as a content root so documentation changes are indexed.

### IDE diagnostics without building
- Use Rider’s on-the-fly analysis: enable `Solution-Wide Analysis` (status bar bulb) to surface errors/warnings live without running a build.
- Open the `Problems` tool window (`Alt+6`) or `Inspection Results` to see current errors/warnings collected from analysis.
- For targeted files, run `Code -> Inspect Code...` on selection/solution for a quick static pass without compiling.

### Automated diagnostics scripts (no Rider required)
- `scripts/run-inspectcode.ps1` — runs JetBrains InspectCode (no-build SWEA) and writes `scripts/reports/inspectcode.xml`. Set `RIDERCMD` env var to `inspectcode.exe` if auto-detect fails.
- `scripts/run-cppcheck.ps1` — runs cppcheck (`--enable=warning,performance,style`) over core + tests; output `scripts/reports/cppcheck.xml`. Requires `cppcheck` in PATH.
- `scripts/run-clang-tidy.ps1` — runs clang-tidy on core targets using `compile_commands.json`; output `scripts/reports/clang-tidy.txt`. Requires clang-tidy and a generated compilation DB.
- `scripts/build-and-log.ps1` — builds via msbuild and tees output to `scripts/reports/build.log`. Use `/p:Configuration` and `/p:Platform` params as needed.
- `scripts/quick-smoke.ps1` — fastest option; runs cppcheck + clang-tidy + InspectCode (all no-build) if tools are available, drops logs into `scripts/reports/quick-*` and a `quick-summary.txt` you can skim between steps.

### AI readability
- Keep docs in plain ASCII markdown; favor concise tables/bullets so analysis tools (and AI) parse quickly.

### AI Quick Reference Table
| Topic | Guideline | Why it matters |
| --- | --- | --- |
| Nesting depth | Keep <=3 levels; refactor deeper logic into helpers/public functions. | Improves readability and testability. |
| Performance | Identify hot paths, minimize allocations, and cache smartly. | Reduces latency and resource use. |
| Stability | Validate inputs, fail fast, and add clear error handling. | Prevents crashes and eases debugging. |
| Modularity | Small, composable units with explicit interfaces. | Enables reuse and easier changes. |
| Threading | Prefer tasks/futures; avoid shared mutable state; document locks. | Safer concurrency and fewer data races. |
| Modern practices | Use idiomatic language features and clean syntax. | Keeps codebase current and readable. |
