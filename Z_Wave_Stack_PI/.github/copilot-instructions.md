# Copilot Instructions

## Project Guidelines
- Prefer NodeInfo state members to be private and guard NodeInfo public state-related functions with DebugLockGuard.
- Non-listing/sleepy nodes should transition to InterviewDone themselves and perform post-interview jobs, rather than stopping at ProtocolInfoDone.
- Prefer `HandleReport` signatures to place `destinationEP` last, with a default value of `0` on `CC_*` overrides.