---
name: speckit-clarify
description: Identify underspecified areas in the current feature spec by asking up to 5 highly targeted clarification questions and encoding answers back into the spec.
---

## User Input
```text
$ARGUMENTS
```

## Outline

Goal: Detect and reduce ambiguity or missing decision points in the active feature specification and record clarifications directly in the spec file.

1. Run setup script, parse FEATURE_DIR and FEATURE_SPEC.

2. **IF EXISTS**: Load `.specify/memory/constitution.md`.

3. Load the current spec. Perform a structured ambiguity & coverage scan across these categories:
   - Functional Scope & Behavior
   - Domain & Data Model
   - Interaction & UX Flow
   - Non-Functional Quality Attributes (performance, scalability, reliability, observability, security, compliance)
   - Integration & External Dependencies
   - Edge Cases & Failure Handling
   - Constraints & Tradeoffs
   - Terminology & Consistency
   - Completion Signals
   - Misc / Placeholders

4. Generate prioritized queue of max **5** clarification questions. Each must be:
   - Answerable by multiple choice (2-5 options) OR short phrase (≤5 words)
   - Materially impactful to architecture, data model, task decomposition, or test design

5. **Sequential questioning**: Present ONE question at a time.
   - For multiple choice: Recommend the best option with reasoning, then show options table.
   - For short answer: Provide suggested answer with reasoning.
   - User can accept recommendation ("yes"/"recommended") or provide custom answer.

6. **Incremental integration**: After each answer:
   - Create/update `## Clarifications` → `### Session YYYY-MM-DD` section
   - Append: `- Q: <question> → A: <final answer>`
   - Immediately update the relevant spec section(s)

7. Save spec after each integration. Validate: ≤5 questions, no contradictory text, consistent terminology.

8. **Re-validate Spec Quality Checklist** if `FEATURE_DIR/checklists/requirements.md` exists.

## Completion Report
- Number of questions asked & answered
- Path to updated spec, sections touched
- Coverage summary by taxonomy category
- Suggested next command
