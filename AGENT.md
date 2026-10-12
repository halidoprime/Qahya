# AGENT.md

## Project Overview

QControl is a GUI application for controlling Linux services (systemctl, ufw, etc.).
It is developed with C++ and Qt. 

## Project Rules

1. **Do not compile the project without the user's explicit permission.**
   Never run `cmake`, `make`, `ninja`, or any build command unless the user asks for it.

2. **Do not make changes the user didn't order.**
   Only modify, create, or delete files/code as explicitly requested.
   No refactoring, reformatting, or "drive-by" fixes beyond the scope of the request.

3. **Report what you did and what you found.**
   After every task, summarize:
   - Files changed/created and why
   - Any issues, bugs, or notable findings discovered in the codebase

## Build System

- CMake (minimum 3.19), Qt6 (>= 6.5) with Core, Widgets, and LinguistTools components
- Build directory: `build/` (out-of-source builds only)
- Translation file: `QControl_en_US.ts` (updated via `qt_add_translations`)

## Important Notes

- This app controls system services (systemctl, ufw), so many operations require
  root/polkit privileges — be careful with any code that elevates privileges.
- Keep the UI and business logic (service control wrappers) cleanly separated.
