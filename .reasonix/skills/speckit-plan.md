---
name: speckit-plan
description: Execute the implementation planning workflow using the plan template to generate design artifacts (research.md, data-model.md, contracts/, quickstart.md).
---

## User Input

```text
$ARGUMENTS
```

You **MUST** consider the user input before proceeding (if not empty).

## Outline

1. **Setup**: Run setup script from repo root and parse JSON for FEATURE_SPEC, IMPL_PLAN, SPECS_DIR, BRANCH.

2. **Load context**: Read FEATURE_SPEC and `.specify/memory/constitution.md`. Load IMPL_PLAN template.

3. **Execute plan workflow**: Follow the structure in IMPL_PLAN template to:
   - Fill Technical Context (mark unknowns as "NEEDS CLARIFICATION")
   - Fill Constitution Check section from constitution
   - Evaluate gates (ERROR if violations unjustified)
   - Phase 0: Generate research.md (resolve all NEEDS CLARIFICATION)
   - Phase 1: Generate data-model.md, contracts/, quickstart.md
   - Phase 1: Update agent context
   - Re-evaluate Constitution Check post-design

## Phases

### Phase 0: Outline & Research
1. Extract unknowns from Technical Context
2. Generate and dispatch research tasks for each unknown
3. Consolidate findings in `research.md`:
   - Decision: [what was chosen]
   - Rationale: [why chosen]
   - Alternatives considered: [what else evaluated]

### Phase 1: Design & Contracts
1. Extract entities from feature spec → `data-model.md`
2. Define interface contracts → `/contracts/` (if project has external interfaces)
3. Create quickstart validation guide → `quickstart.md`
4. Update agent context file

## Key rules
- Use absolute paths for filesystem operations
- ERROR on gate failures or unresolved clarifications
