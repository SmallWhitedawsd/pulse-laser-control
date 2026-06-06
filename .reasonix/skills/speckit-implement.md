---
name: speckit-implement
description: Execute the implementation plan by processing and executing all tasks defined in tasks.md phase by phase.
---

## User Input

```text
$ARGUMENTS
```

You **MUST** consider the user input before proceeding (if not empty).

## Outline

1. Run setup script from repo root and parse FEATURE_DIR and AVAILABLE_DOCS list.

2. **Check checklists status** (if FEATURE_DIR/checklists/ exists):
   - If any checklist incomplete: STOP and ask user "Proceed anyway? (yes/no)"
   - If all complete: Display summary and proceed

3. Load implementation context:
   - **REQUIRED**: tasks.md, plan.md
   - **IF EXISTS**: data-model.md, contracts/, research.md, constitution.md, quickstart.md

4. **Project Setup Verification**: Create/verify ignore files (.gitignore, .dockerignore, etc.) based on detected tech stack.

5. Parse tasks.md structure: phases, dependencies, parallel markers

6. Execute implementation phase-by-phase:
   - **Phase-by-phase**: Complete each phase before moving to next
   - **Respect dependencies**: Sequential in order, [P] tasks can run together
   - **TDD approach**: Execute test tasks before implementation tasks
   - **File-based coordination**: Same-file tasks must run sequentially

7. Implementation execution rules:
   - Setup first → Tests before code → Core development → Integration → Polish

8. Progress tracking:
   - Report after each completed task
   - Halt on non-parallel task failure
   - Mark completed tasks as `[X]` in tasks.md

9. Completion validation:
   - Verify all required tasks completed
   - Check features match specification
   - Validate tests pass and coverage meets requirements
