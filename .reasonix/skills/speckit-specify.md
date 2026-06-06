---
name: speckit-specify
description: Create or update the feature specification from a natural language feature description.
---

## User Input

```text
$ARGUMENTS
```

You **MUST** consider the user input before proceeding (if not empty).

## Outline

The text the user typed after the command **is** the feature description. Assume you always have it available in this conversation. Do not ask the user to repeat it unless they provided an empty command.

Given that feature description, do this:

1. **Generate a concise short name** (2-4 words) for the feature using action-noun format.

2. **Create the spec feature directory** under `specs/`:
   - Use sequential numbering: `NNN` (next available 3-digit number)
   - Construct directory name: `<NNN>-<short-name>`
   - `mkdir -p specs/<directory-name>`

3. **IF EXISTS**: Load `.specify/memory/constitution.md` for project principles and governance constraints.

4. Follow this execution flow:
   1. Parse user description from arguments. If empty: ERROR "No feature description provided"
   2. Extract key concepts: actors, actions, data, constraints
   3. For unclear aspects, make informed guesses based on context. Only mark with [NEEDS CLARIFICATION] if the choice significantly impacts scope/UX. **LIMIT: Maximum 3 [NEEDS CLARIFICATION] markers.**
   4. Fill User Scenarios & Testing section
   5. Generate Functional Requirements (each must be testable)
   6. Define Success Criteria (measurable, technology-agnostic outcomes)
   7. Identify Key Entities (if data involved)

5. Write the specification to `specs/<dir>/spec.md`.

6. **Specification Quality Validation**: Generate checklist at `specs/<dir>/checklists/requirements.md` and validate. If items fail, update spec and re-run (max 3 iterations).

## Quick Guidelines

- Focus on **WHAT** users need and **WHY**.
- Avoid HOW to implement (no tech stack, APIs, code structure).
- Written for business stakeholders, not developers.
- DO NOT create checklists embedded in the spec.

### Success Criteria Guidelines
Success criteria must be: Measurable, Technology-agnostic, User-focused, Verifiable.

**Good**: "Users can complete checkout in under 3 minutes"
**Bad**: "API response time is under 200ms"
