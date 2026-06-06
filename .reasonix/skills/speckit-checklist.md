---
name: speckit-checklist
description: Generate a custom checklist for the current feature — "unit tests for English" that validate requirements quality (completeness, clarity, consistency) rather than implementation.
---

## User Input
```text
$ARGUMENTS
```

## Checklist Purpose: "Unit Tests for English"

**CRITICAL**: Checklists are **UNIT TESTS FOR REQUIREMENTS WRITING** — they validate quality, clarity, and completeness of requirements. NOT for verification/testing of implementation.

- ❌ NOT "Verify the button clicks correctly"
- ✅ "Are visual hierarchy requirements defined for all card types?" (completeness)
- ✅ "Is 'prominent display' quantified with specific sizing?" (clarity)
- ✅ "Are hover state requirements consistent across all interactive elements?" (consistency)

## Execution Steps

1. **Setup**: Run setup script, parse FEATURE_DIR and AVAILABLE_DOCS.

2. Load `.specify/memory/constitution.md` if exists.

3. **Clarify intent**: Ask up to 3 clarifying questions about focus areas, depth, audience, and boundaries.

4. **Load feature context**: spec.md, plan.md (if exists), tasks.md (if exists).

5. **Generate checklist** — "Unit Tests for Requirements":
   - Create `FEATURE_DIR/checklists/` directory
   - Generate unique filename: `[domain].md` (e.g., `ux.md`, `api.md`, `security.md`)
   - If file exists: APPEND items (continue CHK numbering)
   - Every item evaluates REQUIREMENTS quality: Completeness, Clarity, Consistency, Measurability, Coverage

6. **Item Structure**:
   - Question format asking about requirement quality
   - Include quality dimension in brackets: [Completeness], [Clarity], [Consistency], [Coverage], [Gap], [Ambiguity]
   - Reference spec section `[Spec §X.Y]` or markers

### ✅ CORRECT patterns:
- "Are [requirement type] defined/specified/documented for [scenario]?"
- "Is [vague term] quantified/clarified with specific criteria?"
- "Are requirements consistent between [section A] and [section B]?"
- "Can [requirement] be objectively measured/verified?"

### ❌ PROHIBITED (implementation tests):
- Any item starting with "Verify", "Test", "Confirm", "Check" + implementation behavior
- "Displays correctly", "works properly", "functions as expected"
- References to code execution, user actions, system behavior

7. **Report**: Full path, item count, focus areas, depth level.
