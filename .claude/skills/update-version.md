---
name: update-version
description: Update the project version following semantic versioning
version: 0.1.0
author: Claude
tags: [version, release, mojibake]
---

# Update Version Skill

This skill helps you increment the project version in the VERSION file following semantic versioning.

## Version File

- The version is stored in the `VERSION` file at the project root
- Format: `MAJOR.MINOR.PATCH` (e.g., `0.1.0`)
- Follows semantic versioning (semver) conventions

## Instructions

When this skill is invoked, follow these steps:

1. **Read the current version**:
   - Read the `VERSION` file to get the current version
   - Parse it into MAJOR.MINOR.PATCH components

2. **Ask for version bump type**:
   - Ask the user which component to increment:
     - **Patch** (0.1.0 → 0.1.1): Bug fixes and minor changes
     - **Minor** (0.1.9 → 0.2.0): New features, backwards compatible
     - **Major** (0.9.9 → 1.0.0): Breaking changes
   - If not specified, default to patch increment

3. **Calculate the new version**:
   - **Patch increment**: Increment the patch number (rightmost)
     - If patch is 9, increment minor and reset patch to 0
   - **Minor increment**: Increment the minor number (middle), reset patch to 0
     - If minor is 9, increment major and reset minor and patch to 0
   - **Major increment**: Increment the major number (leftmost), reset minor and patch to 0

4. **Update the VERSION file**:
   - Write the new version to the `VERSION` file
   - Keep the trailing newline

5. **Run the update script**:
   - Execute: `make update-version`
   - This will propagate the version change to other files in the project

6. **Confirm**:
   - Show the user the version change (e.g., "Updated version: 0.1.0 → 0.1.1")

## Examples

### Patch increment (most common)
```
0.1.0 → 0.1.1
0.1.9 → 0.2.0  (rollover)
```

### Minor increment
```
0.1.5 → 0.2.0
0.9.3 → 1.0.0  (rollover to major)
```

### Major increment
```
0.5.3 → 1.0.0
1.2.9 → 2.0.0
```

## Notes

- Always maintain the format `MAJOR.MINOR.PATCH` with exactly 3 components
- The `make update-version` command updates version references throughout the codebase
- Keep the trailing newline in the VERSION file
