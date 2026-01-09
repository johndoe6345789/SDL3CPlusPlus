Always build and test. 
Maintain the full testing pyramid: unit tests, static analysis, 
linters, integration tests, and end-to-end coverage. 
Continuously execute against the roadmap.

Keep files small and focused. 
Target <100 LOC per file unless there is a clear and 
documented reason otherwise. 
Enforce one responsibility per file.

Decompose aggressively. 
Break up large blobs of logic into composable units. 
Large files are a smell.

Use structured architecture. 
Prefer plugin / service / controller patterns with explicit dependency injection.

Favor declarative designs. 
Declarative workflows are strongly preferred over imperative glue code.

Everything as a workflow. 
If something is not part of a workflow, question whether it should be executed at all. 
Even complex systems (including game engines) can be expressed this way.

Eliminate repetition. 
Loop, parameterize, and abstract instead of copy-pasting similar code.

Externalize data. 
If you see a list, map, or configuration embedded in code, 
it likely belongs in JSON (or equivalent).

Modular configuration. 
If JSON files grow large, 
split them and reference them hierarchically.

Ship templates, not chores. 
Package concepts as reusable templates instead of forcing users to configure from scratch.

Logging is mandatory. 
Use structured logging with clear 
levels (DEBUG / INFO / TRACE / ERROR) throughout the system.

Declarative workflows first. 
Increase usage wherever possible.

Innovate continuously. 
Avoid stagnation; improve structure, clarity, and expressiveness over time.

Roadmap discipline. 
If you run out of ROADMAP.md tasks, stop and report. 
We should reassess priorities before inventing work.
