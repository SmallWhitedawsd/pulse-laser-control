---
name: speckit-taskstoissues
description: Convert existing tasks into actionable, dependency-ordered GitHub issues for the feature based on available design artifacts.
---

## User Input
```text
$ARGUMENTS
```

## Outline

1. Run setup script from repo root, parse FEATURE_DIR and AVAILABLE_DOCS list.

2. Load `.specify/memory/constitution.md` if exists.

3. Extract path to tasks.md.

4. Get Git remote: `git config --get remote.origin.url`

> ⚠️ ONLY PROCEED IF THE REMOTE IS A GITHUB URL

5. For each task in tasks.md, create a new GitHub issue in the repository matching the remote URL.

> ⚠️ NEVER CREATE ISSUES IN REPOSITORIES THAT DO NOT MATCH THE REMOTE URL

Each issue should include:
- Title from task description
- Body with task details, file paths, phase info, and dependency context
- Labels corresponding to phase/story (e.g., `phase:setup`, `story:US1`)
