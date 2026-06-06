---
name: speckit-analyze
description: Perform a non-destructive cross-artifact consistency and quality analysis across spec.md, plan.md, and tasks.md after task generation.
---

## User Input
```text
$ARGUMENTS
```

## Goal

Identify inconsistencies, duplications, ambiguities, and underspecified items across spec.md, plan.md, and tasks.md before implementation. **STRICTLY READ-ONLY** — do not modify any files.

**Constitution Authority**: Constitution conflicts are automatically CRITICAL.

## Execution Steps

1. **Initialize**: Run setup script, parse FEATURE_DIR. Derive SPEC, PLAN, TASKS paths. Abort if any missing.

2. **Load Artifacts** (progressive disclosure):
   - From spec.md: Overview, Functional Requirements, Success Criteria, User Stories, Edge Cases
   - From plan.md: Architecture/stack, Data Model, Phases, Technical constraints
   - From tasks.md: Task IDs, Descriptions, Phase grouping, Parallel markers [P], File paths
   - From constitution: Principle names and MUST/SHOULD normative statements

3. **Build Semantic Models** (internal, not in output):
   - Requirements inventory with stable keys (FR-###, SC-###)
   - User story/action inventory
   - Task coverage mapping: each task → requirements/stories
   - Constitution rule set

4. **Detection Passes** (max 50 findings):
   - **A. Duplication**: Near-duplicate requirements
   - **B. Ambiguity**: Vague adjectives (fast, scalable, secure), unresolved placeholders (TODO, TKTK, ???)
   - **C. Underspecification**: Missing object/outcome in requirements, tasks referencing undefined components
   - **D. Constitution Alignment**: Conflicts with MUST principles
   - **E. Coverage Gaps**: Requirements with zero tasks, Success Criteria without buildable tasks
   - **F. Inconsistency**: Terminology drift, conflicting requirements, task ordering contradictions

5. **Severity**: CRITICAL > HIGH > MEDIUM > LOW

6. **Report Structure**:

```
## Specification Analysis Report

| ID | Category | Severity | Location(s) | Summary | Recommendation |
|----|----------|----------|-------------|---------|----------------|

**Coverage Summary Table**
**Constitution Alignment Issues**
**Unmapped Tasks**
**Metrics**: Total Requirements, Total Tasks, Coverage %, Ambiguity Count, Duplication Count, Critical Issues Count
```

7. **Next Actions**: If CRITICAL issues → resolve before implement; if only LOW/MEDIUM → may proceed with suggestions.

8. **Remediation Offer**: Ask user if they want concrete remediation edits for top N issues (do NOT auto-apply).
