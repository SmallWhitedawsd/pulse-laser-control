---
name: speckit-tasks
description: Generate an actionable, dependency-ordered tasks.md for the feature based on available design artifacts (plan.md, spec.md, data-model.md, contracts/, research.md).
---

## User Input

```text
$ARGUMENTS
```

You **MUST** consider the user input before proceeding (if not empty).

## Outline

1. **Setup**: Run setup script from repo root and parse FEATURE_DIR, TASKS_TEMPLATE, and AVAILABLE_DOCS list.

2. **Load design documents**: Read from FEATURE_DIR:
   - **Required**: plan.md (tech stack, libraries, structure), spec.md (user stories with priorities)
   - **Optional**: data-model.md, contracts/, research.md, quickstart.md
   - **IF EXISTS**: Load `.specify/memory/constitution.md`

3. **Execute task generation workflow**:
   - Load plan.md and extract tech stack, libraries, project structure
   - Load spec.md and extract user stories with priorities (P1, P2, P3...)
   - Generate tasks organized by user story
   - Generate dependency graph
   - Create parallel execution examples per user story

4. **Generate tasks.md** using the template structure:
   - Phase 1: Setup tasks (project initialization)
   - Phase 2: Foundational tasks (blocking prerequisites for all user stories)
   - Phase 3+: One phase per user story (in priority order)
   - Final Phase: Polish & cross-cutting concerns
   - Dependencies section, parallel execution examples, MVP-first strategy

## Task Generation Rules

**CRITICAL**: Tasks MUST be organized by user story for independent implementation.

### Checklist Format (REQUIRED)
```text
- [ ] [TaskID] [P?] [Story?] Description with file path
```

- **Task ID**: Sequential number (T001, T002...)
- **[P] marker**: Include ONLY if parallelizable
- **[Story] label**: [US1], [US2]... for user story phase tasks

**Examples**:
- ✅ `- [ ] T001 Create project structure per implementation plan`
- ✅ `- [ ] T012 [P] [US1] Create User model in src/models/user.py`
- ❌ `- [ ] Create User model` (missing ID and Story label)

### Phase Structure
- Phase 1: Setup (project initialization)
- Phase 2: Foundational (blocking prerequisites)
- Phase 3+: User Stories (P1, P2, P3...) — each independently testable
- Final Phase: Polish & Cross-Cutting Concerns
