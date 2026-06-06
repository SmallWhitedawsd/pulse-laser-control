---
name: speckit-constitution
description: Create or update the project constitution from interactive or provided principle inputs, ensuring all dependent templates stay in sync.
---

## User Input

```text
$ARGUMENTS
```

You **MUST** consider the user input before proceeding (if not empty).

## Outline

You are updating the project constitution at `.specify/memory/constitution.md`. This file is a TEMPLATE containing placeholder tokens in square brackets (e.g. `[PROJECT_NAME]`, `[PRINCIPLE_1_NAME]`). Your job is to (a) collect/derive concrete values, (b) fill the template precisely, and (c) propagate any amendments across dependent artifacts.

Follow this execution flow:

1. Load the existing constitution at `.specify/memory/constitution.md`.
   - Identify every placeholder token of the form `[ALL_CAPS_IDENTIFIER]`.
   **IMPORTANT**: The user might require less or more principles than the ones used in the template. If a number is specified, respect that - follow the general template.

2. Collect/derive values for placeholders:
   - If user input (conversation) supplies a value, use it.
   - Otherwise infer from existing repo context (README, docs, prior constitution versions if embedded).
   - `CONSTITUTION_VERSION` must increment according to semantic versioning rules:
     - MAJOR: Backward incompatible governance/principle removals or redefinitions.
     - MINOR: New principle/section added or materially expanded guidance.
     - PATCH: Clarifications, wording, typo fixes, non-semantic refinements.

3. Draft the updated constitution content, replacing every placeholder with concrete text.

4. Consistency propagation checklist:
   - Check `.specify/templates/plan-template.md`, `spec-template.md`, `tasks-template.md` for alignment.
   - Check command files in `.specify/templates/commands/*.md` for outdated references.

5. Produce a Sync Impact Report (HTML comment at top of constitution file).

6. Validation: No remaining bracket tokens, version matches report, dates ISO format.

7. Write the completed constitution back to `.specify/memory/constitution.md`.

8. Output final summary: new version, bump rationale, files flagged for follow-up, suggested commit message.

If critical info missing, insert `TODO(<FIELD_NAME>): explanation` and include in the Sync Impact Report.
